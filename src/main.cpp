#include "PCH.h"

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
RE::TESQuest* editedQuest = nullptr;
RE::TESQuest* selectedQuest = nullptr;
RE::TESObjectREFR* targetForm = nullptr;
RE::TESObjectREFR* activator = nullptr;

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
	auto* waitingLogEntries = new RE::TESQuestStageItem[2];
	std::memset(waitingLogEntries, 0, 2 * sizeof(RE::TESQuestStageItem));  // NOLINT(bugprone-undefined-memory-manipulation)

	generatedQuest->initialStage = new RE::TESQuestStage();
	std::memset(generatedQuest->initialStage, 0, sizeof(RE::TESQuestStage));
	generatedQuest->initialStage->data.index = 10;
	generatedQuest->initialStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kStartUpStage);
	generatedQuest->initialStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kKeepInstanceDataFromHereOn);
	generatedQuest->initialStage->questStageItem = waitingLogEntries + 1;
	generatedQuest->initialStage->questStageItem->owner = generatedQuest;
	generatedQuest->initialStage->questStageItem->owningStage = generatedQuest->initialStage;
	generatedQuest->initialStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff}; //TODO create real quest log entry

	generatedQuest->otherStages = new RE::BSSimpleList<RE::TESQuestStage*>();
	auto* lastQuestStage = new RE::TESQuestStage();
	lastQuestStage->data.index = 20;
	lastQuestStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kShutDownStage);
	lastQuestStage->questStageItem = waitingLogEntries;
	lastQuestStage->questStageItem->owner = generatedQuest;
	lastQuestStage->questStageItem->owningStage = lastQuestStage;
	lastQuestStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff};
	lastQuestStage->questStageItem->data = 1; //Means "Last stage"
	generatedQuest->otherStages->emplace_front(lastQuestStage);

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
	createdAlias->owningQuest = selectedQuest;
	createdAlias->fillData.forced = RE::BGSRefAlias::ForcedFillData{targetForm->CreateRefHandle()};
	selectedQuest->aliasAccessLock.LockForWrite();
	selectedQuest->aliases.push_back(createdAlias);
	selectedQuest->aliasAccessLock.UnlockForWrite();

	//Add target
	//=======================
	auto* firstObjective = *selectedQuest->objectives.begin();
	firstObjective->targets = new RE::TESQuestTarget*;
	firstObjective->numTargets = 1;

	const auto rawTargetArray = new std::uintptr_t[3];
	std::memset(rawTargetArray, 0, 3 * sizeof(std::uintptr_t));
	auto* target = new RE::TESQuestTarget();
	target->alias = 0;
	rawTargetArray[0] = reinterpret_cast<std::uintptr_t>(target);
	*firstObjective->targets = reinterpret_cast<RE::TESQuestTarget*>(rawTargetArray);

	//Sets some additional variables (pad24, pad22c and questObjective's pad04 among others)
	//=======================
	generatedQuest->InitializeData(); //Initializes

	//Add script logic
	//===========================
	auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	auto* policy = scriptMachine->GetObjectHandlePolicy();

	const auto selectedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, selectedQuest);
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

	//Notify success
	//===========================
	std::ostringstream ss;
    ss << std::hex << generatedQuest->formID;
	const auto formId = ss.str();
	SKSE::log::debug("Generated quest with formID: {0}"sv, formId);
	return "Generated quest with formID: " + formId;
}

namespace RE
{
	struct TESQuestStageEvent
	{
	public:
		// members
		void* unkPtr;	// 00 //TODO check what it is (maybe QuestStageItem*?)
		FormID	formID;	// 04
		std::uint16_t  targetStage; // 08
	};
	static_assert(sizeof(TESQuestStageEvent) == 0x10);
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

