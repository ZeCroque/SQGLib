#pragma once

namespace SQG
{
	namespace PackageEngine
	{
		// # Common
		// =======================
		extern RE::TESPackage* acquirePackage;	
		extern RE::TESPackage* activatePackage;
		extern RE::TESPackage* travelPackage;
		extern RE::TESPackage* forceGreetPackage;

		void InitAliasPackages(RE::FormID inQuestId);

		// # Sinks
		// =======================
		void RegisterSinks();

		// # Data
		// =======================
		void LoadData();
	}
}