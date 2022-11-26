#include "PCH.h"
#include "PackageUtils.h"

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

constexpr int SUB_TOPIC_COUNT = 4;

RE::TESQuest* referenceQuest = nullptr;
RE::TESQuest* generatedQuest = nullptr;
RE::TESQuest* selectedQuest = nullptr;
RE::TESObjectREFR* targetForm = nullptr;
RE::TESObjectREFR* activator = nullptr;
RE::TESObjectREFR* targetActivator;
RE::TESPackage* acquirePackage;
RE::TESPackage* activatePackage;
RE::TESPackage* travelPackage = nullptr;
RE::TESPackage* customAcquirePackage = nullptr;
RE::TESPackage* customActivatePackage = nullptr;
RE::TESPackage* customTravelPackage = nullptr;
RE::TESTopic* genericHelloTopic = nullptr;
RE::TESTopicInfo* helloTopicInfo = nullptr;
RE::TESTopicInfo* subTopicsInfos[SUB_TOPIC_COUNT] { nullptr };

RE::TESConditionItem* impossibleCondition = nullptr;
RE::TESConditionItem* checkSpeakerCondition = nullptr;
RE::TESConditionItem* underStage12Condition = nullptr;
RE::TESConditionItem* aboveStage12Condition = nullptr;
RE::TESConditionItem* underStage15Condition = nullptr;

struct AnswerData
{
	class DialogEntry& parentEntry; 
	RE::BSFixedString topicInfoText;
	RE::BSFixedString topicOverrideText;
	std::vector<RE::TESConditionItem*> conditions;
	bool alreadySaid { false };
	//TODO  add script fragment
};

class DialogEntry
{
public:
	explicit DialogEntry(const RE::BSString& inTopicText = "") : topicText(inTopicText)
	{
	}

	void AddAnswer(const RE::BSString& inTopicInfoText, const RE::BSString& inTopicOverrideText, const std::vector<RE::TESConditionItem*>& inConditions)
	{
		answers.push_back(new AnswerData(*this, inTopicInfoText, inTopicOverrideText, inConditions, false));
	}

	DialogEntry* AddChildEntry(const RE::BSString& inTopicText)
	{
		auto* result = new DialogEntry(inTopicText);
		childEntries.push_back(result);
		return result;
	}

//private: TODO encapsulation
	RE::BSFixedString topicText;	// in CK : Topic text max 80/TopicInfo text max 150 -> TODO test if hardcap
	std::vector<AnswerData*> answers;
	std::vector<DialogEntry*> childEntries;
};

DialogEntry* dialogRoot = nullptr;	//TODO serialize on save
std::unordered_map<RE::FormID, AnswerData*> topicsInfosBindings;
AnswerData* lastSelectedAnswer = nullptr;

void ProcessDialogEntry(RE::TESObjectREFR* inSpeaker, const DialogEntry& inDialogEntry, RE::TESTopicInfo* inOutTopicInfo)
{
	bool hasAnyValidEntry = false;
	for(auto& answerData : inDialogEntry.answers)
	{
		RE::TESCondition condition;
		RE::TESConditionItem** conditionItemHolder = &condition.head;
		for(const auto* conditionItem : answerData->conditions)
		{
			*conditionItemHolder = new RE::TESConditionItem();
			(*conditionItemHolder)->data = conditionItem->data;
			conditionItemHolder = &((*conditionItemHolder)->next);
		}
		*conditionItemHolder = nullptr;


		if(condition(inSpeaker, inSpeaker))
		{
			inOutTopicInfo->objConditions.head = nullptr;
			topicsInfosBindings[inOutTopicInfo->formID] = answerData;
			inOutTopicInfo->parentTopic->fullName = answerData->topicOverrideText.empty() ? inDialogEntry.topicText : answerData->topicOverrideText;
			hasAnyValidEntry = true;
			break;
		}
	}

	if(!hasAnyValidEntry)
	{
		inOutTopicInfo->objConditions.head = impossibleCondition;
	}
}

