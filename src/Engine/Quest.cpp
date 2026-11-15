#include "Quest.h"

#include "SQG/API/QuestUtils.h"

namespace SQG::Engine::Quest
{
	// # Hooks
	// =======================

	// ## Log Entries
	// =======================
	namespace
	{
		std::map<RE::FormID, std::uint16_t> lastValidLogEntryIndexes;
		char* targetLogEntry = nullptr;

		// We hooked this method that provides the log entry to the UI on mouse hover in order to replace it with custom data read from the DataManager
		bool FillLogEntryHook(const RE::TESQuestStageItem* inQuestStageItem)
		{
			auto* dataManager = DataManager::GetSingleton();
			bool isHandledQuest = false;
			if(!inQuestStageItem->owner)
			{
				return false;
			}

			if(const auto questData = dataManager->questsData.find(inQuestStageItem->owner->formID); questData != dataManager->questsData.end())
			{
				isHandledQuest = true;
				if(const auto logEntry = questData->second.logEntries.find(inQuestStageItem->owner->currentStage); logEntry != questData->second.logEntries.end())
				{
					targetLogEntry = logEntry->second.data();
					lastValidLogEntryIndexes[inQuestStageItem->owner->formID] = inQuestStageItem->owner->currentStage;
				}
				else
				{
					targetLogEntry = dataManager->questsData[inQuestStageItem->owner->formID].logEntries[lastValidLogEntryIndexes[inQuestStageItem->owner->formID]].data();
				}
			}
			return isHandledQuest;
		}

		struct FillLogEntryHookedPatch final : Xbyak::CodeGenerator
		{
			explicit FillLogEntryHookedPatch(const uintptr_t inHookAddress, const uintptr_t inHijackedMethodAddress, const uintptr_t inResumeStandardExecutionAddress, const uintptr_t inBypassAddress, const uintptr_t inStringAdr)
			{
				// ReSharper disable once CppInconsistentNaming
				Xbyak::Label BYPASS_STRING_LOAD;
				// ReSharper disable once CppInconsistentNaming
				Xbyak::Label RESUME_STANDARD_EXECUTION;

				mov(rax, inHijackedMethodAddress); //Calling the method we hijacked (that returns the TESQuestStageItem* in RAX, null if the stage has none)
				call(rax);

				test(rax, rax); //The game tests RAX right after, so let it deal with the missing item itself
				jz(RESUME_STANDARD_EXECUTION);

				// Keep the item while calling the hook (RAX will hold its result). The stack stays 16-byte aligned and keeps the 0x20 shadow space
				sub(rsp, 0x30);
				mov(ptr[rsp + 0x20], rax);
				mov(rcx, rax);
				mov(rax, inHookAddress);	//Calling hook
				call(rax);
				mov(rcx, ptr[rsp + 0x20]);
				add(rsp, 0x30);
				test(al, al); //If the quest for which we're trying to fill the log entry is in one of ours
				jnz(BYPASS_STRING_LOAD); //Then bypass the classic log entry loading process
				//Else
				mov(rax, rcx); //Give the item back to the game, as it would have been without the hook

				L(RESUME_STANDARD_EXECUTION);
				mov(r11, inResumeStandardExecutionAddress); //Resume standard execution. R11 is volatile, unlike R15 that the hooked function doesn't save
				jmp(r11);

				L(BYPASS_STRING_LOAD);
				mov(rax, ptr[inStringAdr]); //Manually move our string
				mov(r11, inBypassAddress); //Resume execution after the normal string loading process we bypassed
				jmp(r11);
			}
		};
	}

	void RegisterHooks(SKSE::Trampoline& inTrampoline)
	{
		const auto fillLogEntryHook = RELOCATION_ID(24550, 25079).address() + 0x5C;
		FillLogEntryHookedPatch fleh{ reinterpret_cast<uintptr_t>(FillLogEntryHook), RELOCATION_ID(24766, 25246).address(), fillLogEntryHook + 0x5, fillLogEntryHook + 0x1B, reinterpret_cast<uintptr_t>(&targetLogEntry) };
		const auto fillLogEntryHooked = inTrampoline.allocate(fleh);
		inTrampoline.write_branch<5>(fillLogEntryHook, fillLogEntryHooked);
	}

	// # Sinks
	// =======================

	// ## Quest stage end callbacks
	// =======================
	namespace 
	{
		// The fragment system can't be manipulated easily in current version of CommonLibSSE (AFAIK) so we're just faking one by reacting to QuestStageEvents
		class QuestStageEventSink final : public RE::BSTEventSink<RE::TESQuestStageEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestStageEvent* inEvent, RE::BSTEventSource<RE::TESQuestStageEvent>*) override
			{
				const auto* dataManager = DataManager::GetSingleton();
				if(const auto questData = dataManager->questsData.find(inEvent->formID); questData != dataManager->questsData.end() && questData->second.script)
				{
					auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
					auto* policy = scriptMachine->GetObjectHandlePolicy();
					const auto* updatedQuest = RE::TESForm::LookupByID<RE::TESQuest>(inEvent->formID);
					const auto updatedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, updatedQuest);

					if(const auto stageToFragmentIndex = questData->second.stagesToFragmentIndex.find(inEvent->stage); stageToFragmentIndex != questData->second.stagesToFragmentIndex.end())
					{
						const auto* methodInfo = questData->second.script->type->GetMemberFuncIter() + stageToFragmentIndex->second;
						RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
						scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
					}
					return RE::BSEventNotifyControl::kStop;
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
		std::unique_ptr<QuestStageEventSink> questStageEventSink;
	}

	void RegisterSinks()
	{
		questStageEventSink = std::make_unique<QuestStageEventSink>();
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(questStageEventSink.get());
	}
}
