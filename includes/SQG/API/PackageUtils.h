#pragma once

namespace SQG
{
	union PackageData;

	void FillPackageWithTemplate(RE::TESPackage* outPackage, RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest);
	RE::TESPackage* CreatePackageFromTemplate(RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest, const std::unordered_map<std::string, PackageData>& inPackageDataMap, const std::list<RE::TESConditionItem*>& inConditions = std::list<RE::TESConditionItem*>());
}
