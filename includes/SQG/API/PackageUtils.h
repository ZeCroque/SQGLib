#pragma once

#include "API.h"

namespace SQG
{
	union PackageData;

	SQG_API void FillPackageWithTemplate(RE::TESPackage* outPackage, RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest);
	SQG_API RE::TESPackage* CreatePackageFromTemplate(RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest, const std::unordered_map<std::string, PackageData>& inPackageDataMap, const std::list<RE::TESConditionItem*>& inConditions = std::list<RE::TESConditionItem*>());
}
