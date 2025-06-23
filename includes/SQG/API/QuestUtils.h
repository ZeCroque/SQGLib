#pragma once

#include "API.h"
#include "SQG/API/Data.h"

namespace SQG
{
	SQG_API RE::TESQuest* CreateQuest();
	SQG_API void AddQuestStage(RE::TESQuest* inQuest, std::uint16_t inIndex, QuestStageType inQuestStageType = QuestStageType::Default, const std::string& inLogEntry = "", int inScriptFragmentIndex = -1);
	SQG_API void AddObjective(RE::TESQuest* inQuest, std::uint16_t inIndex, const std::string& inText, const std::list<uint8_t>& inQuestTargetsAliasIndexes = std::list<uint8_t>());
	SQG_API void AddRefAlias(RE::TESQuest* inQuest, std::uint16_t inIndex, const std::string& inName, RE::TESObjectREFR* inRef); //TODO do other aliases (not forced, location...)
	SQG_API void AddAliasPackage(const RE::TESQuest* inQuest, const RE::TESObjectREFR* inRef, RE::TESPackage* inPackage, const std::string& inFragmentScriptName = "");
}
