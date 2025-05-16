#include <SQG/API/PackageUtils.h>
#include <SQG/API/DialogUtils.h>
#include <SQG/API/QuestUtils.h>
#include <DPF/API.h>

void InitializeLog()
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = SKSE::log::log_directory();
	if (!path) 
	{
		util::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format("{}.log"sv, Plugin::NAME);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
	constexpr auto level = spdlog::level::trace;
#else
	constexpr auto level = spdlog::level::info;
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(level);
	log->flush_on(level);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);
}

RE::TESQuest* referenceQuest = nullptr;
RE::TESQuest* generatedQuest = nullptr;
RE::TESQuest* selectedQuest = nullptr;
RE::TESObjectREFR* targetForm = nullptr;
RE::TESObjectREFR* activator = nullptr;
RE::TESObjectREFR* targetActivator = nullptr;
RE::TESPackage* acquirePackage = nullptr;
RE::TESPackage* activatePackage = nullptr;
RE::TESPackage* travelPackage = nullptr;
RE::TESPackage* customAcquirePackage = nullptr;
RE::TESPackage* customActivatePackage = nullptr;
RE::TESPackage* customTravelPackage = nullptr;
RE::TESTopicInfo* subTopicsInfos[SUB_TOPIC_COUNT] { nullptr };

RE::TESConditionItem* impossibleCondition = nullptr;

std::map<RE::FormID, std::uint16_t> lastValidLogEntryIndexes;
char* targetLogEntry = nullptr;

struct SpeakerData
{
	std::map<RE::FormID, DialogTopicData::AnswerData*> topicsInfosBindings;
	DialogTopicData::AnswerData* lastSelectedAnswer = nullptr;
	bool hasAnyValidEntry = false;
};
std::map<RE::FormID, SpeakerData> speakersData;

bool ProcessDialogEntry(RE::TESObjectREFR* inSpeaker, DialogTopicData& inDialogEntry, RE::TESTopicInfo* inOutTopicInfo)
{
	bool hasAnyValidAnswer = false;
	for(auto& answerData : inDialogEntry.answers)
	{
		RE::TESCondition condition;
		RE::TESConditionItem** conditionItemHolder = &condition.head;
		for(const auto* conditionItem : answerData.conditions)
		{
			*conditionItemHolder = new RE::TESConditionItem();
			(*conditionItemHolder)->data = conditionItem->data;
			conditionItemHolder = &((*conditionItemHolder)->next);
		}
		*conditionItemHolder = nullptr;


		if(condition(inSpeaker, inSpeaker))
		{
			inOutTopicInfo->objConditions.head = nullptr;
			speakersData[inSpeaker->formID].topicsInfosBindings[inOutTopicInfo->formID] = &answerData;
			inOutTopicInfo->parentTopic->fullName = answerData.promptOverride.empty() ? inDialogEntry.prompt : answerData.promptOverride;
			hasAnyValidAnswer = true;
			break;
		}
	}

	if(!hasAnyValidAnswer)
	{
		inOutTopicInfo->objConditions.head = new RE::TESConditionItem();
		std::memcpy(inOutTopicInfo->objConditions.head, impossibleCondition, sizeof(RE::TESConditionItem));  // NOLINT(bugprone-undefined-memory-manipulation)
	}
	return hasAnyValidAnswer;
}

