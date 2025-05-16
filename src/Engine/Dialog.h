#pragma once

namespace SQG
{
	namespace DialogEngine
	{
		// # Hooks
		// =======================
		void RegisterHooks(SKSE::Trampoline& inTrampoline);

		// # Sinks
		// =======================
		void RegisterSinks();

		// # Data
		// =======================
		void LoadData(RE::TESDataHandler* inDataHandler);
	}
}