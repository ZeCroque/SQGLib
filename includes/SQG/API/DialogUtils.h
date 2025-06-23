#pragma once

#include "API.h"
#include "SQG/API/Data.h"

namespace SQG
{
	SQG_API TopicData* AddDialogTopic(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inPrompt = "", TopicData* inParentTopic = nullptr);

	SQG_API void AddForceGreet(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inForceGreet, const std::list<RE::TESConditionItem*>& inConditions);

	SQG_API void AddHelloTopic(const RE::TESObjectREFR* inSpeaker, const std::string& inHello, const std::list<RE::TESConditionItem*>& inConditions = std::list<RE::TESConditionItem*>());
}