void FillQuestWithGeneratedData(RE::TESQuest* inQuest)
{
	//Parametrize quest
	//=======================
	inQuest->SetFormEditorID("SQGTestQuest");
	inQuest->fullName = "00_SQG_POC";

	//Add stages
	//=======================
	AddQuestStage(inQuest, 10, QuestStageType::Startup, "My boss told me to kill a man named \"Gibier\"");
	AddQuestStage(inQuest, 45, QuestStageType::Shutdown, "I decided to spare Gibier.");
	AddQuestStage(inQuest, 40, QuestStageType::Shutdown, "Gibier is dead.");
	AddQuestStage(inQuest, 35, QuestStageType::Default, "When I told Gibier I was going to kill him he asked for a last will. I refused.");
	AddQuestStage(inQuest, 32, QuestStageType::Default, "Gibier has done his last will. The time has came for him to die.");
	AddQuestStage(inQuest, 30);
	AddQuestStage(inQuest, 20);
	AddQuestStage(inQuest, 15, QuestStageType::Default, "When I told Gibier I was going to kill him he asked for a last will. I let him do what he wanted but advised him to not do anything inconsiderate.");
	AddQuestStage(inQuest, 12, QuestStageType::Default, "I spoke with Gibier, whom told me my boss was a liar and begged me to spare him. I need to decide what to do.");

	//Add objectives
	//=======================
	AddObjective(inQuest, 10, "Kill Gibier", {0});
	AddObjective(inQuest, 12, "(Optional) Spare Gibier");
	AddObjective(inQuest, 15, "(Optional) Let Gibier do his last will");

	//Add aliases
	//=======================
	AddRefAlias(inQuest, 0, "SQGTestAliasTarget", targetForm);

	//Add dialogs
	//===========================

	auto* checkSpeakerCondition = new RE::TESConditionItem();
	checkSpeakerCondition->data.dataID = std::numeric_limits<std::uint32_t>::max();
	checkSpeakerCondition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
	checkSpeakerCondition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kGetIsID);
	checkSpeakerCondition->data.functionData.params[0] = targetForm->data.objectReference;
	checkSpeakerCondition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
	checkSpeakerCondition->data.comparisonValue.f = 1.f;
	checkSpeakerCondition->next = nullptr;

	auto* aboveStage0Condition = new RE::TESConditionItem();
	aboveStage0Condition->data.dataID = std::numeric_limits<std::uint32_t>::max();
	aboveStage0Condition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
	aboveStage0Condition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kGetStage);
	aboveStage0Condition->data.functionData.params[0] = inQuest;
	aboveStage0Condition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kGreaterThan;
	aboveStage0Condition->data.comparisonValue.f = 0.f;
	aboveStage0Condition->next = nullptr;

	auto* underStage12Condition = new RE::TESConditionItem();
	underStage12Condition->data.dataID = std::numeric_limits<std::uint32_t>::max();
	underStage12Condition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
	underStage12Condition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kGetStage);
	underStage12Condition->data.functionData.params[0] = inQuest;
	underStage12Condition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kLessThan;
	underStage12Condition->data.comparisonValue.f = 12.f;
	underStage12Condition->next = nullptr;

	auto* aboveStage12Condition = new RE::TESConditionItem();
	aboveStage12Condition->data.dataID = std::numeric_limits<std::uint32_t>::max();
	aboveStage12Condition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
	aboveStage12Condition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kGetStage);
	aboveStage12Condition->data.functionData.params[0] = inQuest;
	aboveStage12Condition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kGreaterThanOrEqualTo;
	aboveStage12Condition->data.comparisonValue.f = 12.f;
	aboveStage12Condition->next = nullptr;

	auto* underStage15Condition = new RE::TESConditionItem();
	underStage15Condition->data.dataID = std::numeric_limits<std::uint32_t>::max();
	underStage15Condition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
	underStage15Condition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kGetStage);
	underStage15Condition->data.functionData.params[0] = inQuest;
	underStage15Condition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kLessThan;
	underStage15Condition->data.comparisonValue.f = 15.f;
	underStage15Condition->next = nullptr;
	
	auto* wonderEntry = AddDialogTopic(inQuest, targetForm, "I've not decided yet. I'd like to hear your side of the story.");
	wonderEntry->AddAnswer
	(
		"Thank you very much, you'll see that I don't diserve this cruelty. Your boss is a liar.",
		"",
		{checkSpeakerCondition, aboveStage0Condition, underStage12Condition}
	);
	wonderEntry->AddAnswer
	(
		"Your boss is a liar.",
		"Tell me again about the reasons of the contract",
		{checkSpeakerCondition, aboveStage0Condition, underStage15Condition}
	);
	auto* moreWonderEntry = AddDialogTopic(inQuest, targetForm, "What did he do ?", wonderEntry);
	moreWonderEntry->AddAnswer
	(
		"He lied, I'm the good one in the story.",
		"",
		{},
		12,
		0
	);

	auto* attackEntry = AddDialogTopic(inQuest, targetForm, "As you guessed. Prepare to die !");
	attackEntry->AddAnswer
	(
		"Wait a minute ! Could I have a last will please ?",
		"",
		{checkSpeakerCondition, aboveStage0Condition, underStage12Condition}
	);
	attackEntry->AddAnswer
	(
		"Wait a minute ! Could I have a last will please ?",
		"I can't believe my boss would lie. Prepare to die !",
		{checkSpeakerCondition, aboveStage0Condition, underStage15Condition}
	);
	auto* lastWillYesEntry = AddDialogTopic(inQuest, targetForm, "Yes, of course, proceed but don't do anything inconsiderate.", attackEntry);
	lastWillYesEntry->AddAnswer
	(						
		"Thank you for your consideration",
		"",
		{},
		15,
		1
	);
	auto* lastWillNoEntry = AddDialogTopic(inQuest, targetForm, "No, I came here for business, not charity.", attackEntry);
	lastWillNoEntry->AddAnswer
	(
		"Your lack of dignity is a shame.",
		"",
		{},
		35,
		2
	);

	auto* spareEntry = AddDialogTopic(inQuest, targetForm, "Actually, I decided to spare you.");
	spareEntry->AddAnswer
	(
		"You're the kindest. I will make sure to hide myself from the eyes of your organization.",
		"",
		{checkSpeakerCondition, aboveStage12Condition, underStage15Condition},
		45,
		3
	);
}

