#include "SQG/API/DialogUtils.h"

#include "SQG/API/PackageUtils.h"
#include "SQG/API/QuestUtils.h"

namespace SQG
{
	TopicData* AddDialogTopic(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inPrompt, TopicData* inParentTopic)
	{
		auto* parentTopic = !inParentTopic ? &DataManager::GetSingleton()->dialogsData[inSpeaker->formID].topLevelTopic : inParentTopic;

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

		auto* topic = parentTopic->childEntries.emplace_back(std::make_unique<TopicData>()).get();
		topic->owningQuest = inQuest;
		topic->prompt = inPrompt;
		return topic;
	}

	void AddForceGreet(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inForceGreet, const std::list<RE::TESConditionItem*>& inConditions)
	{
		const auto* dataManager = DataManager::GetSingleton();

		std::unordered_map<std::string, PackageData> packageDataMap;
		packageDataMap["Topic"] = PackageData(dataManager->forceGreetTopic);
		auto* customForceGreetPackage = CreatePackageFromTemplate(dataManager->forceGreetPackage, inQuest, packageDataMap, inConditions);
		AddAliasPackage(inQuest, inSpeaker, customForceGreetPackage);

		DataManager::GetSingleton()->dialogsData[inSpeaker->formID].forceGreetAnswer = {nullptr, inForceGreet, "", {}, -1, -1, false};
	}

	void AddHelloTopic(const RE::TESObjectREFR* inSpeaker, const std::string& inHello, const std::list<RE::TESConditionItem*>& inConditions)
	{
		DataManager::GetSingleton()->dialogsData[inSpeaker->formID].helloAnswers.emplace_back(nullptr, inHello, "", inConditions, -1, -1, false);
	}
}
