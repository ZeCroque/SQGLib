#pragma once

#include "API.h"
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

#pragma warning( push )
#pragma warning( disable : 4251)

namespace SQG
{
	// Dialogs data
	// =======================
	constexpr int SUB_TOPIC_COUNT = 4;

	struct SQG_API TopicData
	{
		struct AnswerData
		{
			AnswerData();
			AnswerData(TopicData* inParentEntry, const RE::BSFixedString& inAnswer, const RE::BSFixedString& inPromptOverride, const std::list<RE::TESConditionItem*>& inConditions, int inTargetStage, int inFragmentId, bool inAlreadySaid = false);

			TopicData* parentEntry;
			RE::BSFixedString answer;
			RE::BSFixedString promptOverride;
			std::list<RE::TESConditionItem*> conditions;
			int targetStage;
			int fragmentId;
			bool alreadySaid{ false };
		};

		TopicData();
		TopicData(TopicData const&) = delete;
		TopicData(TopicData const&&) = delete;
		~TopicData() = default;
		TopicData& operator=(TopicData const&&) = delete;
		TopicData& operator=(TopicData const&) = delete;

		void AddAnswer(const RE::BSString& inTopicInfoText, const RE::BSString& inTopicOverrideText, const std::list<RE::TESConditionItem*>& inConditions, int inTargetStage = -1, int inFragmentId = -1);

		RE::TESQuest* owningQuest;
		RE::BSFixedString prompt;
		std::list<AnswerData> answers;
		std::vector<std::unique_ptr<TopicData>> childEntries;
	};

	struct SQG_API DialogData
	{
		DialogData();
		DialogData(DialogData const&) = delete;
		DialogData(DialogData const&&) = delete;
		~DialogData() = default;
		DialogData& operator=(DialogData const&&) = delete;
		DialogData& operator=(DialogData const&) = delete;

		TopicData topLevelTopic;
		TopicData::AnswerData forceGreetAnswer;
		std::list<TopicData::AnswerData> helloAnswers;
	};

	// Quest Data
	// =======================
	enum class SQG_API QuestStageType : uint8_t
	{
		Default = 0,
		Startup = 1,
		Shutdown = 2
	};

	struct SQG_API QuestData
	{
		static constexpr int QUEST_STAGE_ITEMS_CHUNK_SIZE = 50;

		QuestData();
		QuestData(QuestData const&) = delete;
		QuestData(QuestData const&&) = delete;
		~QuestData() = default;
		QuestData& operator=(QuestData const&&) = delete;
		QuestData& operator=(QuestData const&) = delete;

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

	// Package Data
	// =======================
	union SQG_API PackageData
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

	// Data Manager
	// =======================
	class SQG_API DataManager : public REX::Singleton<DataManager>
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
		std::map<RE::FormID, DialogData> dialogsData;
		std::map<RE::FormID, std::string> packagesFragmentName;
		std::map<RE::FormID, QuestData> questsData;

		void ClearSerializationData();
	};
}

#pragma warning( pop ) 
