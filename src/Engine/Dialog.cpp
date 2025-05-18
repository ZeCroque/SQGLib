#include "Dialog.h"

#include "SQG/API/DialogUtils.h"

namespace SQG
{
	namespace DialogEngine
	{
		// # Common
		// =======================
		struct SpeakerData
		{
			std::map<RE::FormID, SQG::DialogTopicData::AnswerData*> topicsInfosBindings;
			bool hasAnyValidEntry = false;
		};
		std::map<RE::FormID, SpeakerData> speakersData;

		// # Hooks
		// =======================

		// ## Populate response
		// =======================
		static void PopulateResponseTextHook(RE::TESTopicInfo::ResponseData* generatedResponse, const RE::TESTopicInfo* parentTopicInfo)
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
					if(parentTopicInfo->formID == forceGreetTopic->topicInfos[0]->formID)
					{
						if(const auto forceGreetAnswer = forceGreetAnswers.find(speaker->formID); forceGreetAnswer != forceGreetAnswers.end())
						{
							generatedResponse->responseText = forceGreetAnswer->second;
						}
					}
					else if(const auto topicInfoBinding = speakersData[speaker->formID].topicsInfosBindings.find(parentTopicInfo->formID); topicInfoBinding != speakersData[speaker->formID].topicsInfosBindings.end())
					{
						generatedResponse->responseText = topicInfoBinding->second->answer;
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

		// ## On respond said
		// =======================
		static void OnResponseSaidHook()
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

		// ## Common
		// =======================
		void RegisterHooks(SKSE::Trampoline& inTrampoline)
		{
			const auto hookAddress = REL::Offset(0x393A32).address();
			PopulateResponseTextHookedPatch tsh{ reinterpret_cast<uintptr_t>(PopulateResponseTextHook), REL::ID(24985).address(), hookAddress + 0x5 };
		    inTrampoline.write_branch<5>(hookAddress, inTrampoline.allocate(tsh));

			const auto onResponseSaidHook = REL::Offset(0x5E869D).address();
			OnResponseSaidHookedPatch dsh{ reinterpret_cast<uintptr_t>(OnResponseSaidHook), REL::Offset(0x64F360).address(), onResponseSaidHook + 0x7 };
			const auto onDialogueSayHooked = inTrampoline.allocate(dsh);
		    inTrampoline.write_branch<5>(onResponseSaidHook, onDialogueSayHooked);
		}

		// # Sinks
		// =======================

		// ## TopicInfo sink
		// =======================
		RE::TESTopicInfo* subTopicsInfos[SQG::SUB_TOPIC_COUNT] { nullptr };
		RE::TESTopic* forceGreetTopic = nullptr;
		RE::TESConditionItem* impossibleCondition = nullptr;

		bool ProcessDialogEntry(RE::TESObjectREFR* inSpeaker, SQG::DialogTopicData& inDialogEntry, RE::TESTopicInfo* inOutTopicInfo)
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

		void ProcessDialogEntries(RE::TESObjectREFR* inSpeaker, const std::vector<std::unique_ptr<SQG::DialogTopicData>>& inEntries)
		{
			speakersData[inSpeaker->formID].hasAnyValidEntry = false;
			for(size_t i = 0; i < SQG::SUB_TOPIC_COUNT; ++i)
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
				if(SQG::dialogTopicsData.contains(a_event->speakerRef->formID))
				{
					if(a_event->type == RE::TESTopicInfoEvent::TopicInfoEventType::kTopicBegin)
					{
						if(!speakersData.contains(a_event->speakerRef->formID) || !speakersData[a_event->speakerRef->formID].hasAnyValidEntry)
						{
							ProcessDialogEntries(a_event->speakerRef.get(), SQG::dialogTopicsData[a_event->speakerRef->formID].childEntries);

							if(!speakersData[a_event->speakerRef->formID].hasAnyValidEntry)
							{
								return RE::BSEventNotifyControl::kContinue;
							}
						}
					}
					else
					{
						if(const auto topicInfoBinding = speakersData[a_event->speakerRef->formID].topicsInfosBindings.find(a_event->topicInfoFormID); topicInfoBinding != speakersData[a_event->speakerRef->formID].topicsInfosBindings.end())
						{
							SQG::DialogTopicData::AnswerData* answer = topicInfoBinding->second;
							answer->alreadySaid = true;

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
							ProcessDialogEntries(a_event->speakerRef.get(), !answer->parentEntry->childEntries.empty() ? answer->parentEntry->childEntries : SQG::dialogTopicsData[a_event->speakerRef->formID].childEntries);
						}
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
		std::unique_ptr<TopicInfoEventSink> topicInfoEventSink;

		// ## Common
		// =======================
		void RegisterSinks()
		{
			topicInfoEventSink = std::make_unique<TopicInfoEventSink>();
			RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(topicInfoEventSink.get());
		}

		// # Data
		// =======================
		void LoadData(RE::TESDataHandler* inDataHandler)
		{
			//TODO handle failure
			forceGreetTopic = reinterpret_cast<RE::TESTopic*>(inDataHandler->LookupForm(RE::FormID{0x00EAB4}, "SQGLib.esp"));
			subTopicsInfos[0] = reinterpret_cast<RE::TESTopicInfo*>(inDataHandler->LookupForm(RE::FormID{0x00BF96}, "SQGLib.esp"));
			subTopicsInfos[1] = reinterpret_cast<RE::TESTopicInfo*>(inDataHandler->LookupForm(RE::FormID{0x00BF99}, "SQGLib.esp"));
			subTopicsInfos[2] = reinterpret_cast<RE::TESTopicInfo*>(inDataHandler->LookupForm(RE::FormID{0x00BF9C}, "SQGLib.esp"));
			subTopicsInfos[3] = reinterpret_cast<RE::TESTopicInfo*>(inDataHandler->LookupForm(RE::FormID{0x00BF9F}, "SQGLib.esp"));

			impossibleCondition = new RE::TESConditionItem();
			impossibleCondition->data.dataID = std::numeric_limits<std::uint32_t>::max();
			impossibleCondition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
			impossibleCondition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kIsXBox);
			impossibleCondition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
			impossibleCondition->data.comparisonValue.f = 1.f;
			impossibleCondition->next = nullptr;
		}
	}
}