		const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() +  a_event->targetStage / 10 - 1;
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
		scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);

		return RE::BSEventNotifyControl::kStop; //TODO check what happens if we return continue here
	}
};

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
	std::ostringstream ss;
	ss << std::hex << referenceQuest;
	SKSE::log::debug("base-adr:{0}"sv, ss.str());
	ss.str(std::string());
	ss << std::hex << referenceQuest->formID;
	SKSE::log::debug("formid:{0}"sv, ss.str());
	ss.str(std::string());
	ss << std::hex << &referenceQuest->aliases;
	SKSE::log::debug("aliases-adr:{0}"sv, ss.str());
	ss.str(std::string());
	ss << std::hex << *referenceQuest->aliases.begin();
	SKSE::log::debug("aliases[0]-adr:{0}"sv, ss.str());
	ss.str(std::string());
	ss << std::hex << targetForm;
	SKSE::log::debug("targetForm:{0}"sv, ss.str());
	ss.str(std::string());
	ss << std::hex << &targetForm->extraList;
	SKSE::log::debug("targetFormExtraLists:{0}"sv, ss.str());
	ss.str(std::string());
	ss << std::hex << targetForm->extraList.GetByType(RE::ExtraDataType::kAliasInstanceArray);
	SKSE::log::debug("targetFormExtraAliasInstanceArrays:{0}"sv, ss.str());
	ss.str(std::string());
	ss << std::hex << activator;
	SKSE::log::debug("Activator:{0}"sv, ss.str());

	if(const auto* aliasInstancesList = reinterpret_cast<RE::ExtraAliasInstanceArray*>(targetForm->extraList.GetByType(RE::ExtraDataType::kAliasInstanceArray)))
	{
		ss.str(std::string());
		ss << std::hex << aliasInstancesList;
		SKSE::log::debug("targetFormExtraAliasInstanceArray:{0}"sv, ss.str());

		aliasInstancesList->lock.LockForRead();
		auto* castedPackage = reinterpret_cast<RE::TESCustomPackageData*>((*aliasInstancesList->aliases[0]->instancedPackages)[0]->data);
		ss.str(std::string());
		ss << std::hex << castedPackage;
		SKSE::log::debug("customPackageData:{0}"sv, ss.str());

		for(auto i = 0; i < castedPackage->data.dataSize; ++i)
		{
			auto* packageData = castedPackage->data.data[i];
			SKSE::log::debug("package-data[{0}]-type:{1}"sv, i, packageData->GetTypeName());

			if(packageData->GetTypeName() == "Location")
			{
				auto castedPackageData = reinterpret_cast<RE::BGSPackageDataLocation*>(packageData - 1);
				auto activatorSmartPtr = castedPackageData->pointer->data.refHandle.get();
				auto* activatorLocalRef = activatorSmartPtr.get();
				int u = 54;
			}
			//TODO other types...

			RE::BSString result;
			packageData->GetDataAsString(&result);
			SKSE::log::debug("package-data[{0}]-as-string:{1}"sv, i, result);
			int z = 42;
			
		}
		aliasInstancesList->lock.UnlockForRead();
	}


	auto* packageFormFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESPackage>();
	auto* package = packageFormFactory->Create();

	delete package->data;
	auto* rawCreatedPackageData = new char[sizeof(RE::TESCustomPackageData)];
	auto& referencePackageList = *(reinterpret_cast<RE::ExtraAliasInstanceArray*>(targetForm->extraList.GetByType(RE::ExtraDataType::kAliasInstanceArray))->aliases[0]->instancedPackages);
	std::memcpy(rawCreatedPackageData, referencePackageList[0]->data, sizeof(RE::TESCustomPackageData));  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess) //TODO relocate vtable and copy it from here instead of using package of reference quest
	package->data = reinterpret_cast<RE::TESCustomPackageData*>(rawCreatedPackageData);
	package->ownerQuest = selectedQuest;

	//TODO!! debug nvidia exception on close
}

bool RegisterFunctions(RE::BSScript::IVirtualMachine* inScriptMachine)
{
	inScriptMachine->RegisterFunction("GenerateQuest", "SQGLib", GenerateQuest);
	inScriptMachine->RegisterFunction("SwapSelectedQuest", "SQGLib", SwapSelectedQuest);
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
			if (message->type == SKSE::MessagingInterface::kDataLoaded)
			{
				auto* dataHandler = RE::TESDataHandler::GetSingleton();
				selectedQuest = editedQuest = reinterpret_cast<RE::TESQuest*>(dataHandler->LookupForm(RE::FormID{0x003e36}, "SQGLib.esp"));
				referenceQuest = reinterpret_cast<RE::TESQuest*>(dataHandler->LookupForm(RE::FormID{0x003371}, "SQGLib.esp"));
				targetForm = reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x00439A}, "SQGLib.esp"));
				activator =  reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x001885}, "SQGLib.esp"));  

				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new QuestStageEventSink());
			}
			else if(message->type == SKSE::MessagingInterface::kPostLoadGame)
			{
				StartQuest(referenceQuest);
			}
		})
	)
	{
	    return false;
	}
	
	return true;
}
