#pragma once

#include "Engine/Data.h"

namespace DPF
{
	class FileReader;
	class FileWriter;
}

namespace SQG
{
	DialogTopicData* AddDialogTopic(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inPrompt = "", DialogTopicData* inParentTopic = nullptr);

	void AddForceGreet(RE::TESQuest* inQuest, const RE::TESObjectREFR* inSpeaker, const std::string& inForceGreet, const std::list<RE::TESConditionItem*>& inConditions);

	void AddHelloTopic(const RE::TESObjectREFR* inSpeaker, const std::string& inHello, const std::list<RE::TESConditionItem*>& inConditions = std::list<RE::TESConditionItem*>());

	DialogTopicData::AnswerData DeserializeAnswer(DPF::FileReader* inSerializer, DialogTopicData* inParent);
	void DeserializeDialogTopic(DPF::FileReader* inSerializer, DialogTopicData& outData);
	void SerializeAnswer(DPF::FileWriter* inSerializer, const DialogTopicData::AnswerData& inAnswerData);
	void SerializeDialogTopic(DPF::FileWriter* inSerializer, const DialogTopicData& inData);
}
