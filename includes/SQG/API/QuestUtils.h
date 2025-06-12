#pragma once

#include "Engine/Data.h"

namespace SQG
{
	RE::TESQuest* CreateQuest();
	void AddQuestStage(RE::TESQuest* inQuest, std::uint16_t inIndex, QuestStageType inQuestStageType = QuestStageType::Default, const std::string& inLogEntry = "", int inScriptFragmentIndex = -1);
	void AddObjective(RE::TESQuest* inQuest, std::uint16_t inIndex, const std::string& inText, const std::list<uint8_t>& inQuestTargetsAliasIndexes = std::list<uint8_t>());
	void AddRefAlias(RE::TESQuest* inQuest, std::uint16_t inIndex, const std::string& inName, RE::TESObjectREFR* inRef); //TODO do other aliases (not forced, location...)
	void AddAliasPackage(const RE::TESQuest* inQuest, const RE::TESObjectREFR* inRef, RE::TESPackage* inPackage, const std::string& inFragmentScriptName = "");
}
