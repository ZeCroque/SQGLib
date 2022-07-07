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

namespace RE
{
	struct TESQuestStartStopEvent
	{
	public:
		// members
		FormID        formID;   // 00
		bool          started;  // 04
		std::uint8_t  unk05;    // 05
		std::uint16_t pad06;    // 06
	};
	static_assert(sizeof(TESQuestStartStopEvent) == 0x8);
}

class QuestStartEventSink : public RE::BSTEventSink<RE::TESQuestStartStopEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestStartStopEvent* a_event, RE::BSTEventSource<RE::TESQuestStartStopEvent>* a_eventSource) override
	{
			auto* player = RE::PlayerCharacter::GetSingleton();
	auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* storyTeller = RE::BGSStoryTeller::GetSingleton();
		int i = 42;
		return RE::BSEventNotifyControl::kStop;
	}
};

namespace RE
{
	struct TESQuestInitEvent
	{
	public:
		// members
		FormID        formID;   // 00
		std::uint16_t unk04;    // 04
		std::uint16_t unk06;    // 06
	};
	static_assert(sizeof(TESQuestStartStopEvent) == 0x8);
}

class QuestInitEventSink : public RE::BSTEventSink<RE::TESQuestInitEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestInitEvent* a_event, RE::BSTEventSource<RE::TESQuestInitEvent>* a_eventSource) override
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* storyTeller = RE::BGSStoryTeller::GetSingleton();
		int i = 42;
		return RE::BSEventNotifyControl::kStop;
	}
};

class QuestStageEventSink : public RE::BSTEventSink<RE::TESQuestStageEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestStageEvent* a_event, RE::BSTEventSource<RE::TESQuestStageEvent>* a_eventSource) override
	{
			auto* player = RE::PlayerCharacter::GetSingleton();
	auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* storyTeller = RE::BGSStoryTeller::GetSingleton();
		int i = 42;
		return RE::BSEventNotifyControl::kStop;
	}
};

class QuestStageItemDoneEventSink : public RE::BSTEventSink<RE::TESQuestStageItemDoneEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestStageItemDoneEvent* a_event, RE::BSTEventSource<RE::TESQuestStageItemDoneEvent>* a_eventSource) override
	{
			auto* player = RE::PlayerCharacter::GetSingleton();
	auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* storyTeller = RE::BGSStoryTeller::GetSingleton();
		int i = 42;
		return RE::BSEventNotifyControl::kStop;
	}
};

std::string HelloWorld(RE::StaticFunctionTag*)
{
	auto* storyTeller = RE::BGSStoryTeller::GetSingleton();

	auto* dataHandler = RE::TESDataHandler::GetSingleton();
	auto& questFormArray = dataHandler->GetFormArray(RE::FormType::Quest);
	auto* questFormFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESQuest>();
	auto* referenceQuest = reinterpret_cast<RE::TESQuest*>(questFormArray[questFormArray.size() - 1]);

	auto* quest = questFormFactory->Create();
	quest->SetFormEditorID("SQGTestQuest");
	quest->fullName = "00_SQG_POC";
	quest->InitItem(); //Initializes formFlags
	quest->formFlags |= RE::TESForm::RecordFlags::kFormRetainsID | RE::TESForm::RecordFlags::kPersistent;
	quest->AddChange(RE::TESForm::ChangeFlags::kCreated); //Seems to save the whole quest data and hence supersede the others (except for stages and objective for some reason)

	quest->data.flags.set(RE::QuestFlag::kRunOnce);

	quest->executedStages = reinterpret_cast<RE::BSSimpleList<RE::TESQuestStage>*>(new std::int64_t[3]);
	std::memset(reinterpret_cast<void*>(quest->executedStages), 0, 3 * sizeof(std::int64_t));
	std::memcpy(reinterpret_cast<void*>(quest->executedStages), reinterpret_cast<void*>(referenceQuest->executedStages), 2 * sizeof(std::int64_t));
	//TODO!!! reinterpret cast to ListNode* and try to add new ListNode directly

	quest->waitingStages = new RE::BSSimpleList<RE::TESQuestStage*>(*referenceQuest->waitingStages);

	auto* questStage = new RE::TESQuestStage();
	questStage->data.index = 20;
	questStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kShutDownStage);
	*quest->waitingStages->begin() = questStage;

	auto* questObjective = new RE::BGSQuestObjective();
	questObjective->index = 10;
	questObjective->displayText = "First objective";
	questObjective->ownerQuest = quest;
	questObjective->initialized = true; //TODO!!! why do we have to set this manually?

	quest->objectives.push_front(questObjective); //TODO!!! add target to quest objective

	questFormArray.push_back(quest); //todo!!! remove this?

	quest->InitializeData(); //Initializes pad24, pad22c and questObjective's pad04
		
	RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new QuestStartEventSink());
	RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new QuestInitEventSink());
	RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new QuestStageEventSink());
	RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new QuestStageItemDoneEventSink());	

	quest->SetEnabled(true);
	quest->Reset();
	quest->Start();
	storyTeller->BeginStartUpQuest(quest);

	std::ostringstream ss;
    ss << std::hex << quest->formID;
	SKSE::log::debug("{0}"sv, ss.str());
	return ss.str();
}

bool RegisterFunctions(RE::BSScript::IVirtualMachine* inScriptMachine)
{
	inScriptMachine->RegisterFunction("HelloWorld", "SQGLib", HelloWorld);
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
	
	return true;
}
