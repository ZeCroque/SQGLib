#include "SQG/DialogUtils.h"

std::map<RE::FormID, DialogTopicData> dialogTopicsData;

DialogTopicData* AddDialogTopic(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inPrompt, DialogTopicData* inParentTopic)
{
	if(inParentTopic && inParentTopic->childEntries.size() == SUB_TOPIC_COUNT)
	{
		//You cannot have more than 4 subtopics
		assert(true);
		return nullptr;
	}

	auto* topic = (!inParentTopic ? &dialogTopicsData[inSpeaker->formID] : inParentTopic)->childEntries.emplace_back(std::make_unique<DialogTopicData>()).get();
	topic->owningQuest = inQuest;
	topic->prompt = inPrompt;
	return topic;
}

void DialogTopicData::AddAnswer(const RE::BSString& inTopicInfoText, const RE::BSString& inTopicOverrideText, const std::vector<RE::TESConditionItem*>& inConditions, const int inTargetStage, const int inFragmentId)
{
	answers.emplace_back(this, inTopicInfoText, inTopicOverrideText, inConditions, inTargetStage ,inFragmentId, false);
}