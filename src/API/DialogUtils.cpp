#include "SQG/API/DialogUtils.h"

#include "DPF/API.h"
#include "Engine/Dialog.h"
#include "Engine/Package.h"
#include "SQG/API/PackageUtils.h"
#include "SQG/API/QuestUtils.h"

namespace SQG
{
	std::map<RE::FormID, DialogTopicData> dialogTopicsData;
	std::map<RE::FormID, std::string> forceGreetAnswers;

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

	void AddForceGreet(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inForceGreet, const std::list<PackageConditionDescriptor>& inConditions)
	{
		auto* customForceGreetPackage = SQG::CreatePackageFromTemplate(DPF::CreateForm(SQG::PackageEngine::forceGreetPackage), SQG::PackageEngine::forceGreetPackage, inQuest);
		std::unordered_map<std::string, SQG::PackageData> packageDataMap;
		packageDataMap["Topic"] = SQG::PackageData(SQG::DialogEngine::forceGreetTopic);
		FillPackageData(customForceGreetPackage, packageDataMap);
		SQG::FillPackageCondition(customForceGreetPackage, inConditions);
		SQG::AddAliasPackage(inQuest, inSpeaker, customForceGreetPackage);

		SQG::forceGreetAnswers[inSpeaker->formID] = inForceGreet;
	}
}
