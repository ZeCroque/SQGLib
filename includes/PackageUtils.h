#pragma once

namespace SQG
{
	// Base
	// =======================
	RE::TESPackage* CreatePackageFromTemplate(RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest);

	// PackageData
	// =======================
	union PackageData
	{
		using PackageNativeData = RE::BGSNamedPackageData<RE::IPackageData>::Data;

		PackageData();
		PackageData(RE::PackageLocation::Type inType, const RE::PackageLocation::Data& inData, std::uint32_t inRadius);
		PackageData(RE::PackageTarget::Type inType, const RE::PackageTarget::Target& inData);
		explicit PackageData(const RE::BGSNamedPackageData<RE::IPackageData>::Data& inData);
		~PackageData();
		PackageData& operator=(const PackageData& inOther);

		RE::PackageTarget targetData;
		RE::PackageLocation locationData;
		PackageNativeData nativeData{};	
	};

	void FillPackageData(const RE::TESPackage* outPackage, const std::unordered_map<std::string, PackageData>& inPackageDataMap); //TODO use uid

	// Conditions
	// =======================
	struct PackageConditionDescriptor
	{
		RE::FUNCTION_DATA::FunctionID functionId;
		RE::TESForm* functionCaller;
		RE::CONDITION_ITEM_DATA::OpCode opCode;
		bool useGlobal;
		RE::CONDITION_ITEM_DATA::GlobalOrFloat data;
		bool isOr;
	};

	void FillPackageCondition(RE::TESPackage* inPackage, const std::list<PackageConditionDescriptor>& packageConditionDescriptors);
}