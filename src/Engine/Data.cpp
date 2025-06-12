#include "Data.h"

namespace SQG
{
	// Quest Data
	// =======================
	QuestData::Objective::~Objective()
	{
		for(uint32_t i = 0; i < objective.numTargets; ++i)
		{
			delete objective.targets[i];
		}
		delete[] objective.targets;
	}

	// Package Manager
	// =======================
	PackageData::PackageData()
	{
		std::memset(this, 0, sizeof(PackageData));  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess)
	}

	PackageData::PackageData(const RE::PackageLocation::Type inType, const RE::PackageLocation::Data& inData, std::uint32_t inRadius) : PackageData()
	{
		locationData.locType = inType;
		locationData.data = inData;
		locationData.rad = inRadius;
	}

	PackageData::PackageData(const RE::PackageTarget::Type inType, const RE::PackageTarget::Target& inData) : PackageData()
	{
		targetData.targType = inType;
		targetData.target = inData;
	}

	PackageData::PackageData(const RE::BGSNamedPackageData<RE::IPackageData>::Data& inData) : PackageData()
	{
		nativeData = inData;
	}

	PackageData::PackageData(RE::TESTopic* inTopic) : PackageData()
	{
		topicData = inTopic;
	}

	PackageData::~PackageData()  // NOLINT(modernize-use-equals-default)
	{
	}

	PackageData& PackageData::operator=(const PackageData& inOther)
	{
		std::memcpy(this, &inOther, sizeof(PackageData));  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess)
		return *this;
	}

	// Data Manager
	// =======================
	void DataManager::InitData()
	{
		const auto dataHandler = RE::TESDataHandler::GetSingleton();

		forceGreetTopic = reinterpret_cast<RE::TESTopic*>(dataHandler->LookupForm(RE::FormID{ 0x00EAB4 }, "SQGLib.esp"));
		subTopicsInfos[0] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{ 0x00BF96 }, "SQGLib.esp"));
		subTopicsInfos[1] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{ 0x00BF99 }, "SQGLib.esp"));
		subTopicsInfos[2] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{ 0x00BF9C }, "SQGLib.esp"));
		subTopicsInfos[3] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{ 0x00BF9F }, "SQGLib.esp"));

		impossibleCondition = new RE::TESConditionItem();
		impossibleCondition->data.dataID = std::numeric_limits<std::uint32_t>::max();
		impossibleCondition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
		impossibleCondition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kIsXBox);
		impossibleCondition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
		impossibleCondition->data.comparisonValue.f = 1.f;
		impossibleCondition->next = nullptr;

		activatePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{ 0x0019B2D });
		acquirePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{ 0x0019713 });
		travelPackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{ 0x0016FAA });
		forceGreetPackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{ 0x003C1C4 });

		referenceQuest = reinterpret_cast<RE::TESQuest*>(dataHandler->LookupForm(RE::FormID{ 0x003371 }, "SQGLib.esp"));
	}

	void DataManager::ClearSerializationData()
	{
		questsData.clear();
		dialogTopicsData.clear();
		forceGreetAnswers.clear();
		helloAnswers.clear();
		compiledScripts.clear();
		packagesFragmentName.clear();
	}
}