std::string GenerateQuest(RE::StaticFunctionTag*)
{
	//Create quest if needed
	//=======================
	if(generatedQuest)
	{
		return "Quest yet generated.";
	}
	auto* questFormFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESQuest>();
	selectedQuest = generatedQuest = questFormFactory->Create();

	//Parametrize quest
	//=======================
	generatedQuest->SetFormEditorID("SQGTestQuest");
	generatedQuest->fullName = "00_SQG_POC";
	generatedQuest->InitItem(); //Initializes formFlags
	generatedQuest->formFlags |= RE::TESForm::RecordFlags::kFormRetainsID | RE::TESForm::RecordFlags::kPersistent;
	generatedQuest->AddChange(RE::TESForm::ChangeFlags::kCreated); //Seems to save the whole quest data and hence supersede the others (except for stages and objective for some reason)
	generatedQuest->data.flags.set(RE::QuestFlag::kRunOnce);

	//Add stages
	//=======================
	auto* logEntries = new RE::TESQuestStageItem[4];
	std::memset(logEntries, 0, 4 * sizeof(RE::TESQuestStageItem));  // NOLINT(bugprone-undefined-memory-manipulation)

	generatedQuest->initialStage = new RE::TESQuestStage();
	std::memset(generatedQuest->initialStage, 0, sizeof(RE::TESQuestStage));
	generatedQuest->initialStage->data.index = 10;
	generatedQuest->initialStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kStartUpStage);
	generatedQuest->initialStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kKeepInstanceDataFromHereOn);
	generatedQuest->initialStage->questStageItem = logEntries + 3;
	generatedQuest->initialStage->questStageItem->owner = generatedQuest;
	generatedQuest->initialStage->questStageItem->owningStage = generatedQuest->initialStage;
	generatedQuest->initialStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff}; //TODO create real quest log entry

	generatedQuest->otherStages = new RE::BSSimpleList<RE::TESQuestStage*>();

	auto* questStage = new RE::TESQuestStage();
	questStage->data.index = 40;
	questStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kShutDownStage);
	questStage->questStageItem = logEntries + 2;
	questStage->questStageItem->owner = generatedQuest;
	questStage->questStageItem->owningStage = questStage;
	questStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff};
	questStage->questStageItem->data = 1; //Means "Last stage"
	generatedQuest->otherStages->emplace_front(questStage);

	questStage = new RE::TESQuestStage();
	questStage->data.index = 30;
	questStage->questStageItem = logEntries + 1;
	questStage->questStageItem->owner = generatedQuest;
	questStage->questStageItem->owningStage = questStage;
	questStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff};
	generatedQuest->otherStages->emplace_front(questStage);

	questStage = new RE::TESQuestStage();
	questStage->data.index = 20;
	questStage->questStageItem = logEntries;
	questStage->questStageItem->owner = generatedQuest;
	questStage->questStageItem->owningStage = questStage;
	questStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff};
	generatedQuest->otherStages->emplace_front(questStage);

	questStage = new RE::TESQuestStage();
	questStage->data.index = 12;
	generatedQuest->otherStages->emplace_front(questStage);

	//Add objectives
	//=======================
	auto* questObjective = new RE::BGSQuestObjective();
	questObjective->index = 10;
	questObjective->displayText = "First objective";
	questObjective->ownerQuest = generatedQuest;
	questObjective->initialized = true; //Seems to be unused and never set by the game. Setting it in case because it is on data from CK.
	generatedQuest->objectives.push_front(questObjective);

	//Add aliases
	//=======================
	auto* rawCreatedAlias = new char[sizeof(RE::BGSRefAlias)];
	std::memcpy(rawCreatedAlias, referenceQuest->aliases[0], sizeof(RE::BGSRefAlias));  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess) //TODO relocate vtable and copy it from here instead of using reference quest
	auto* createdAlias = reinterpret_cast<RE::BGSRefAlias*>(rawCreatedAlias); 
	createdAlias->aliasID = 0;
	createdAlias->aliasName = "SQGTestAliasTarget";
	createdAlias->fillType = RE::BGSBaseAlias::FILL_TYPE::kForced;
	createdAlias->owningQuest = generatedQuest;
	createdAlias->fillData.forced = RE::BGSRefAlias::ForcedFillData{targetForm->CreateRefHandle()};
	generatedQuest->aliasAccessLock.LockForWrite();
	generatedQuest->aliases.push_back(createdAlias);
	generatedQuest->aliasAccessLock.UnlockForWrite();

	//Add target //TODO Investigate further this part as it is very likely bugged and related to the memory corruption and/or crashes
	//=======================
	const auto target = new RE::TESQuestTarget[3](); 
	target->alias = 0; 
	auto* firstObjective = *generatedQuest->objectives.begin();
	firstObjective->targets = new RE::TESQuestTarget*;
	std::memset(&target[1], 0, 2 * sizeof(RE::TESQuestTarget*));  // NOLINT(bugprone-undefined-memory-manipulation)
	*firstObjective->targets = target;
	firstObjective->numTargets = 1;

	//Sets some additional variables (pad24, pad22c and questObjective's pad04 among others)
	//=======================
	generatedQuest->InitializeData();

	//Add script logic
	//===========================
	auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	auto* policy = scriptMachine->GetObjectHandlePolicy();

	const auto selectedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, generatedQuest);
	//TODO!! try to call script compiler from c++ before loading custom script
	RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
	scriptMachine->CreateObjectWithProperties("SQGDebug", 1, questCustomScriptObject); //TODO defensive pattern against return value;
	scriptMachine->BindObject(questCustomScriptObject, selectedQuestHandle, false);
	policy->PersistHandle(selectedQuestHandle); //TODO check if useful

	RE::BSTSmartPointer<RE::BSScript::Object> referenceAliasBaseScriptObject;
	scriptMachine->CreateObject("SQGQuestTargetScript", referenceAliasBaseScriptObject);
	const auto questAliasHandle = selectedQuestHandle & 0x0000FFFFFFFF;
	scriptMachine->BindObject(referenceAliasBaseScriptObject, questAliasHandle, false);
	policy->PersistHandle(questAliasHandle); //TODO check if useful

	RE::BSScript::Variable propertyValue;
	propertyValue.SetObject(referenceAliasBaseScriptObject);
	scriptMachine->SetPropertyValue(questCustomScriptObject, "SQGTestAliasTarget", propertyValue);
	questCustomScriptObject->initialized = true; //Required when creating object with properties

	//Add dialogs
	//===========================
	if(!impossibleCondition)
	{
		RE::CONDITION_ITEM_DATA::GlobalOrFloat conditionItemData{};
		conditionItemData.f = 1.f;

		impossibleCondition = new RE::TESConditionItem();
		impossibleCondition->data.dataID = std::numeric_limits<std::uint32_t>::max();
		impossibleCondition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
		impossibleCondition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kIsXBox);
		impossibleCondition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
		impossibleCondition->data.comparisonValue = conditionItemData;
		impossibleCondition->next = nullptr;

		checkSpeakerCondition = new RE::TESConditionItem();
		checkSpeakerCondition->data.dataID = std::numeric_limits<std::uint32_t>::max();
		checkSpeakerCondition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
		checkSpeakerCondition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kGetIsID);
		checkSpeakerCondition->data.functionData.params[0] = targetForm->data.objectReference;
		checkSpeakerCondition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
		checkSpeakerCondition->data.comparisonValue = conditionItemData;
		checkSpeakerCondition->next = nullptr;

		conditionItemData.f = 12.f;
		underStage12Condition = new RE::TESConditionItem();
		underStage12Condition->data.dataID = std::numeric_limits<std::uint32_t>::max();
		underStage12Condition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
		underStage12Condition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kGetStage);
		underStage12Condition->data.functionData.params[0] = generatedQuest;
		underStage12Condition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kLessThan;
		underStage12Condition->data.comparisonValue = conditionItemData;
		underStage12Condition->next = nullptr;

		aboveStage12Condition = new RE::TESConditionItem();
		aboveStage12Condition->data.dataID = std::numeric_limits<std::uint32_t>::max();
		aboveStage12Condition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
		aboveStage12Condition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kGetStage);
		aboveStage12Condition->data.functionData.params[0] = generatedQuest;
		aboveStage12Condition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kGreaterThanOrEqualTo;
		aboveStage12Condition->data.comparisonValue = conditionItemData;
		aboveStage12Condition->next = nullptr;

		conditionItemData.f = 15.f;
		underStage15Condition = new RE::TESConditionItem();
		underStage15Condition->data.dataID = std::numeric_limits<std::uint32_t>::max();
		underStage15Condition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
		underStage15Condition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kGetStage);
		underStage15Condition->data.functionData.params[0] = generatedQuest;
		underStage15Condition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kLessThan;
		underStage15Condition->data.comparisonValue = conditionItemData;
		underStage15Condition->next = nullptr;
	}
	
	dialogRoot = new DialogEntry();
	dialogRoot->AddAnswer("So you came here to kill me, right ?", "", {checkSpeakerCondition});
	
	auto* wonderEntry = dialogRoot->AddChildEntry("I've not decided yet. I'd like to hear your side of the story.");
