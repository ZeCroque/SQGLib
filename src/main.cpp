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
RE::TESQuest* selectedQuest = nullptr;

std::string GenerateQuest(RE::StaticFunctionTag*)
{
	if(generatedQuest)
	{
		return "Quest yet generated.";
	}

	auto* questFormFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESQuest>();

	generatedQuest = questFormFactory->Create();
	generatedQuest->SetFormEditorID("SQGTestQuest");
	generatedQuest->fullName = "00_SQG_POC";
	generatedQuest->InitItem(); //Initializes formFlags
	generatedQuest->formFlags |= RE::TESForm::RecordFlags::kFormRetainsID | RE::TESForm::RecordFlags::kPersistent;
	generatedQuest->AddChange(RE::TESForm::ChangeFlags::kCreated); //Seems to save the whole quest data and hence supersede the others (except for stages and objective for some reason)

	generatedQuest->data.flags.set(RE::QuestFlag::kRunOnce);

	auto* executedStages = new RE::BSSimpleList<RE::TESQuestStage>::Node[4];
	std::memset(executedStages, 0, 3 * sizeof(RE::BSSimpleList<RE::TESQuestStage>::Node));  // NOLINT(bugprone-undefined-memory-manipulation)
	executedStages[0].item.data.index = 10;
	executedStages[0].item.data.flags.set(RE::QUEST_STAGE_DATA::Flag::kStartUpStage);
	executedStages[0].item.data.flags.set(RE::QUEST_STAGE_DATA::Flag::kKeepInstanceDataFromHereOn);
	executedStages[0].next = &executedStages[1];
	auto* rawValue = reinterpret_cast<std::uintptr_t*>(&executedStages[2].item);
	*rawValue = 0x10000ffffffff; //Win32 Allocator
	*(rawValue + 1) = reinterpret_cast<std::uintptr_t>(generatedQuest);
	*(rawValue + 2) = reinterpret_cast<std::uintptr_t>(executedStages);
	generatedQuest->executedStages = reinterpret_cast<RE::BSSimpleList<RE::TESQuestStage>*>(executedStages); //TODO if tied to array and not queststage itself, create a wrapper structure containing the raw value in addition to the list

	auto* waitingStages = new RE::BSSimpleList<RE::TESQuestStage*>::Node();
	std::memset(waitingStages, 0, sizeof(RE::BSSimpleList<RE::TESQuestStage*>::Node));  // NOLINT(bugprone-undefined-memory-manipulation)
	waitingStages[0].item = new RE::TESQuestStage();
	waitingStages[0].item->data.index = 20;
	waitingStages[0].item->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kShutDownStage);
	generatedQuest->waitingStages = reinterpret_cast<RE::BSSimpleList<RE::TESQuestStage*>*>(waitingStages);

	auto* questObjective = new RE::BGSQuestObjective();
	questObjective->index = 10;
	questObjective->displayText = "First objective";
	questObjective->ownerQuest = generatedQuest;
	questObjective->initialized = true; //TODO do we have to set this manually?

	generatedQuest->objectives.push_front(questObjective);

	generatedQuest->InitializeData(); //Initializes pad24, pad22c and questObjective's pad04

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
		void* unkPtr;	// 00 //TODO check what it is
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
		if(updatedQuest != generatedQuest && updatedQuest != RE::TESForm::LookupByID<RE::TESQuest>(std::strtoul("5003e36", nullptr, 16))) //TODO find a proper way to bypass unwanted events
		{
			return RE::BSEventNotifyControl::kStop;
		}

		const auto updatedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, updatedQuest);
		RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
		scriptMachine->FindBoundObject(updatedQuestHandle, "SQGDebug", questCustomScriptObject);

		const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() +  a_event->targetStage / 10 - 1;
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
		scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);

		return RE::BSEventNotifyControl::kStop;
	}
};

std::string SwapSelectedQuest(RE::StaticFunctionTag*)
{
	selectedQuest = selectedQuest == referenceQuest ? generatedQuest : referenceQuest;
	return "Selected " + std::string(selectedQuest ? selectedQuest->GetFullName() : "nullptr");
}

void StartSelectedQuest(RE::StaticFunctionTag*)
{
	if(selectedQuest)
	{
		auto* storyTeller = RE::BGSStoryTeller::GetSingleton();
		storyTeller->BeginStartUpQuest(selectedQuest);
		//TODO check why quest targets aren't set
	}
}

