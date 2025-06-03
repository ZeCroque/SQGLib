#pragma once

namespace SQG::Engine::Quest
{
	// # Common
	// =======================
	extern RE::TESQuest* referenceQuest;

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
