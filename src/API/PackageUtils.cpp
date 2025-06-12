#include "SQG/API/PackageUtils.h"

#include "DPF/API.h"
#include "Engine/Data.h"

namespace SQG
{
	namespace
	{
		void FillPackageData(RE::TESPackage* outPackage, const std::unordered_map<std::string, PackageData>& inPackageDataMap, const std::list<RE::TESConditionItem*>& inConditions)
		{
			const auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(outPackage->data);
			for (const auto& [name, packageData] : inPackageDataMap)
			{
				for (auto& nameMapData : customPackageData->nameMap->nameMap)
				{
					if (nameMapData.name.c_str() == name)
					{
						for (uint16_t i = 0; i < customPackageData->data.dataSize; ++i)
						{
							if (customPackageData->data.uids[i] == nameMapData.uid)
							{
								if (const auto packageDataTypeName = customPackageData->data.data[i]->GetTypeName(); packageDataTypeName == "Location")
								{
									const auto* bgsPackageDataLocation = reinterpret_cast<RE::BGSPackageDataLocation*>(customPackageData->data.data[i] - 1);  // NOLINT(clang-diagnostic-reinterpret-base-class, bugprone-pointer-arithmetic-on-polymorphic-object)
									bgsPackageDataLocation->pointer->locType = packageData.locationData.locType;
									bgsPackageDataLocation->pointer->rad = packageData.locationData.rad;
									if (packageData.locationData.locType == RE::PackageLocation::Type::kNearReference)
									{
										bgsPackageDataLocation->pointer->data = packageData.locationData.data;
									}
									//TODO other types
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
									if (packageDataTypeName == "SingleRef")
									{
										const auto* bgsPackageDataSingleRef = reinterpret_cast<RE::BGSPackageDataSingleRef*>(customPackageData->data.data[i]);
										bgsPackageDataSingleRef->pointer->targType = packageData.targetData.targType;
										bgsPackageDataSingleRef->pointer->target = packageData.targetData.target;
									}
									else if (packageDataTypeName == "TargetSelector")
									{
										const auto* bgsPackageDataTargetSelector = reinterpret_cast<RE::BGSPackageDataTargetSelector*>(customPackageData->data.data[i]);
										bgsPackageDataTargetSelector->pointer->targType = packageData.targetData.targType;
										bgsPackageDataTargetSelector->pointer->target = packageData.targetData.target;
									}
									else if (packageDataTypeName == "Bool")
									{
										const auto bgsPackageDataBool = reinterpret_cast<RE::BGSPackageDataBool*>(customPackageData->data.data[i]);
										bgsPackageDataBool->data.i = packageData.nativeData.b ? 2 : 0;
									}
									else if (packageDataTypeName == "Int")
									{
										const auto bgsPackageDataInt = reinterpret_cast<RE::BGSPackageDataInt*>(customPackageData->data.data[i]);
										bgsPackageDataInt->data.i = packageData.nativeData.i;
									}
									else if (packageDataTypeName == "Float")
									{
										const auto bgsPackageDataFloat = reinterpret_cast<RE::BGSPackageDataFloat*>(customPackageData->data.data[i]);
										bgsPackageDataFloat->data.f = packageData.nativeData.f;
									}
								}
								break;
							}
						}
					}
				}
			}
			RE::TESConditionItem** conditionItemHolder = &outPackage->packConditions.head;
			for (const auto inCondition : inConditions)
			{
				const auto conditionItem = *conditionItemHolder = inCondition;
				conditionItemHolder = &conditionItem->next;
			}
		}
	}

	void FillPackageWithTemplate(RE::TESPackage* outPackage, RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest)
	{
		outPackage->ownerQuest = inOwnerQuest;
		outPackage->procedureType = inPackageTemplate->procedureType;
		outPackage->packData.packType = RE::PACKAGE_TYPE::kPackage;
		outPackage->packData.interruptOverrideType = RE::PACK_INTERRUPT_TARGET::kSpectator;
		outPackage->packData.maxSpeed = RE::PACKAGE_DATA::PreferredSpeed::kRun;
		outPackage->packData.foBehaviorFlags.set(RE::PACKAGE_DATA::InterruptFlag::kHellosToPlayer, RE::PACKAGE_DATA::InterruptFlag::kRandomConversations, RE::PACKAGE_DATA::InterruptFlag::kObserveCombatBehaviour, RE::PACKAGE_DATA::InterruptFlag::kGreetCorpseBehaviour, RE::PACKAGE_DATA::InterruptFlag::kReactionToPlayerActions, RE::PACKAGE_DATA::InterruptFlag::kFriendlyFireComments, RE::PACKAGE_DATA::InterruptFlag::kAggroRadiusBehavior, RE::PACKAGE_DATA::InterruptFlag::kAllowIdleChatter, RE::PACKAGE_DATA::InterruptFlag::kWorldInteractions); 

		std::memcpy(outPackage->data, inPackageTemplate->data, sizeof(RE::TESCustomPackageData)); // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess)
		auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(outPackage->data);
		customPackageData->templateParent = inPackageTemplate;
	}

	RE::TESPackage* CreatePackageFromTemplate(RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest, const std::unordered_map<std::string, PackageData>& inPackageDataMap, const std::list<RE::TESConditionItem*>& inConditions)  // NOLINT(misc-use-internal-linkage)
	{
		auto* package = DPF::CreateForm(inPackageTemplate);
		FillPackageWithTemplate(package, inPackageTemplate, inOwnerQuest);
		FillPackageData(package, inPackageDataMap, inConditions);
		return package;
	}
}