;	wonderEntry->AddAnswer
	(
		"Thank you very much, you'll see that I don't diserve this cruelty. Your boss is a liar.",
		"",
		{checkSpeakerCondition, underStage12Condition}
	);
	wonderEntry->AddAnswer
	(
		"Your boss is a liar.",
		"Tell me again about the reasons of the contract",
		{checkSpeakerCondition, underStage15Condition}
	);
	auto* moreWonderEntry = wonderEntry->AddChildEntry("What did he do ?");
	moreWonderEntry->AddAnswer
	(
		"He lied, I'm the good one in the story.",
		"",
		{}
	);

	auto* attackEntry = dialogRoot->AddChildEntry("As you guessed. Prepare to die !");
	attackEntry->AddAnswer
	(
		"Wait a minute ! Could I have a last will please ?",
		"",
		{checkSpeakerCondition, underStage12Condition}
	);
	attackEntry->AddAnswer
	(
		"Wait a minute ! Could I have a last will please ?",
		"I can't believe my boss would lie. Prepare to die !",
		{checkSpeakerCondition, underStage15Condition}
	);
	auto* lastWillYesEntry = attackEntry->AddChildEntry("Yes, of course, proceed but don't do anything inconsiderate.");
	lastWillYesEntry->AddAnswer
	(						
		"Thank you for your consideration",
		"",
		{}
	);
	auto lastWillNoEntry = attackEntry->AddChildEntry("No, I came here for business, not charity.");
	lastWillNoEntry->AddAnswer
	(
		"Your lack of dignity is a shame.",
		"",
		{}
	);

	auto* spareEntry = dialogRoot->AddChildEntry("Actually, I decided to spare you.");
	spareEntry->AddAnswer
	(
		"You're the kindest. I will make sure to hide myself from the eyes of your organization.",
		"",
		{checkSpeakerCondition, aboveStage12Condition, underStage15Condition}
	);

	ProcessDialogEntry(targetForm, *dialogRoot, helloTopicInfo);

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

		if(a_event->targetStage == 10)
		{
			const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter();
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}
		else if(a_event->targetStage == 40)
		{
			const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() + 1;
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
		if(const auto* updatedQuest = RE::TESForm::LookupByID<RE::TESQuest>(a_event->formID); updatedQuest == generatedQuest) //TODO find a proper way to bypass unwanted events
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

							customAcquirePackage = SQG::CreatePackageFromTemplate(acquirePackage, generatedQuest);

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
							conditionItemData.f = 10.f;
							packageConditionList.emplace_back(RE::FUNCTION_DATA::FunctionID::kGetStage, generatedQuest, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, false, conditionItemData, false);
							FillPackageCondition(customAcquirePackage, packageConditionList);

							const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, customAcquirePackage);
							RE::BSTSmartPointer<RE::BSScript::Object> packageCustomScriptObject;
							scriptMachine->CreateObject("PF_SQGAcquirePackage", packageCustomScriptObject); //TODO defensive pattern against return value;
							scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);
							policy->PersistHandle(packageHandle); //TODO check if useful

							instancedPackages->push_back(customAcquirePackage);
						}

						{
							//ACTIVATE PACKAGE
							//=============================

							customActivatePackage = SQG::CreatePackageFromTemplate(activatePackage, generatedQuest);

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
							scriptMachine->CreateObject("PF_SQGActivatePackage", packageCustomScriptObject); //TODO defensive pattern against return value;
							scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);
							policy->PersistHandle(packageHandle); //TODO check if useful

							instancedPackages->push_back(customActivatePackage);
						}

						{
							//TRAVEL PACKAGE
							//=============================

							customTravelPackage = SQG::CreatePackageFromTemplate(travelPackage, generatedQuest);

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
							scriptMachine->CreateObject("PF_SQGTravelPackage", packageCustomScriptObject); //TODO defensive pattern against return value;
							scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);
							policy->PersistHandle(packageHandle); //TODO check if useful

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

