#include "SQG/API/PackageUtils.h"

namespace SQG
{
	// Base
	// =======================
	RE::TESPackage* CreatePackageFromTemplate(RE::TESPackage* package, RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest)
	{

		package->ownerQuest = inOwnerQuest;
		package->procedureType = inPackageTemplate->procedureType;
		package->packData.packType = RE::PACKAGE_TYPE::kPackage;
		package->packData.interruptOverrideType = RE::PACK_INTERRUPT_TARGET::kSpectator;
		package->packData.maxSpeed = RE::PACKAGE_DATA::PreferredSpeed::kRun;
		package->packData.foBehaviorFlags.set(RE::PACKAGE_DATA::InterruptFlag::kHellosToPlayer, RE::PACKAGE_DATA::InterruptFlag::kRandomConversations, RE::PACKAGE_DATA::InterruptFlag::kObserveCombatBehaviour, RE::PACKAGE_DATA::InterruptFlag::kGreetCorpseBehaviour, RE::PACKAGE_DATA::InterruptFlag::kReactionToPlayerActions, RE::PACKAGE_DATA::InterruptFlag::kFriendlyFireComments, RE::PACKAGE_DATA::InterruptFlag::kAggroRadiusBehavior, RE::PACKAGE_DATA::InterruptFlag::kAllowIdleChatter, RE::PACKAGE_DATA::InterruptFlag::kWorldInteractions); 

		std::memcpy(package->data, inPackageTemplate->data, sizeof(RE::TESCustomPackageData)); // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess)
		auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(package->data);
		customPackageData->templateParent = inPackageTemplate;

		return package;
	}

	// PackageData
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

	void FillPackageData(const RE::TESPackage* outPackage, const std::unordered_map<std::string, PackageData>& inPackageDataMap) //TODO use uid
	{
		const auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(outPackage->data);
		for(const auto& [name, packageData] : inPackageDataMap)
		{
			for(auto& nameMapData : customPackageData->nameMap->nameMap)
			{
				if(nameMapData.name.c_str() == name)
				{
					for(auto i = 0; i < customPackageData->data.dataSize; ++i)
					{
						if(customPackageData->data.uids[i] == nameMapData.uid)
						{
							const auto packageDataTypeName = customPackageData->data.data[i]->GetTypeName();

							if(packageDataTypeName == "Location")
							{
								const auto* bgsPackageDataLocation = reinterpret_cast<RE::BGSPackageDataLocation*>(customPackageData->data.data[i] - 1);
								bgsPackageDataLocation->pointer->locType = packageData.locationData.locType;
								bgsPackageDataLocation->pointer->rad = packageData.locationData.rad;
								bgsPackageDataLocation->pointer->data = packageData.locationData.data;

								RE::BSString result;
								bgsPackageDataLocation->GetDataAsString(&result);
								SKSE::log::debug("package-data[{0}]-as-string:{1}"sv, packageDataTypeName.c_str(), result.c_str());
							}
							else if (packageDataTypeName == "Topic")
							{
								const auto bgsPackageTopicData = reinterpret_cast<RE::BGSPackageDataTopic*>(customPackageData->data.data[i]);
								bgsPackageTopicData->unk00 = 0;
								bgsPackageTopicData->unk08 = 0;
								bgsPackageTopicData->topic = packageData.topicData;
							}
							else
							{
								if(packageDataTypeName == "SingleRef")
								{
									const auto* bgsPackageDataSingleRef = reinterpret_cast<RE::BGSPackageDataSingleRef*>(customPackageData->data.data[i]);
									bgsPackageDataSingleRef->pointer->targType = packageData.targetData.targType;
									bgsPackageDataSingleRef->pointer->target = packageData.targetData.target;
								}
								else if(packageDataTypeName == "TargetSelector")
								{
									const auto* bgsPackageDataTargetSelector = reinterpret_cast<RE::BGSPackageDataTargetSelector*>(customPackageData->data.data[i]);
									bgsPackageDataTargetSelector->pointer->targType = packageData.targetData.targType;
									bgsPackageDataTargetSelector->pointer->target = packageData.targetData.target;
								}
								else if(packageDataTypeName == "Bool")
								{
									const auto bgsPackageDataBool = reinterpret_cast<RE::BGSPackageDataBool*>(customPackageData->data.data[i]);
									bgsPackageDataBool->data.i = packageData.nativeData.b ? 2 : 0;
								}
								else if(packageDataTypeName == "Int")
								{
									const auto bgsPackageDataInt = reinterpret_cast<RE::BGSPackageDataInt*>(customPackageData->data.data[i]);
									bgsPackageDataInt->data.i = packageData.nativeData.i;
								}
								else if(packageDataTypeName == "Float")
								{
									const auto bgsPackageDataFloat = reinterpret_cast<RE::BGSPackageDataFloat*>(customPackageData->data.data[i]);
									bgsPackageDataFloat->data.f = packageData.nativeData.f;
								}
								RE::BSString result;
								customPackageData->data.data[i]->GetDataAsString(&result);
								SKSE::log::debug("package-data[{0}]-as-string:{1}"sv, packageDataTypeName.c_str(), result.c_str());
							}
							break;
						}
					}
				}
			}
		}
	}

	// Conditions
	// =======================
	void FillPackageCondition(RE::TESPackage* inPackage, const std::list<PackageConditionDescriptor>& packageConditionDescriptors)
	{
		RE::TESConditionItem** conditionItemHolder = &inPackage->packConditions.head;
		for(auto& packageConditionDescriptor : packageConditionDescriptors)
		{
			const auto conditionItem = *conditionItemHolder = new RE::TESConditionItem();
			conditionItem->data.dataID = std::numeric_limits<std::uint32_t>::max();
			std::memset(reinterpret_cast<void*>(&inPackage->packConditions.head->data.functionData.function), 0, sizeof(inPackage->packConditions.head->data.functionData.function.underlying()));

			conditionItem->data.functionData.function.set(packageConditionDescriptor.functionId);
			conditionItem->data.functionData.params[0] = packageConditionDescriptor.functionCaller;
			conditionItem->data.flags.opCode = packageConditionDescriptor.opCode;
			conditionItem->data.flags.global = packageConditionDescriptor.useGlobal;
			conditionItem->data.comparisonValue = packageConditionDescriptor.data;
			conditionItem->data.flags.isOR = packageConditionDescriptor.isOr;

			conditionItemHolder = &conditionItem->next;
		}
	}
}
