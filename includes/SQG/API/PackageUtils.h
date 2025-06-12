#pragma once

namespace DPF
{
	class FileReader;
	class FileWriter;
}

namespace SQG
{
	void FillPackageWithTemplate(RE::TESPackage* outPackage, RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest);
	RE::TESPackage* CreatePackageFromTemplate(RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest, const std::unordered_map<std::string, PackageData>& inPackageDataMap, const std::list<RE::TESConditionItem*>& inConditions = std::list<RE::TESConditionItem*>());
	
	void DeserializePackageData(DPF::FileReader* inSerializer, RE::TESPackage* outPackage);
	void SerializePackageData(DPF::FileWriter* inSerializer, const RE::TESPackage* inPackage);
}
