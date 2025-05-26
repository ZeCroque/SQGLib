#pragma once

namespace SQG
{
	enum class QuestStageType
	{
		Default = 0,
		Startup = 1,
		Shutdown = 2
	};

	struct QuestData
	{
		static constexpr int QUEST_STAGE_ITEMS_CHUNK_SIZE = 50;

		std::vector<std::unique_ptr<RE::TESQuestStage>> stages;

		//Quest loaded by the game all have their TESQuestStageItems instanced continuously. This replicates it, could be done in a more dynamic way but I think it's enough for now.
		RE::TESQuestStageItem stageItems[QUEST_STAGE_ITEMS_CHUNK_SIZE];
		int lastStageItemIndex;
		std::map<std::uint16_t, std::string> logEntries;

		struct Objective
		{
			~Objective();

			RE::BGSQuestObjective objective;
		};
		std::vector<std::unique_ptr<Objective>> objectives;

		struct AliasPackageData
		{
			RE::TESPackage* package;
			std::string fragmentScriptName;
		};
		std::map<RE::FormID, std::vector<AliasPackageData>> aliasesPackagesData;

		RE::BSScript::Object* script = nullptr;
		std::map<uint16_t, int> stagesToFragmentIndex;
	};

	//The first elem of the pair is the index of last added item
	extern std::map<RE::FormID, QuestData> questsData;

	RE::TESQuest* CreateQuest();
	void AddQuestStage(RE::TESQuest* inQuest, std::uint16_t inIndex, QuestStageType inQuestStageType = QuestStageType::Default, const std::string& inLogEntry = "", int inScriptFragmentIndex = -1);
	void AddObjective(RE::TESQuest* inQuest, std::uint16_t inIndex, const std::string& inText, const std::vector<uint8_t>& inQuestTargetsAliasIndexes = std::vector<uint8_t>());
	void AddRefAlias(RE::TESQuest* inQuest, std::uint16_t inIndex, const std::string& inName, RE::TESObjectREFR* inRef); //TODO do other aliases (not forced, location...)
	void AddAliasPackage(const RE::TESQuest* inQuest, const RE::TESObjectREFR* inRef, RE::TESPackage* inPackage, const std::string& inFragmentScriptName = "");
}
