#pragma once

constexpr int SUB_TOPIC_COUNT = 4;

struct DialogTopicData
{
	struct AnswerData
	{
		DialogTopicData* parentEntry; 
		RE::BSFixedString answer;
		RE::BSFixedString promptOverride;
		std::vector<RE::TESConditionItem*> conditions;
		int targetStage;
		int fragmentId;
		bool alreadySaid { false };
	};

	void AddAnswer(const RE::BSString& inTopicInfoText, const RE::BSString& inTopicOverrideText, const std::vector<RE::TESConditionItem*>& inConditions, int inTargetStage = -1, int inFragmentId = -1);

	RE::TESQuest* owningQuest;
	RE::BSFixedString prompt;	// in CK : Topic text max 80/TopicInfo text max 150 -> TODO test if hardcap
	std::vector<AnswerData> answers;
	std::vector<std::unique_ptr<DialogTopicData>> childEntries;
};
extern std::map<RE::FormID, DialogTopicData> dialogTopicsData;

DialogTopicData* AddDialogTopic(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inPrompt = "", DialogTopicData* inParentTopic = nullptr);