void AttachScriptsToQuest(const RE::TESQuest* inQuest)
{
	//Add script logic
	//===========================
	auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	auto* policy = scriptMachine->GetObjectHandlePolicy();

	const auto selectedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, inQuest);
	//TODO!! try to call script compiler from c++ before loading custom script
	RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
	scriptMachine->CreateObjectWithProperties("SQGDebug", 1, questCustomScriptObject);
	scriptMachine->BindObject(questCustomScriptObject, selectedQuestHandle, false);

	RE::BSTSmartPointer<RE::BSScript::Object> referenceAliasBaseScriptObject;
	scriptMachine->CreateObject("SQGQuestTargetScript", referenceAliasBaseScriptObject);
	const auto questAliasHandle = selectedQuestHandle & 0x0000FFFFFFFF;
	scriptMachine->BindObject(referenceAliasBaseScriptObject, questAliasHandle, false);

	RE::BSScript::Variable propertyValue;
	propertyValue.SetObject(referenceAliasBaseScriptObject);
	scriptMachine->SetPropertyValue(questCustomScriptObject, "SQGTestAliasTarget", propertyValue);
	questCustomScriptObject->initialized = true; //Required when creating object with properties

	RE::BSTSmartPointer<RE::BSScript::Object> dialogFragmentsCustomScriptObject;
	scriptMachine->CreateObject("SQGTopicFragments", dialogFragmentsCustomScriptObject);
	scriptMachine->BindObject(dialogFragmentsCustomScriptObject, selectedQuestHandle, false);
}

std::string GenerateQuest(RE::StaticFunctionTag*)
{
	//Create quest if needed
	//=======================
	if(generatedQuest)
	{
		return "Quest yet generated.";
	}

	selectedQuest = generatedQuest = DPF::CreateForm(referenceQuest);

	FillQuestWithGeneratedData(generatedQuest);
	AttachScriptsToQuest(generatedQuest);

	//Notify success
	//===========================
	std::ostringstream ss;
    ss << std::hex << generatedQuest->formID;
	const auto formId = ss.str();
	SKSE::log::debug("Generated quest with formID: {0}"sv, formId);
	return "Generated quest with formID: " + formId;
}

class QuestStageEventSink final : public RE::BSTEventSink<RE::TESQuestStageEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestStageEvent* a_event, RE::BSTEventSource<RE::TESQuestStageEvent>* a_eventSource) override
	{
		auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* policy = scriptMachine->GetObjectHandlePolicy();

		const auto* updatedQuest = RE::TESForm::LookupByID<RE::TESQuest>(a_event->formID);
		if(updatedQuest != generatedQuest) //TODO find a proper way to bypass unwanted events
		{
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto updatedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, updatedQuest);
		RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
		scriptMachine->FindBoundObject(updatedQuestHandle, "SQGDebug", questCustomScriptObject);

		if(a_event->stage == 10)
		{
			const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter();
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}
		else if(a_event->stage == 12)
		{
			const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() + 1;
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}
		else if(a_event->stage == 15)
		{
			const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() + 2;
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}
		else if(a_event->stage == 32)
		{
			const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() + 3;
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}
		else if(a_event->stage == 35)
		{
			const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() + 4;
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}
		else if(a_event->stage == 40)
		{
			const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() + 5;
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}
		else if(a_event->stage == 45)
		{
			const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() + 6;
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}

		return RE::BSEventNotifyControl::kStop;
	}
};

