#include "Dialog.h"

#include "Data.h"
#include "SQG/API/ConditionUtils.h"

namespace SQG::Engine::Dialog
{
	// # Common
	// =======================
	namespace
	{
		struct SpeakerData
		{
			std::map<RE::FormID, SQG::TopicData::AnswerData*> topicsInfosBindings;
			bool hasAnyValidEntry = false;
		};
		std::map<RE::FormID, SpeakerData> speakersData;
		TopicData::AnswerData* lastSelectedAnswer;
		RE::FormID lastSpeakerId;
	}

	// # Hooks
	// =======================
	namespace
	{
		// ## Populate response
		// =======================
		void PopulateResponseTextHook(RE::TESTopicInfo::ResponseData* generatedResponse, const RE::TESTopicInfo* parentTopicInfo)
		{
			auto* dataManager = DataManager::GetSingleton();
			if(const auto* topicManager = RE::MenuTopicManager::GetSingleton(); topicManager && topicManager->dialogueList)
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
					if(parentTopicInfo->formID == DataManager::GetSingleton()->forceGreetTopic->topicInfos[0]->formID)
					{
						if(const auto forceGreetAnswer = dataManager->dialogsData.find(speaker->formID); forceGreetAnswer != dataManager->dialogsData.end())
						{
							generatedResponse->responseText = forceGreetAnswer->second.forceGreetAnswer.answer;
							lastSelectedAnswer = &forceGreetAnswer->second.forceGreetAnswer;
						}
					}
					else if(parentTopicInfo->parentTopic->data.subtype.all(RE::DIALOGUE_DATA::Subtype::kHello))
					{
						if(!lastSelectedAnswer || (lastSelectedAnswer->parentEntry && lastSelectedAnswer->parentEntry->childEntries.empty()))
						{
							if(const auto helloAnswer = dataManager->dialogsData.find(speaker->formID); helloAnswer != dataManager->dialogsData.end())
							{
								std::vector<TopicData::AnswerData*> availableHello;
								availableHello.reserve(helloAnswer->second.helloAnswers.size());
								for (auto& answer : helloAnswer->second.helloAnswers)
								{
									if (CheckCondition(speaker, answer.conditions))
									{
										availableHello.push_back(&answer);
									}
								}
								std::mt19937 mt(static_cast<int32_t>(std::time({})));  // NOLINT(cert-msc51-cpp)
								std::uniform_int_distribution d(0, static_cast<int>(availableHello.size()) - 1);
								generatedResponse->responseText = availableHello[d(mt)]->answer;
							}
						}
						else
						{
							generatedResponse->responseText = lastSelectedAnswer->answer;
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
		void OnResponseSaidHook()
		{
			if (const auto* topicManager = RE::MenuTopicManager::GetSingleton(); topicManager && topicManager->dialogueList)
			{
				const RE::TESObjectREFR* speaker = nullptr;
				if(const auto* currentSpeaker = topicManager->speaker.get().get())
				{
					speaker = currentSpeaker;
				}
				else if(const auto* lastSpeaker = topicManager->lastSpeaker.get().get())
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
	}

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
	namespace
	{
		// ## TopicInfo sink
		// =======================

		bool ProcessDialogEntry(RE::TESObjectREFR* inSpeaker, SQG::TopicData& inDialogEntry, RE::TESTopicInfo* inOutTopicInfo)
		{
			bool hasAnyValidAnswer = false;
			for(auto& answerData : inDialogEntry.answers)
			{
				if(CheckCondition(inSpeaker, answerData.conditions))
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
				std::memcpy(inOutTopicInfo->objConditions.head, DataManager::GetSingleton()->impossibleCondition, sizeof(RE::TESConditionItem));  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-nontrivial-memcall)
			}
			return hasAnyValidAnswer;
		}

		void ProcessDialogEntries(RE::TESObjectREFR* inSpeaker, const std::vector<std::unique_ptr<SQG::TopicData>>& inEntries)
		{
			speakersData[inSpeaker->formID].hasAnyValidEntry = false;
			for(size_t i = 0; i < SUB_TOPIC_COUNT; ++i)
			{
				const auto* dataManager = DataManager::GetSingleton();
				if(i < inEntries.size())
				{
					if(ProcessDialogEntry(inSpeaker, *inEntries[i], dataManager->subTopicsInfos[i]))
					{
						speakersData[inSpeaker->formID].hasAnyValidEntry = true;
					}
				}
				else
				{
					dataManager->subTopicsInfos[i]->objConditions.head = new RE::TESConditionItem();
					std::memcpy(dataManager->subTopicsInfos[i]->objConditions.head, dataManager->impossibleCondition, sizeof(RE::TESConditionItem));  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-nontrivial-memcall)
				}
			}
		}

		class TopicInfoEventSink final : public RE::BSTEventSink<RE::TESTopicInfoEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESTopicInfoEvent* inEvent, RE::BSTEventSource<RE::TESTopicInfoEvent>*) override
			{
				if(auto* dataManager = DataManager::GetSingleton(); dataManager->dialogsData.contains(inEvent->speakerRef->formID))
				{
					if(inEvent->type == RE::TESTopicInfoEvent::TopicInfoEventType::kTopicBegin)
					{
						if(!speakersData.contains(inEvent->speakerRef->formID) || !speakersData[inEvent->speakerRef->formID].hasAnyValidEntry || lastSpeakerId != inEvent->speakerRef->formID)
						{
							ProcessDialogEntries(inEvent->speakerRef.get(), dataManager->dialogsData[inEvent->speakerRef->formID].topLevelTopic.childEntries);
						}
					}
					else
					{
						if(const auto topicInfoBinding = speakersData[inEvent->speakerRef->formID].topicsInfosBindings.find(inEvent->topicInfoFormID); topicInfoBinding != speakersData[inEvent->speakerRef->formID].topicsInfosBindings.end())
						{
							SQG::TopicData::AnswerData* answer = topicInfoBinding->second;
							answer->alreadySaid = true;
							lastSelectedAnswer = answer;

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
								scriptMachine->FindBoundObject(updatedQuestHandle, (std::string("TF_") + answer->parentEntry->owningQuest->GetFormEditorID()).c_str(), questCustomScriptObject);

								const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() + answer->fragmentId;
								RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
								scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
							}

							//Process following entries in tree  or root dialog entries if it was a leaf
							ProcessDialogEntries(inEvent->speakerRef.get(), !answer->parentEntry->childEntries.empty() ? answer->parentEntry->childEntries : dataManager->dialogsData[inEvent->speakerRef->formID].topLevelTopic.childEntries);
						}
					}
					lastSpeakerId = inEvent->speakerRef->formID;
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
		std::unique_ptr<TopicInfoEventSink> topicInfoEventSink;
	}

	void RegisterSinks()
	{
		topicInfoEventSink = std::make_unique<TopicInfoEventSink>();
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(topicInfoEventSink.get());
	}
}
