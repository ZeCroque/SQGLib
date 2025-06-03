#pragma once

namespace DPF
{
	class FileReader;
	class FileWriter;
}

namespace SQG
{
	constexpr int SUB_TOPIC_COUNT = 4;

	struct DialogTopicData
	{
		struct AnswerData
		{
			DialogTopicData* parentEntry; 
			RE::BSFixedString answer;
			RE::BSFixedString promptOverride;
			std::list<RE::TESConditionItem*> conditions;
			int targetStage;
			int fragmentId;
			bool alreadySaid { false };
		};

		void AddAnswer(const RE::BSString& inTopicInfoText, const RE::BSString& inTopicOverrideText, const std::list<RE::TESConditionItem*>& inConditions, int inTargetStage = -1, int inFragmentId = -1);

		RE::TESQuest* owningQuest;
		RE::BSFixedString prompt;
		std::list<AnswerData> answers;
		std::vector<std::unique_ptr<DialogTopicData>> childEntries;
	};
	extern std::map<RE::FormID, DialogTopicData> dialogTopicsData;
	extern std::map<RE::FormID, DialogTopicData::AnswerData> forceGreetAnswers;
	extern std::map<RE::FormID, std::list<DialogTopicData::AnswerData>> helloAnswers;

	DialogTopicData* AddDialogTopic(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inPrompt = "", DialogTopicData* inParentTopic = nullptr);

	void AddForceGreet(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inForceGreet, const std::list<RE::TESConditionItem*>& inConditions);

	void AddHelloTopic(const RE::TESObjectREFR* inSpeaker, const std::string& inHello, const std::list<RE::TESConditionItem*>& inConditions = std::list<RE::TESConditionItem*>());

	DialogTopicData::AnswerData DeserializeAnswer(DPF::FileReader* inSerializer, DialogTopicData* inParent);
	void DeserializeDialogTopic(DPF::FileReader* inSerializer, DialogTopicData& outData);
	void SerializeAnswer(DPF::FileWriter* inSerializer, const DialogTopicData::AnswerData& inAnswerData);
	void SerializeDialogTopic(DPF::FileWriter* inSerializer, const DialogTopicData& inData);
}
