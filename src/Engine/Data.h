#pragma once

#include "REX/REX/Singleton.h"

namespace DPF
{
	class FileReader;
	class FileWriter;
}

// ReSharper disable once CppInconsistentNaming
namespace caprica::papyrus
{
	struct PapyrusCompilationNode;
}

namespace SQG
{
	// Dialogs data
	// =======================
	constexpr int SUB_TOPIC_COUNT = 4;

	struct DialogTopicData
	{
		struct AnswerData
		{
			DialogTopicData* parentEntry;
			RE::BSFixedString answer;
			RE::BSFixedString promptOverride;
			std::list<RE::TESConditionItem*> conditions;
			int targetStage;
			int fragmentId;
			bool alreadySaid{ false };
		};

		void AddAnswer(const RE::BSString& inTopicInfoText, const RE::BSString& inTopicOverrideText, const std::list<RE::TESConditionItem*>& inConditions, int inTargetStage = -1, int inFragmentId = -1);

		RE::TESQuest* owningQuest;
		RE::BSFixedString prompt;
		std::list<AnswerData> answers;
		std::vector<std::unique_ptr<DialogTopicData>> childEntries;
	};

	DialogTopicData::AnswerData DeserializeAnswer(DPF::FileReader* inSerializer, DialogTopicData* inParent);
	void DeserializeDialogTopic(DPF::FileReader* inSerializer, DialogTopicData& outData);
	void SerializeAnswer(DPF::FileWriter* inSerializer, const DialogTopicData::AnswerData& inAnswerData);
	void SerializeDialogTopic(DPF::FileWriter* inSerializer, const DialogTopicData& inData);

	// Quest Data
	// =======================
	enum class QuestStageType : uint8_t
	{
		Default = 0,
		Startup = 1,
		Shutdown = 2
	};

	struct QuestData
	{
		static constexpr int QUEST_STAGE_ITEMS_CHUNK_SIZE = 50;

		RE::TESQuest* quest;

		std::vector<std::unique_ptr<RE::TESQuestStage>> stages;

		//Quest loaded by the game all have their TESQuestStageItems instanced continuously. This replicates it, could be done in a more dynamic way but I think it's enough for now.
		RE::TESQuestStageItem stageItems[QUEST_STAGE_ITEMS_CHUNK_SIZE];
		int lastStageItemIndex;
		std::map<std::uint16_t, std::string> logEntries;

		struct Objective  // NOLINT(cppcoreguidelines-special-member-functions)
		{
			~Objective();

			RE::BGSQuestObjective objective;
		};
		std::list<std::unique_ptr<Objective>> objectives;

		struct AliasPackageData
		{
			RE::TESPackage* package;
			std::string fragmentScriptName;
		};
		std::map<RE::FormID, std::list<AliasPackageData>> aliasesPackagesData;

		RE::BSScript::Object* script = nullptr;
		std::map<uint16_t, int> stagesToFragmentIndex;
	};

	void DeserializeQuestData(DPF::FileReader* inSerializer, const RE::FormID inFormId);
	void SerializeQuestData(DPF::FileWriter* inSerializer, QuestData& inData);

	// Package Data
	// =======================
	union PackageData
	{
		using PackageNativeData = RE::BGSNamedPackageData<RE::IPackageData>::Data;

		PackageData();
		PackageData(RE::PackageLocation::Type inType, const RE::PackageLocation::Data& inData, std::uint32_t inRadius);
		PackageData(RE::PackageTarget::Type inType, const RE::PackageTarget::Target& inData);
		explicit PackageData(const RE::BGSNamedPackageData<RE::IPackageData>::Data& inData);
		explicit PackageData(RE::TESTopic* inTopic);
		~PackageData();
		PackageData& operator=(const PackageData& inOther);

		RE::PackageTarget targetData;
		RE::PackageLocation locationData;
		RE::TESTopic* topicData;
		PackageNativeData nativeData{};
	};

	void DeserializePackageData(DPF::FileReader* inSerializer, RE::TESPackage* outPackage);
	void SerializePackageData(DPF::FileWriter* inSerializer, const RE::TESPackage* inPackage);

	// Condition Data
	// =======================
	RE::TESConditionItem* DeserializeCondition(DPF::FileReader* inSerializer);
	void SerializeCondition(DPF::FileWriter* inSerializer, const RE::TESConditionItem* inCondition);

	// Script Data
	// =======================
	void DeserializeScript(DPF::FileReader* inSerializer, const std::string& inScriptName);
	void SerializeScript(DPF::FileWriter* inSerializer, const caprica::papyrus::PapyrusCompilationNode* inNode);

	// Data Manager
	// =======================
	class DataManager : public REX::Singleton<DataManager>
	{
	public:
		// Resources
		// =======================

		//Dialogs
		RE::TESTopic* forceGreetTopic;
		RE::TESTopicInfo* subTopicsInfos[SUB_TOPIC_COUNT];
		RE::TESConditionItem* impossibleCondition;

		//Packages
		RE::TESPackage* acquirePackage;
		RE::TESPackage* activatePackage;
		RE::TESPackage* travelPackage;
		RE::TESPackage* forceGreetPackage;

		//Quests
		RE::TESQuest* referenceQuest;

		//Common
		void InitData();

		// Serialized data
		// =======================
		std::map<std::string, caprica::papyrus::PapyrusCompilationNode*> compiledScripts;

		std::map<RE::FormID, DialogTopicData> dialogTopicsData;
		std::map<RE::FormID, DialogTopicData::AnswerData> forceGreetAnswers;
		std::map<RE::FormID, std::list<DialogTopicData::AnswerData>> helloAnswers;

		std::map<RE::FormID, std::string> packagesFragmentName;

		//The first elem of the pair is the index of last added item
		std::map<RE::FormID, QuestData> questsData;

		void ClearSerializationData();
	};
}