void StartQuest(RE::TESQuest* inQuest)
{
	inQuest->Start();
	auto* storyTeller = RE::BGSStoryTeller::GetSingleton();
	storyTeller->BeginStartUpQuest(inQuest);
}

void StartSelectedQuest(RE::StaticFunctionTag*)
{
	if(selectedQuest)
	{
		StartQuest(selectedQuest);
	}
}

void DraftDebugFunction(RE::StaticFunctionTag*)
{
	//TODO!! debug nvidia exception on close
	generatedQuest->currentStage = 12;
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

namespace RE
{
	struct TESTopicInfoEvent
	{
		enum class TopicInfoEventType
		{
			kBegin = 0,
			kEnd = 1,
		};
		std::uint64_t		unk00;			// 00
		RE::TESObjectREFR*	speaker;		// 08
		RE::FormID			topicInfoId;	// 10
		TopicInfoEventType	eventType;		// 14
	};
}

class TopicInfoEventSink final : public RE::BSTEventSink<RE::TESTopicInfoEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESTopicInfoEvent* a_event, RE::BSTEventSource<RE::TESTopicInfoEvent>* a_eventSource) override
	{
		if(a_event->speaker == targetForm)
		{
			AnswerData* answer = nullptr;
			if(const auto topicInfoBinding = topicsInfosBindings.find(a_event->topicInfoId); a_event->eventType == RE::TESTopicInfoEvent::TopicInfoEventType::kEnd &&  topicInfoBinding != topicsInfosBindings.end())
			{
				answer = topicInfoBinding->second;
			}
			else if(a_event->eventType == RE::TESTopicInfoEvent::TopicInfoEventType::kBegin)
			{
				if(a_event->topicInfoId == helloTopicInfo->formID)
				{
					answer = dialogRoot->answers[0];
				}
				else
				{
					if(const auto* topicInfo = reinterpret_cast<RE::TESTopicInfo*>(RE::TESForm::LookupByID(a_event->topicInfoId)); topicInfo->parentTopic == genericHelloTopic)
					{
						answer = !lastSelectedAnswer || lastSelectedAnswer->parentEntry.childEntries.empty() ? dialogRoot->answers[0] : lastSelectedAnswer;
					}
				}
			}

			if(answer)
			{
				//TODO script fragment
				answer->alreadySaid = true;
				lastSelectedAnswer = answer;

				const auto& entries = !answer->parentEntry.childEntries.empty() ? answer->parentEntry.childEntries : dialogRoot->childEntries;
				for(size_t i = 0; i < SUB_TOPIC_COUNT; ++i)
				{
					if(i < entries.size())
					{
						ProcessDialogEntry(a_event->speaker, *entries[i], subTopicsInfos[i]);
					}
					else
					{
						subTopicsInfos[i]->objConditions.head = impossibleCondition;
					}
				}
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}
};

void PopulateResponseTextHook(RE::TESTopicInfo::ResponseData* generatedResponse, const RE::TESTopicInfo* parentTopicInfo)
{
	if(const auto topicInfoBinding = topicsInfosBindings.find(parentTopicInfo->formID); topicInfoBinding != topicsInfosBindings.end())
	{
		generatedResponse->responseText = topicInfoBinding->second->topicInfoText;
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
	if(const auto* topicManager = RE::MenuTopicManager::GetSingleton(); topicManager->dialogueList)
	{
		for(auto* dialog : *topicManager->dialogueList)
		{
			if(const auto topicInfoBinding = topicsInfosBindings.find(dialog->parentTopicInfo->formID); topicInfoBinding != topicsInfosBindings.end())
			{
				dialog->neverSaid = !topicInfoBinding->second->alreadySaid;
				dialog->parentTopicInfo->saidOnce = topicInfoBinding->second->alreadySaid;
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

// ReSharper disable once CppInconsistentNaming
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* inLoadInterface)
{
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
				referenceQuest = reinterpret_cast<RE::TESQuest*>(dataHandler->LookupForm(RE::FormID{0x003371}, "SQGLib.esp"));
				targetForm = reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x00439A}, "SQGLib.esp"));
				activator =  reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x001885}, "SQGLib.esp"));  
				targetActivator = reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x008438}, "SQGLib.esp"));
				activatePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0019B2D});
				acquirePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0019713});
				travelPackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0016FAA});  //TODO parse a package template descriptor map
				genericHelloTopic = RE::TESForm::LookupByID<RE::TESTopic>(RE::FormID{0x00142B5});

				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new QuestStageEventSink());
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new QuestInitEventSink());
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new PackageEventSink());
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new TopicInfoEventSink());
			}
			else if(message->type == SKSE::MessagingInterface::kPostLoadGame)
			{
				helloTopicInfo = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{0x00C503}, "SQGLib.esp"));
				subTopicsInfos[0] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{0x00BF96}, "SQGLib.esp"));
				subTopicsInfos[1] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{0x00BF99}, "SQGLib.esp"));
				subTopicsInfos[2] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{0x00BF9C}, "SQGLib.esp"));
				subTopicsInfos[3] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{0x00BF9F}, "SQGLib.esp"));
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

	return true;
}