void EmptyDebugFunction(RE::StaticFunctionTag*)
{
	auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	auto* policy = scriptMachine->GetObjectHandlePolicy();

	RE::TESQuest* editedQuest;
	if(!generatedQuest)
	{
		const RE::FormID emptyQuestFormId = std::strtoul("5003e36", nullptr, 16); //TODO getModIndex
		editedQuest = RE::TESForm::LookupByID<RE::TESQuest>(emptyQuestFormId);
	}
	else
	{
		editedQuest = generatedQuest;
	}
	RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new QuestStageEventSink());

	const RE::FormID targetFormId = std::strtoul("500439A", nullptr, 16);
	const auto targetForm = RE::TESForm::LookupByID<RE::TESObjectREFR>(targetFormId);

	auto* rawCreatedAlias = new char[0x48]; //TODO make size constexpr
	std::memcpy(rawCreatedAlias, referenceQuest->aliases[0], 0x48);  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess) //TODO relocate vtable and copy it from here instead of using reference quest
	auto* createdAlias = reinterpret_cast<RE::BGSRefAlias*>(rawCreatedAlias); 

	createdAlias->aliasID = 0;
	createdAlias->aliasName = "SQGTestAliasTarget";
	createdAlias->fillType = RE::BGSBaseAlias::FILL_TYPE::kForced;
	createdAlias->owningQuest = editedQuest;
	createdAlias->fillData.forced = RE::BGSRefAlias::ForcedFillData{targetForm->CreateRefHandle()};

	editedQuest->aliasAccessLock.LockForWrite();
	editedQuest->aliases.push_back(createdAlias);
	editedQuest->aliasAccessLock.UnlockForWrite();


	//Add packages //TODO!!
	//============================
	/*if(auto* aliasInstancesList = dynamic_cast<RE::ExtraAliasInstanceArray*>(targetForm->extraList.GetByType(RE::ExtraDataType::kAliasInstanceArray)))
	{
		auto* packageFormFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESPackage>();
		aliasInstancesList->lock.LockForWrite();
		for(auto* aliasInstanceData : aliasInstancesList->aliases)
		{
			if(aliasInstanceData->quest == editedQuest && aliasInstanceData->alias->aliasName == "SQGTestAliasTarget")
			{
				auto* package = packageFormFactory->Create();
				//TODO fill package
				const_cast<RE::BSTArray<RE::TESPackage*>*>(aliasInstanceData->instancedPackages)->emplace_back(package);
			}
		}
		aliasInstancesList->lock.UnlockForWrite();
	}*/

	//Add target
	//=======================
	const auto target = new RE::TESQuestTarget[3]();
	target->alias = 0; 
	auto* firstObjective = *editedQuest->objectives.begin();
	firstObjective->targets = new RE::TESQuestTarget*;
	std::memset(&target[1], 0, 2 * sizeof(RE::TESQuestTarget*));  // NOLINT(bugprone-undefined-memory-manipulation)
	*firstObjective->targets = target;
	firstObjective->numTargets = 1;

	//Script part
	//===========================
	const auto editedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, editedQuest);
	//TODO!!! try to call script compiler from c++ before loading custom script
	RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
	scriptMachine->CreateObjectWithProperties("SQGDebug", 1, questCustomScriptObject); //TODO defensive pattern against return value;
	scriptMachine->BindObject(questCustomScriptObject, editedQuestHandle, false);
	policy->PersistHandle(editedQuestHandle); //TODO check if useful

	RE::BSTSmartPointer<RE::BSScript::Object> referenceAliasBaseScriptObject;
	scriptMachine->CreateObject("SQGQuestTargetScript", referenceAliasBaseScriptObject);
	const auto questAliasHandle = editedQuestHandle & 0x0000FFFFFFFF;
	scriptMachine->BindObject(referenceAliasBaseScriptObject, questAliasHandle, false);
	policy->PersistHandle(questAliasHandle); //TODO check if useful

	RE::BSScript::Variable propertyValue;
	propertyValue.SetObject(referenceAliasBaseScriptObject);
	scriptMachine->SetPropertyValue(questCustomScriptObject, "SQGTestAliasTarget", propertyValue);
	questCustomScriptObject->initialized = true;

	//Execute console command because native C++ method doesn't init quest targets
	//=====================
	const auto scriptFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
	auto script = scriptFactory ? scriptFactory->Create() : nullptr;
	if (script)
	{
		const auto selectedRef = RE::Console::GetSelectedRef();
		script->SetCommand("StartQuest SQGEmptySample");
		script->CompileAndRun(selectedRef.get());
		//delete script;
	}
	//TODO!!!! debug printing this very method ptr adr to break into it in IDA debugger and find the engine startQuest function with quest target initialization, then use it to start generated quest (replace empty quest)

	//TODO!!! debug nvidia exception
}

bool RegisterFunctions(RE::BSScript::IVirtualMachine* inScriptMachine)
{
	inScriptMachine->RegisterFunction("GenerateQuest", "SQGLib", GenerateQuest);
	inScriptMachine->RegisterFunction("SwapSelectedQuest", "SQGLib", SwapSelectedQuest);
	inScriptMachine->RegisterFunction("StartSelectedQuest", "SQGLib", StartSelectedQuest);
	inScriptMachine->RegisterFunction("EmptyDebugFunction", "SQGLib", EmptyDebugFunction);
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
				auto& questFormArray = dataHandler->GetFormArray(RE::FormType::Quest);
				auto it = questFormArray.end();
				bool found = false;
				do
				{
					--it;
					auto* quest = reinterpret_cast<RE::TESQuest*>(*it);
					std::ostringstream ss;
				    ss << std::hex << quest->formID;
					if (std::regex_search(ss.str(), std::regex(".*003371")))
					{
						found = true;
						referenceQuest = quest;
					}
				} while(!found && it != questFormArray.begin());
			}
		})
	)
	{
	    return false;
	}
	
	return true;
}
