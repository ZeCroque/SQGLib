#pragma once

namespace SQG
{
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

	RE::TESPackage* CreatePackageFromTemplate(RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest, const std::unordered_map<std::string, PackageData>& inPackageDataMap, const std::list<RE::TESConditionItem*>& inConditions = std::list<RE::TESConditionItem*>());
}