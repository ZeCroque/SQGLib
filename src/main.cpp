#include <SQG/API/PackageUtils.h>
#include <SQG/API/DialogUtils.h>
#include <SQG/API/QuestUtils.h>
#include <DPF/API.h>

#include "Engine/Dialog.h"
#include "Engine/Quest.h"

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


void FillQuestWithGeneratedData(RE::TESQuest* inQuest)
{
	//Parametrize quest
	//=======================
	inQuest->SetFormEditorID("SQGTestQuest");
	inQuest->fullName = "00_SQG_POC";

	//Add stages
	//=======================
	SQG::AddQuestStage(inQuest, 10, SQG::QuestStageType::Startup, "My boss told me to kill a man named \"Gibier\"", 0);
	SQG::AddQuestStage(inQuest, 45, SQG::QuestStageType::Shutdown, "I decided to spare Gibier.", 6);
	SQG::AddQuestStage(inQuest, 40, SQG::QuestStageType::Shutdown, "Gibier is dead.", 5);
	SQG::AddQuestStage(inQuest, 35, SQG::QuestStageType::Default, "When I told Gibier I was going to kill him he asked for a last will. I refused.", 4);
	SQG::AddQuestStage(inQuest, 32, SQG::QuestStageType::Default, "Gibier has done his last will. The time has came for him to die.", 3);
	SQG::AddQuestStage(inQuest, 30);
	SQG::AddQuestStage(inQuest, 20);
	SQG::AddQuestStage(inQuest, 15, SQG::QuestStageType::Default, "When I told Gibier I was going to kill him he asked for a last will. I let him do what he wanted but advised him to not do anything inconsiderate.", 2);
	SQG::AddQuestStage(inQuest, 12, SQG::QuestStageType::Default, "I spoke with Gibier, whom told me my boss was a liar and begged me to spare him. I need to decide what to do.", 1);

	//Add objectives
	//=======================
	SQG::AddObjective(inQuest, 10, "Kill Gibier", {0});
	SQG::AddObjective(inQuest, 12, "(Optional) Spare Gibier");
	SQG::AddObjective(inQuest, 15, "(Optional) Let Gibier do his last will");

	//Add aliases
	//=======================
	SQG::AddRefAlias(inQuest, 0, "SQGTestAliasTarget", targetForm);

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
	
	auto* wonderEntry = SQG::AddDialogTopic(inQuest, targetForm, "I've not decided yet. I'd like to hear your side of the story.");
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
	auto* moreWonderEntry = SQG::AddDialogTopic(inQuest, targetForm, "What did he do ?", wonderEntry);
	moreWonderEntry->AddAnswer
	(
		"He lied, I'm the good one in the story.",
		"",
		{},
		12,
		0
	);

	auto* attackEntry = SQG::AddDialogTopic(inQuest, targetForm, "As you guessed. Prepare to die !");
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
	auto* lastWillYesEntry = SQG::AddDialogTopic(inQuest, targetForm, "Yes, of course, proceed but don't do anything inconsiderate.", attackEntry);
	lastWillYesEntry->AddAnswer
	(						
		"Thank you for your consideration",
		"",
		{},
		15,
		1
	);
	auto* lastWillNoEntry = SQG::AddDialogTopic(inQuest, targetForm, "No, I came here for business, not charity.", attackEntry);
	lastWillNoEntry->AddAnswer
	(
		"Your lack of dignity is a shame.",
		"",
		{},
		35,
		2
	);

	auto* spareEntry = SQG::AddDialogTopic(inQuest, targetForm, "Actually, I decided to spare you.");
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
	SQG::questsData[inQuest->formID].script = questCustomScriptObject.get();

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

std::unique_ptr<QuestInitEventSink> questInitEventSink;
std::unique_ptr<PackageEventSink> packageEventSink;


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
				DPF::Init(0x800, "SQGLib.esp");

				selectedQuest = referenceQuest = reinterpret_cast<RE::TESQuest*>(dataHandler->LookupForm(RE::FormID{0x003371}, "SQGLib.esp"));
				targetForm = reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x00439A}, "SQGLib.esp"));
				activator =  reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x001885}, "SQGLib.esp"));  
				targetActivator = reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x008438}, "SQGLib.esp"));

				activatePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0019B2D});
				acquirePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0019713});
				travelPackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0016FAA});

				SQG::DialogEngine::LoadData(dataHandler);

				questInitEventSink = std::make_unique<QuestInitEventSink>();
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(questInitEventSink.get());

				packageEventSink = std::make_unique<PackageEventSink>();
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(packageEventSink.get());

				SQG::QuestEngine::RegisterSinks();
				SQG::DialogEngine::RegisterSinks();
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

	SQG::QuestEngine::RegisterHooks(trampoline);
	SQG::DialogEngine::RegisterHooks(trampoline);

	return true;
}