class QuestInitEventSink : public RE::BSTEventSink<RE::TESQuestInitEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestInitEvent* a_event, RE::BSTEventSource<RE::TESQuestInitEvent>* a_eventSource) override
	{
		if(auto* updatedQuest = RE::TESForm::LookupByID<RE::TESQuest>(a_event->formID); updatedQuest == generatedQuest) //TODO find a proper way to bypass unwanted events
		{
			if(const auto* aliasInstancesList = reinterpret_cast<RE::ExtraAliasInstanceArray*>(targetForm->extraList.GetByType(RE::ExtraDataType::kAliasInstanceArray)))
			{
				aliasInstancesList->lock.LockForRead();
				for(auto* aliasInstanceData : aliasInstancesList->aliases)
				{
					if(aliasInstanceData->quest == generatedQuest)
					{
						auto* instancedPackages = new RE::BSTArray<RE::TESPackage*>(); //Done in two time to deal with constness
						aliasInstanceData->instancedPackages = instancedPackages;

						auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
						auto* policy = scriptMachine->GetObjectHandlePolicy();

						{
							//ACQUIRE PACKAGE
							//=============================

							customAcquirePackage = SQG::CreatePackageFromTemplate(DPF::CreateForm(acquirePackage), acquirePackage, generatedQuest);

							std::unordered_map<std::string, SQG::PackageData> packageDataMap;
							RE::PackageTarget::Target targetData{};
							targetData.objType = RE::PACKAGE_OBJECT_TYPE::kWEAP;
							packageDataMap["Target Criteria"] = SQG::PackageData(RE::PackageTarget::Type::kObjectType, targetData);
							RE::BGSNamedPackageData<RE::IPackageData>::Data numData{};
							numData.i = 2;
							packageDataMap["Num to acquire"] = SQG::PackageData(numData); 
							RE::BGSNamedPackageData<RE::IPackageData>::Data allowStealingData{};
							allowStealingData.b = true;
							packageDataMap["AllowStealing"] = SQG::PackageData(allowStealingData);
							FillPackageData(customAcquirePackage, packageDataMap);

							std::list<SQG::PackageConditionDescriptor> packageConditionList;
							RE::CONDITION_ITEM_DATA::GlobalOrFloat conditionItemData{};
							conditionItemData.f = 15.f;
							packageConditionList.emplace_back(RE::FUNCTION_DATA::FunctionID::kGetStage, generatedQuest, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, false, conditionItemData, false);
							FillPackageCondition(customAcquirePackage, packageConditionList);

							const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, customAcquirePackage);
							RE::BSTSmartPointer<RE::BSScript::Object> packageCustomScriptObject;
							scriptMachine->CreateObject("PF_SQGAcquirePackage", packageCustomScriptObject);
							scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);

							instancedPackages->push_back(customAcquirePackage);
						}

						{
							//ACTIVATE PACKAGE
							//=============================

							customActivatePackage = SQG::CreatePackageFromTemplate(DPF::CreateForm(activatePackage), activatePackage, generatedQuest);

							std::unordered_map<std::string, SQG::PackageData> packageDataMap;
							RE::PackageTarget::Target targetData{};
							targetData.handle = targetActivator->CreateRefHandle();
							packageDataMap["Target"] = SQG::PackageData(RE::PackageTarget::Type::kNearReference, targetData);
							FillPackageData(customActivatePackage, packageDataMap);

							std::list<SQG::PackageConditionDescriptor> packageConditionList;
							RE::CONDITION_ITEM_DATA::GlobalOrFloat conditionItemData{};
							conditionItemData.f = 20.f;
							packageConditionList.emplace_back(RE::FUNCTION_DATA::FunctionID::kGetStage, generatedQuest, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, false, conditionItemData, false);
							FillPackageCondition(customActivatePackage, packageConditionList);

							const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, customActivatePackage);
							RE::BSTSmartPointer<RE::BSScript::Object> packageCustomScriptObject;
							scriptMachine->CreateObject("PF_SQGActivatePackage", packageCustomScriptObject);
							scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);

							instancedPackages->push_back(customActivatePackage);
						}

						{
							//TRAVEL PACKAGE
							//=============================

							customTravelPackage = SQG::CreatePackageFromTemplate(DPF::CreateForm(travelPackage), travelPackage, generatedQuest);

							std::unordered_map<std::string, SQG::PackageData> packageDataMap;
							RE::PackageLocation::Data locationData{};
							locationData.refHandle = activator->CreateRefHandle();
							packageDataMap["Place to Travel"] = SQG::PackageData(RE::PackageLocation::Type::kNearReference, locationData, 0);
							FillPackageData(customTravelPackage, packageDataMap);

							std::list<SQG::PackageConditionDescriptor> packageConditionList;
							RE::CONDITION_ITEM_DATA::GlobalOrFloat conditionItemData{};
							conditionItemData.f = 30.f;
							packageConditionList.emplace_back(RE::FUNCTION_DATA::FunctionID::kGetStage, generatedQuest, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, false, conditionItemData, false);
							FillPackageCondition(customTravelPackage, packageConditionList);

							const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, customTravelPackage);
							RE::BSTSmartPointer<RE::BSScript::Object> packageCustomScriptObject;
							scriptMachine->CreateObject("PF_SQGTravelPackage", packageCustomScriptObject);
							scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);

							instancedPackages->push_back(customTravelPackage);
						}
					}
				}
				aliasInstancesList->lock.UnlockForRead();
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}
};

