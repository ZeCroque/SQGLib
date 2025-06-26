#include <DPF/API.h>

#include "Engine/Dialog.h"
#include "Engine/Package.h"
#include "Engine/Quest.h"
#include "Engine/Script.h"
#include "SQG/API/Data.h"

// # SKSE
// =======================

namespace SQG
{
	extern void Deserialize(DPF::FileReader* inSerializer);
	extern void Serialize(DPF::FileWriter* inSerializer);
}

namespace
{
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
#ifndef NDEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	InitializeLog();
	SKSE::log::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());

	Init(inLoadInterface);

	if(const auto messaging = SKSE::GetMessagingInterface(); !messaging 
		|| !messaging->RegisterListener([](SKSE::MessagingInterface::Message* message)
		{
			if(message->type == SKSE::MessagingInterface::kDataLoaded)
			{
				if(DPF::Init(0x800, "SQGLib.esp", "sqg"))
				{
					SQG::DataManager::GetSingleton()->InitData();

					SQG::Engine::Script::Init();

					SQG::Engine::Quest::RegisterSinks();
					SQG::Engine::Dialog::RegisterSinks();
					SQG::Engine::Package::RegisterSinks();
				}
				else
				{
					SKSE::stl::report_and_fail("You must enable the SQGLib.esp plugin in your mod manager.");
				}
			}
			else if(message->type == SKSE::MessagingInterface::kPreLoadGame)
			{
				auto* dataManager = SQG::DataManager::GetSingleton();
				dataManager->ClearSerializationData();

				if(const auto serializer = DPF::LoadCache(message))
				{
					SQG::Deserialize(serializer);
				}
			}
			else if(message->type == SKSE::MessagingInterface::kPostLoadGame)
			{
				auto* dataManager = SQG::DataManager::GetSingleton();
				auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				auto* policy = scriptMachine->GetObjectHandlePolicy();

				for(auto& [formId, data] : dataManager->questsData)
				{
					const auto questHandle = policy->GetHandleForObject(RE::FormType::Quest, data.quest);
					RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
					scriptMachine->FindBoundObject(questHandle, data.quest->GetFormEditorID(), questCustomScriptObject);
					dataManager->questsData[formId].script = questCustomScriptObject.get();

					SQG::Engine::Package::InitAliasPackages(formId);
				}

			}
			else if(message->type == SKSE::MessagingInterface::kSaveGame)
			{
				if (auto* serializer = DPF::SaveCache(message, true))
				{
					SQG::Serialize(serializer);
					serializer->Close();
				}
			}
			else if(message->type == SKSE::MessagingInterface::kDeleteGame)
			{
				DPF::DeleteCache(message);
			}
		})
	)
	{
	    return false;
	}

	SKSE::AllocTrampoline(200);
	auto& trampoline = SKSE::GetTrampoline();
	SQG::Engine::Quest::RegisterHooks(trampoline);
	SQG::Engine::Dialog::RegisterHooks(trampoline);

	return true;
}
