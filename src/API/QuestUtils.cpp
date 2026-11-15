#include "SQG/API/QuestUtils.h"

#include "DPF/API.h"

namespace SQG
{
	RE::TESQuest* CreateQuest()
	{
		auto* dataManager = DataManager::GetSingleton();
		const auto result = DPF::CreateForm(dataManager->referenceQuest);
		dataManager->questsData[result->formID].quest = result;
		return result;
	}

	void AddQuestStage(RE::TESQuest* inQuest, const std::uint16_t inIndex, const QuestStageType inQuestStageType, const std::string& inLogEntry, const int inScriptFragmentIndex)
	{
		auto* dataManager = DataManager::GetSingleton();
		const auto& stage = dataManager->questsData[inQuest->formID].stages.emplace_back(std::make_unique<RE::TESQuestStage>());
		switch(inQuestStageType)
		{

		case QuestStageType::Startup:
			stage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kStartUpStage);
			stage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kKeepInstanceDataFromHereOn);
			break;
		case QuestStageType::Shutdown:
			stage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kShutDownStage);
			break;
		case QuestStageType::Default:
			break;
		}

		inQuest->stages.emplace_front(stage.get());

		stage->data.index = inIndex;

		if(!inLogEntry.empty())
		{
			stage->questStageItem = &dataManager->questsData[inQuest->formID].stageItems.emplace_back();
			stage->questStageItem->hasLogEntry = true;
			stage->questStageItem->owner = inQuest;
			stage->questStageItem->owningStage = stage.get();
			stage->questStageItem->data = inQuestStageType == QuestStageType::Shutdown;
			dataManager->questsData[inQuest->formID].logEntries[inIndex] = inLogEntry;
		}

		if(inScriptFragmentIndex > -1)
		{
			dataManager->questsData[inQuest->formID].stagesToFragmentIndex[inIndex] = inScriptFragmentIndex;
		}
	}

	void AddObjective(RE::TESQuest* inQuest, const std::uint16_t inIndex, const std::string& inText, const std::list<uint8_t>& inQuestTargetsAliasIndexes)
	{
		const auto& objectiveData = DataManager::GetSingleton()->questsData[inQuest->formID].objectives.emplace_back(std::make_unique<QuestData::Objective>());
		objectiveData->objective.index = inIndex;
		objectiveData->objective.displayText = inText;
		objectiveData->objective.ownerQuest = inQuest;
		objectiveData->objective.initialized = true; //Seems to be unused and never set by the game. Setting it in case because it is on data from CK.

		if(const auto targetsCount = inQuestTargetsAliasIndexes.size())
		{
			objectiveData->objective.numTargets = static_cast<std::uint32_t>(targetsCount);
			objectiveData->objective.targets = new RE::TESQuestTarget*[targetsCount]();
			int i = 0;
			for(const auto questTargetAliasIndex : inQuestTargetsAliasIndexes)
			{
				objectiveData->objective.targets[i] = new RE::TESQuestTarget();
				objectiveData->objective.targets[i]->alias = questTargetAliasIndex;
				++i;
			}
		}
		inQuest->objectives.push_front(&objectiveData->objective);
	}

	void AddRefAlias(RE::TESQuest* inQuest, const std::uint16_t inIndex, const std::string& inName, RE::TESObjectREFR* inRef)
	{
		auto* createdAlias = RE::BGSBaseAlias::Create<RE::BGSRefAlias>(); 
		createdAlias->aliasID = inIndex;
		createdAlias->aliasName = inName;
		createdAlias->fillType = RE::BGSBaseAlias::FILL_TYPE::kForced;
		createdAlias->owningQuest = inQuest;
		createdAlias->fillData.forced = RE::BGSRefAlias::ForcedFillData{inRef->CreateRefHandle()};
		inQuest->aliasAccessLock.LockForWrite();
		inQuest->aliases.push_back(createdAlias);
		inQuest->aliasAccessLock.UnlockForWrite();
	}

	void AddAliasPackage(const RE::TESQuest* inQuest, const RE::TESObjectREFR* inRef, RE::TESPackage* inPackage, const std::string& inFragmentScriptName)
	{
		DataManager::GetSingleton()->questsData[inQuest->formID].aliasesPackagesData[inRef->formID].push_back({inPackage, inFragmentScriptName});
	}
}
