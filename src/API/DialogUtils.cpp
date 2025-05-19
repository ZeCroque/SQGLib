#include "SQG/API/DialogUtils.h"

#include "Engine/Dialog.h"
#include "Engine/Package.h"
#include "SQG/API/PackageUtils.h"
#include "SQG/API/QuestUtils.h"

namespace SQG
{
	std::map<RE::FormID, DialogTopicData> dialogTopicsData;
	std::map<RE::FormID, DialogTopicData::AnswerData> forceGreetAnswers;
	std::map<RE::FormID, std::list<DialogTopicData::AnswerData>> helloAnswers;

	DialogTopicData* AddDialogTopic(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inPrompt, DialogTopicData* inParentTopic)
	{
		auto* parentTopic = !inParentTopic ? &dialogTopicsData[inSpeaker->formID] : inParentTopic;

		if(!parentTopic->childEntries.capacity())
		{
			parentTopic->childEntries.reserve(SUB_TOPIC_COUNT);
		}
		if(parentTopic->childEntries.size() == SUB_TOPIC_COUNT)
		{
			//You cannot have more than 4 subtopics
			assert(true);
			return nullptr;
		}

		auto* topic = parentTopic->childEntries.emplace_back(std::make_unique<DialogTopicData>()).get();
		topic->owningQuest = inQuest;
		topic->prompt = inPrompt;
		return topic;
	}

	void DialogTopicData::AddAnswer(const RE::BSString& inTopicInfoText, const RE::BSString& inTopicOverrideText, const std::list<RE::TESConditionItem*>& inConditions, const int inTargetStage, const int inFragmentId)
	{
		answers.emplace_back(this, inTopicInfoText, inTopicOverrideText, inConditions, inTargetStage ,inFragmentId, false);
	}

	void AddForceGreet(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inForceGreet, const std::list<RE::TESConditionItem*>& inConditions)
	{
		std::unordered_map<std::string, SQG::PackageData> packageDataMap;
		packageDataMap["Topic"] = SQG::PackageData(SQG::DialogEngine::forceGreetTopic);
		auto* customForceGreetPackage = SQG::CreatePackageFromTemplate(SQG::PackageEngine::forceGreetPackage, inQuest, packageDataMap, inConditions);
		SQG::AddAliasPackage(inQuest, inSpeaker, customForceGreetPackage);

		SQG::forceGreetAnswers[inSpeaker->formID] = {nullptr, inForceGreet, "", {}, -1, -1, false};
	}

	void AddHelloTopic(const RE::TESObjectREFR* inSpeaker, const std::string& inHello, const std::list<RE::TESConditionItem*>& inConditions)
	{
		SQG::helloAnswers[inSpeaker->formID].emplace_back(nullptr, inHello, "", inConditions, -1, -1, false);
	}
}
