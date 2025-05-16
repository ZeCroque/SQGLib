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

		// # Sinks
		// =======================
		void RegisterSinks();

		// # Data
		// =======================
		void LoadData();
	}
}