class PackageEventSink : public RE::BSTEventSink<RE::TESPackageEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESPackageEvent* a_event, RE::BSTEventSource<RE::TESPackageEvent>* a_eventSource) override
	{
		if(customAcquirePackage && a_event->packageFormId == customAcquirePackage->formID)
		{
			if(a_event->packageEventType == RE::TESPackageEvent::PackageEventType::kEnd)
			{
				auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				auto* policy = scriptMachine->GetObjectHandlePolicy();

				const auto customAcquirePackageHandle = policy->GetHandleForObject(RE::FormType::Package, customAcquirePackage);
				RE::BSTSmartPointer<RE::BSScript::Object> customAcquirePackageScriptObject;
				scriptMachine->FindBoundObject(customAcquirePackageHandle, "PF_SQGAcquirePackage", customAcquirePackageScriptObject);
				const auto* methodInfo = customAcquirePackageScriptObject->type->GetMemberFuncIter();
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
				scriptMachine->DispatchMethodCall(customAcquirePackageHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
			}
		}
		else if(customActivatePackage && a_event->packageFormId == customActivatePackage->formID && a_event->packageEventType == RE::TESPackageEvent::PackageEventType::kEnd)
		{
			auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* policy = scriptMachine->GetObjectHandlePolicy();

			const auto customActivatePackageHandle = policy->GetHandleForObject(RE::FormType::Package, customActivatePackage);
			RE::BSTSmartPointer<RE::BSScript::Object> customActivatePackageScriptObject;
			scriptMachine->FindBoundObject(customActivatePackageHandle, "PF_SQGActivatePackage", customActivatePackageScriptObject);
			const auto* methodInfo = customActivatePackageScriptObject->type->GetMemberFuncIter();
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(customActivatePackageHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}
		else if(customTravelPackage && a_event->packageFormId == customTravelPackage->formID)
		{
			auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* policy = scriptMachine->GetObjectHandlePolicy();

			const auto customTravelPackageHandle = policy->GetHandleForObject(RE::FormType::Package, customTravelPackage);
			RE::BSTSmartPointer<RE::BSScript::Object> customTravelPackageScriptObject;
			scriptMachine->FindBoundObject(customTravelPackageHandle, "PF_SQGTravelPackage", customTravelPackageScriptObject);
			const auto* methodInfo = a_event->packageEventType == RE::TESPackageEvent::PackageEventType::kBegin ? customTravelPackageScriptObject->type->GetMemberFuncIter() + 1 : customTravelPackageScriptObject->type->GetMemberFuncIter();
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(customTravelPackageHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}
		return RE::BSEventNotifyControl::kContinue;
	}
};

RE::TESQuest* GetSelectedQuest(RE::StaticFunctionTag*)
{
	return selectedQuest;
}

std::string SwapSelectedQuest(RE::StaticFunctionTag*)
{
	selectedQuest = selectedQuest == referenceQuest ? generatedQuest : referenceQuest;
	return "Selected " + std::string(selectedQuest ? selectedQuest->GetFullName() : "nullptr");
}

void StartSelectedQuest(RE::StaticFunctionTag*)
{
	if(selectedQuest)
	{
		selectedQuest->Start();
	}
}

void DraftDebugFunction(RE::StaticFunctionTag*)
{
	int z = 42;
}	

bool RegisterFunctions(RE::BSScript::IVirtualMachine* inScriptMachine)
{
	inScriptMachine->RegisterFunction("GenerateQuest", "SQGLib", GenerateQuest);
	inScriptMachine->RegisterFunction("SwapSelectedQuest", "SQGLib", SwapSelectedQuest);
	inScriptMachine->RegisterFunction("GetSelectedQuest", "SQGLib", GetSelectedQuest);
	inScriptMachine->RegisterFunction("StartSelectedQuest", "SQGLib", StartSelectedQuest);
	inScriptMachine->RegisterFunction("DraftDebugFunction", "SQGLib", DraftDebugFunction);
	return true;
}

// ReSharper disable once CppInconsistentNaming
extern "C" DLLEXPORT bool SKSEPlugin_Query(const SKSE::QueryInterface* inQueryInterface, SKSE::PluginInfo* outPluginInfo)
{
	if (inQueryInterface->IsEditor()) 
	{
		SKSE::log::critical("This plugin is not designed to run in Editor\n"sv);
		return false;
	}

	if (const auto runtimeVersion = inQueryInterface->RuntimeVersion(); runtimeVersion < SKSE::RUNTIME_1_5_97) 
	{
		SKSE::log::critical("Unsupported runtime version %s!\n"sv, runtimeVersion.string());
		return false;
	}

	outPluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	outPluginInfo->name = Plugin::NAME.data();
	outPluginInfo->version = Plugin::VERSION[0];

	return true;
}

void ProcessDialogEntries(RE::TESObjectREFR* inSpeaker, const std::vector<std::unique_ptr<DialogTopicData>>& inEntries)
{
	speakersData[inSpeaker->formID].hasAnyValidEntry = false;
	for(size_t i = 0; i < SUB_TOPIC_COUNT; ++i)
	{
		if(i < inEntries.size())
		{
			if(ProcessDialogEntry(inSpeaker, *inEntries[i], subTopicsInfos[i]))
			{
				speakersData[inSpeaker->formID].hasAnyValidEntry = true;
			}
		}
		else
		{
			subTopicsInfos[i]->objConditions.head = new RE::TESConditionItem();
			std::memcpy(subTopicsInfos[i]->objConditions.head, impossibleCondition, sizeof(RE::TESConditionItem));  // NOLINT(bugprone-undefined-memory-manipulation)
		}
	}
}

class TopicInfoEventSink final : public RE::BSTEventSink<RE::TESTopicInfoEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESTopicInfoEvent* a_event, RE::BSTEventSource<RE::TESTopicInfoEvent>* a_eventSource) override
	{
		if(a_event->type == RE::TESTopicInfoEvent::TopicInfoEventType::kTopicEnd && dialogTopicsData.contains(a_event->speakerRef->formID))
		{
			if(!speakersData.contains(a_event->speakerRef->formID) || !speakersData[a_event->speakerRef->formID].hasAnyValidEntry)
			{
				ProcessDialogEntries(a_event->speakerRef.get(), dialogTopicsData[targetForm->formID].childEntries);

				if(!speakersData[a_event->speakerRef->formID].hasAnyValidEntry)
				{
					return RE::BSEventNotifyControl::kContinue;
				}
			}

			if(const auto topicInfoBinding = speakersData[a_event->speakerRef->formID].topicsInfosBindings.find(a_event->topicInfoFormID); topicInfoBinding != speakersData[a_event->speakerRef->formID].topicsInfosBindings.end())
			{
				DialogTopicData::AnswerData* answer = topicInfoBinding->second;
				
				answer->alreadySaid = true;
				speakersData[a_event->speakerRef->formID].lastSelectedAnswer = answer; //Data will be read by hook

				if(answer->targetStage != -1) //Done specific logic for target stage because asynchronous nature of script would prevent the topic list to be correctly updated. It is however still required to set the stage with fragment as well or QuestFragments won't be triggered otherwise.
				{
					answer->parentEntry->owningQuest->currentStage = static_cast<std::uint16_t>(answer->targetStage);
				}

				if(answer->fragmentId != -1)
				{
					auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
					auto* policy = scriptMachine->GetObjectHandlePolicy();
					const auto updatedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, answer->parentEntry->owningQuest);
					RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
					scriptMachine->FindBoundObject(updatedQuestHandle, "SQGTopicFragments", questCustomScriptObject);

					const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() + answer->fragmentId;
					RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
					scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
				}	

				//Process following entries in tree  or root dialog entries if it was a leaf
				ProcessDialogEntries(a_event->speakerRef.get(), !answer->parentEntry->childEntries.empty() ? answer->parentEntry->childEntries : dialogTopicsData[a_event->speakerRef->formID].childEntries);
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}
};

void PopulateResponseTextHook(RE::TESTopicInfo::ResponseData* generatedResponse, const RE::TESTopicInfo* parentTopicInfo)
{
	const auto* topicManager = RE::MenuTopicManager::GetSingleton();
	if(topicManager && topicManager->dialogueList)
	{
		RE::TESObjectREFR* speaker = nullptr;
		if(auto* currentSpeaker = topicManager->speaker.get().get())
		{
			speaker = currentSpeaker;
		}
		else if(auto* lastSpeaker = topicManager->lastSpeaker.get().get())
		{
			speaker = lastSpeaker;	
		}

		if(speaker)
		{
			if(const auto topicInfoBinding = speakersData[speaker->formID].topicsInfosBindings.find(parentTopicInfo->formID); topicInfoBinding != speakersData[speaker->formID].topicsInfosBindings.end())
			{
				generatedResponse->responseText = speakersData[speaker->formID].topicsInfosBindings.find(parentTopicInfo->formID)->second->answer;
			}
		}
	}
}

struct PopulateResponseTextHookedPatch final : Xbyak::CodeGenerator
{
	explicit PopulateResponseTextHookedPatch(const uintptr_t inHookAddress, const uintptr_t inPopulateResponseTextAddress, const uintptr_t inResumeAddress)
	{
		mov(r13, rcx);	//Saving RCX value at it is overwritten by PopulateResponseText

		mov(rax, inPopulateResponseTextAddress); //Calling PopulateResponseText as usual
		call(rax);

		push(rax); //Saving state
		push(rcx);
		push(rdx);

		mov(rcx, r13); //Moving data to function arguments (see: https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170)
		mov(rdx, rbp);

		mov(rax, inHookAddress); //Calling hook
		call(rax);

		pop(rax); //Restoring state
		pop(rcx);
		pop(rdx);

		mov(r15, inResumeAddress); //Resuming standard execution
		jmp(r15);
    }
};

void OnResponseSaidHook()
{
	const auto* topicManager = RE::MenuTopicManager::GetSingleton();
	if(topicManager && topicManager->dialogueList)
	{
		RE::TESObjectREFR* speaker = nullptr;
		if(auto* currentSpeaker = topicManager->speaker.get().get())
		{
			speaker = currentSpeaker;
		}
		else if(auto* lastSpeaker = topicManager->lastSpeaker.get().get())
		{
			speaker = lastSpeaker;	
		}

		if(speaker)
		{
			for(auto* dialog : *topicManager->dialogueList)
			{
				if(const auto topicInfoBinding = speakersData[speaker->formID].topicsInfosBindings.find(dialog->parentTopicInfo->formID); topicInfoBinding != speakersData[speaker->formID].topicsInfosBindings.end())
				{
					dialog->neverSaid = !topicInfoBinding->second->alreadySaid;
					dialog->parentTopicInfo->saidOnce = topicInfoBinding->second->alreadySaid;
				}
			}
		}
	}
	
}

struct OnResponseSaidHookedPatch final : Xbyak::CodeGenerator
{
    explicit OnResponseSaidHookedPatch(const uintptr_t inHookAddress, const uintptr_t inHijackedMethodAddress, const uintptr_t inResumeAddress)
    {
		mov(r14, inHijackedMethodAddress); //Calling the method we hijacked (didn't bother to check what it does, it's just at a convenient place for our hook)
		call(r14);

		mov(r14, inHookAddress);	//Calling hook
		call(r14);

		mov(r14, inResumeAddress); //Resuming standard execution
		jmp(r14);
    }
};

bool FillLogEntryHook(const RE::TESQuestStageItem* inQuestStageItem)
{
	const auto isGeneratedQuest = inQuestStageItem->owner == generatedQuest;
	if(const auto questData = questsData.find(inQuestStageItem->owner->formID); questData != questsData.end())
	{
		if(const auto logEntry = questData->second.logEntries.find(inQuestStageItem->owner->currentStage); logEntry != questData->second.logEntries.end())
		{
			targetLogEntry = logEntry->second.data();
			lastValidLogEntryIndexes[inQuestStageItem->owner->formID] = inQuestStageItem->owner->currentStage;
		}
	}
	else
	{
		targetLogEntry = questsData[inQuestStageItem->owner->formID].logEntries[lastValidLogEntryIndexes[inQuestStageItem->owner->formID]].data();
	}

	return isGeneratedQuest;
}

struct FillLogEntryHookedPatch final : Xbyak::CodeGenerator
{
    explicit FillLogEntryHookedPatch(const uintptr_t inHookAddress, const uintptr_t inHijackedMethodAddress, const uintptr_t inResumeStandardExecutionAddress, const uintptr_t inBypassAddress, const uintptr_t inStringAdr)
    {
	    // ReSharper disable once CppInconsistentNaming
	    Xbyak::Label BYPASS_STRING_LOAD;

		mov(rax, inHijackedMethodAddress); //Calling the method we hijacked (that is setting in RCX TESQuestStageItem*)
		call(rax);

		push(rax);
		mov(rax, inHookAddress);	//Calling hook
		call(rax);
		test(al, al); //If the quest for which we're trying to fill the log entry is in one of ours
		jnz(BYPASS_STRING_LOAD); //Then bypass the classic log entry loading process
		//Else
		pop(rax);
		mov(r15, inResumeStandardExecutionAddress); //Resume standard execution
		jmp(r15);

		L(BYPASS_STRING_LOAD);
		pop(rax);	//Removing unecessary data from the stack
    	mov(rax, ptr[inStringAdr]); //Manually move our string
    	mov(r15, inBypassAddress); //Resume execution after the normal string loading process we bypassed
    	jmp(r15);
    }
};

std::unique_ptr<QuestStageEventSink> questStageEventSink;
std::unique_ptr<QuestInitEventSink> questInitEventSink;
std::unique_ptr<PackageEventSink> packageEventSink;
std::unique_ptr<TopicInfoEventSink> topicInfoEventSink;

// ReSharper disable once CppInconsistentNaming
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* inLoadInterface)
{
#ifndef NDEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	InitializeLog();
	SKSE::log::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());

	Init(inLoadInterface);

	if (const auto papyrusInterface = SKSE::GetPapyrusInterface(); !papyrusInterface || !papyrusInterface->Register(RegisterFunctions)) 
	{
		return false;
	}

	if(const auto messaging = SKSE::GetMessagingInterface(); !messaging 
		|| !messaging->RegisterListener([](SKSE::MessagingInterface::Message* message)
		{
			if (auto* dataHandler = RE::TESDataHandler::GetSingleton(); message->type == SKSE::MessagingInterface::kDataLoaded)
			{

				selectedQuest = referenceQuest = reinterpret_cast<RE::TESQuest*>(dataHandler->LookupForm(RE::FormID{0x003371}, "SQGLib.esp"));
				targetForm = reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x00439A}, "SQGLib.esp"));
				activator =  reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x001885}, "SQGLib.esp"));  
				targetActivator = reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x008438}, "SQGLib.esp"));
				subTopicsInfos[0] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{0x00BF96}, "SQGLib.esp"));
				subTopicsInfos[1] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{0x00BF99}, "SQGLib.esp"));
				subTopicsInfos[2] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{0x00BF9C}, "SQGLib.esp"));
				subTopicsInfos[3] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{0x00BF9F}, "SQGLib.esp"));
				activatePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0019B2D});
				acquirePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0019713});
				travelPackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0016FAA});

				impossibleCondition = new RE::TESConditionItem();
				impossibleCondition->data.dataID = std::numeric_limits<std::uint32_t>::max();
				impossibleCondition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
				impossibleCondition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kIsXBox);
				impossibleCondition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
				impossibleCondition->data.comparisonValue.f = 1.f;
				impossibleCondition->next = nullptr;

				questStageEventSink = std::make_unique<QuestStageEventSink>();
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(questStageEventSink.get());

				questInitEventSink = std::make_unique<QuestInitEventSink>();
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(questInitEventSink.get());

				packageEventSink = std::make_unique<PackageEventSink>();
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(packageEventSink.get());

				topicInfoEventSink = std::make_unique<TopicInfoEventSink>();
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(topicInfoEventSink.get());

				DPF::Init(0x800, "SQGLib.esp");
			}
			else if(message->type == SKSE::MessagingInterface::kPreLoadGame)
			{
				std::string name = static_cast<char*>(message->data);
				name = name.substr(0, name.size() - 3).append("sqg");
				DPF::LoadCache(name);
			}
			else if(message->type == SKSE::MessagingInterface::kSaveGame)
			{
				std::string name = static_cast<char*>(message->data);
				name = name.append(".sqg");
				DPF::SaveCache(name);
			}
			else if(message->type == SKSE::MessagingInterface::kDeleteGame)
			{
				std::string name = static_cast<char*>(message->data);
				name = name.append(".sqg");
				DPF::DeleteCache(name);
			}
		})
	)
	{
	    return false;
	}

	SKSE::AllocTrampoline(1<<10);
	auto& trampoline = SKSE::GetTrampoline();

	const auto hookAddress = REL::Offset(0x393A32).address();
	PopulateResponseTextHookedPatch tsh{ reinterpret_cast<uintptr_t>(PopulateResponseTextHook), REL::ID(24985).address(), hookAddress + 0x5 };
    trampoline.write_branch<5>(hookAddress, trampoline.allocate(tsh));

	const auto onResponseSaidHook = REL::Offset(0x5E869D).address();
	OnResponseSaidHookedPatch dsh{ reinterpret_cast<uintptr_t>(OnResponseSaidHook), REL::Offset(0x64F360).address(), onResponseSaidHook + 0x7 };
	const auto onDialogueSayHooked = trampoline.allocate(dsh);
    trampoline.write_branch<5>(onResponseSaidHook, onDialogueSayHooked);

	const auto fillLogEntryHook = REL::Offset(0x378F6C).address();
	FillLogEntryHookedPatch fleh{reinterpret_cast<uintptr_t>(FillLogEntryHook), REL::Offset(0x382050).address(), fillLogEntryHook + 0x5, fillLogEntryHook + 0x1B, reinterpret_cast<uintptr_t>(&targetLogEntry)};
	const auto fillLogEntryHooked = trampoline.allocate(fleh);
    trampoline.write_branch<5>(fillLogEntryHook, fillLogEntryHooked);

	return true;
}
