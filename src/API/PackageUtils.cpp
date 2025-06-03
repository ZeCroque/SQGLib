#include "SQG/API/PackageUtils.h"

#include "DPF/API.h"
#include "SQG/API/ConditionUtils.h"

namespace SQG
{
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

	void FillPackageData(RE::TESPackage* outPackage, const std::unordered_map<std::string, PackageData>& inPackageDataMap, const std::list<RE::TESConditionItem*>& inConditions)
	{
		const auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(outPackage->data);
		for(const auto& [name, packageData] : inPackageDataMap)
		{
			for(auto& nameMapData : customPackageData->nameMap->nameMap)
			{
				if(nameMapData.name.c_str() == name)
				{
					for(uint16_t i = 0; i < customPackageData->data.dataSize; ++i)
					{
						if(customPackageData->data.uids[i] == nameMapData.uid)
						{
							if(const auto packageDataTypeName = customPackageData->data.data[i]->GetTypeName(); packageDataTypeName == "Location")
							{
								const auto* bgsPackageDataLocation = reinterpret_cast<RE::BGSPackageDataLocation*>(customPackageData->data.data[i] - 1);  // NOLINT(clang-diagnostic-reinterpret-base-class, bugprone-pointer-arithmetic-on-polymorphic-object)
								bgsPackageDataLocation->pointer->locType = packageData.locationData.locType;
								bgsPackageDataLocation->pointer->rad = packageData.locationData.rad;
								if(packageData.locationData.locType == RE::PackageLocation::Type::kNearReference)
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
							}
							break;
						}
					}
				}
			}
		}
		RE::TESConditionItem** conditionItemHolder = &outPackage->packConditions.head;
		for(const auto inCondition : inConditions)
		{
			const auto conditionItem = *conditionItemHolder = inCondition;
			conditionItemHolder = &conditionItem->next;
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

	RE::TESPackage* CreatePackageFromTemplate(RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest, const std::unordered_map<std::string, PackageData>& inPackageDataMap, const std::list<RE::TESConditionItem*>& inConditions)
	{
		auto* package = DPF::CreateForm(inPackageTemplate);
		FillPackageWithTemplate(package, inPackageTemplate, inOwnerQuest);
		FillPackageData(package, inPackageDataMap, inConditions);
		return package;
	}

	void DeserializePackageData(DPF::FileReader* inSerializer, RE::TESPackage* outPackage)
	{
		const auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(outPackage->data);

		const int dataCount = inSerializer->Read<uint16_t>();
		for(auto i = 0; i < dataCount; ++i)
		{
			const auto dataUid = inSerializer->Read<int8_t>();
			for(uint16_t j = 0; j < customPackageData->data.dataSize; ++j)
			{
				if(dataUid == customPackageData->data.uids[j])
				{
					const auto packageDataTypeName = inSerializer->ReadString();

					if(packageDataTypeName == "Location")
					{
						const auto* bgsPackageDataLocation = reinterpret_cast<RE::BGSPackageDataLocation*>(customPackageData->data.data[i] - 1);  // NOLINT(clang-diagnostic-reinterpret-base-class, bugprone-pointer-arithmetic-on-polymorphic-object)
						bgsPackageDataLocation->pointer->rad = inSerializer->Read<uint32_t>();
						bgsPackageDataLocation->pointer->locType = inSerializer->Read<REX::EnumSet<RE::PackageLocation::Type, std::uint8_t>>();
						if(bgsPackageDataLocation->pointer->locType == RE::PackageLocation::Type::kNearReference)
						{
							bgsPackageDataLocation->pointer->data.refHandle = reinterpret_cast<RE::TESObjectREFR*>(inSerializer->ReadFormRef())->GetHandle();
						}
						//TODO other types
					}
					else if (packageDataTypeName == "Topic")
					{
						const auto bgsPackageTopicData = reinterpret_cast<RE::BGSPackageDataTopic*>(customPackageData->data.data[i]);
						bgsPackageTopicData->unk00 = 0;
						bgsPackageTopicData->unk08 = 0;
						bgsPackageTopicData->topic = reinterpret_cast<RE::TESTopic*>(inSerializer->ReadFormRef());
					}
					else if(packageDataTypeName == "SingleRef")
					{
						const auto* bgsPackageDataSingleRef = reinterpret_cast<RE::BGSPackageDataSingleRef*>(customPackageData->data.data[i]);
						bgsPackageDataSingleRef->pointer->targType = RE::PackageTarget::Type::kNearReference;
						bgsPackageDataSingleRef->pointer->target.handle = reinterpret_cast<RE::TESObjectREFR*>(inSerializer->ReadFormRef())->GetHandle();
					}
					else if(packageDataTypeName == "TargetSelector")
					{
						const auto* bgsPackageDataTargetSelector = reinterpret_cast<RE::BGSPackageDataTargetSelector*>(customPackageData->data.data[i]);
						bgsPackageDataTargetSelector->pointer->targType = RE::PackageTarget::Type::kObjectType;
						bgsPackageDataTargetSelector->pointer->target.objType = inSerializer->Read<REX::EnumSet<RE::PACKAGE_OBJECT_TYPE, std::uint32_t>>();
					}
					else if(packageDataTypeName == "Bool")
					{
						const auto bgsPackageDataBool = reinterpret_cast<RE::BGSPackageDataBool*>(customPackageData->data.data[i]);
						bgsPackageDataBool->data.i = inSerializer->Read<uint32_t>();
					}
					else if(packageDataTypeName == "Int")
					{
						const auto bgsPackageDataInt = reinterpret_cast<RE::BGSPackageDataInt*>(customPackageData->data.data[i]);
						bgsPackageDataInt->data.i = inSerializer->Read<uint32_t>();
					}
					else if(packageDataTypeName == "Float")
					{
						const auto bgsPackageDataFloat = reinterpret_cast<RE::BGSPackageDataFloat*>(customPackageData->data.data[i]);
						bgsPackageDataFloat->data.f = inSerializer->Read<float>();
					}
					break;
				}
			}
		}

		RE::TESConditionItem** conditionItemHolder = &outPackage->packConditions.head;
		while(inSerializer->Read<bool>())
		{
			*conditionItemHolder = DeserializeCondition(inSerializer);
			conditionItemHolder = &(*conditionItemHolder)->next;
		}
	}

	void SerializePackageData(DPF::FileWriter* inSerializer, const RE::TESPackage* inPackage)
	{
		const auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(inPackage->data);

		inSerializer->Write<uint16_t>(customPackageData->data.dataSize);
		for(uint16_t i = 0; i < customPackageData->data.dataSize; ++i)
		{
			inSerializer->Write<int8_t>(customPackageData->data.uids[i]);
			
			const auto packageDataTypeName = customPackageData->data.data[i]->GetTypeName();
			inSerializer->WriteString(packageDataTypeName.c_str());  
			if(packageDataTypeName == "Location")
			{
				const auto* bgsPackageDataLocation = reinterpret_cast<RE::BGSPackageDataLocation*>(customPackageData->data.data[i] - 1); // NOLINT(clang-diagnostic-reinterpret-base-class, bugprone-pointer-arithmetic-on-polymorphic-object)
				inSerializer->Write<uint32_t>(bgsPackageDataLocation->pointer->rad);
				inSerializer->Write<REX::EnumSet<RE::PackageLocation::Type, std::uint8_t>>(bgsPackageDataLocation->pointer->locType);
				if(bgsPackageDataLocation->pointer->locType == RE::PackageLocation::Type::kNearReference)
				{
					inSerializer->WriteFormRef(bgsPackageDataLocation->pointer->data.refHandle.get().get());
				}
				//TODO other types
			}
			else if (packageDataTypeName == "Topic")
			{
				const auto bgsPackageTopicData = reinterpret_cast<RE::BGSPackageDataTopic*>(customPackageData->data.data[i]);
				inSerializer->WriteFormRef(bgsPackageTopicData->topic);
			}
			else if(packageDataTypeName == "SingleRef")
			{
				const auto* bgsPackageDataSingleRef = reinterpret_cast<RE::BGSPackageDataSingleRef*>(customPackageData->data.data[i]);
				inSerializer->WriteFormRef(bgsPackageDataSingleRef->pointer->target.handle.get().get());
			}
			else if(packageDataTypeName == "TargetSelector")
			{
				const auto* bgsPackageDataTargetSelector = reinterpret_cast<RE::BGSPackageDataTargetSelector*>(customPackageData->data.data[i]);
				inSerializer->Write<REX::EnumSet<RE::PACKAGE_OBJECT_TYPE, std::uint32_t>>(bgsPackageDataTargetSelector->pointer->target.objType);
			}
			else if(packageDataTypeName == "Bool")
			{
				const auto bgsPackageDataBool = reinterpret_cast<RE::BGSPackageDataBool*>(customPackageData->data.data[i]);
				inSerializer->Write<uint32_t>(bgsPackageDataBool->data.i);
			}
			else if(packageDataTypeName == "Int")
			{
				const auto bgsPackageDataInt = reinterpret_cast<RE::BGSPackageDataInt*>(customPackageData->data.data[i]);
				inSerializer->Write<uint32_t>(bgsPackageDataInt->data.i);
			}
			else if(packageDataTypeName == "Float")
			{
				const auto bgsPackageDataFloat = reinterpret_cast<RE::BGSPackageDataFloat*>(customPackageData->data.data[i]);
				inSerializer->Write<float>(bgsPackageDataFloat->data.f);
			}
		}

		RE::TESConditionItem* const* conditionItemHolder = &inPackage->packConditions.head;
		while(*conditionItemHolder)
		{
			inSerializer->Write<bool>(true);
			SerializeCondition(inSerializer, *conditionItemHolder);
			conditionItemHolder = &(*conditionItemHolder)->next;
		}
		inSerializer->Write<bool>(false);
	}
}
