#pragma once

namespace SQG::Engine::Dialog
{
	// # Hooks
	// =======================
	void RegisterHooks(SKSE::Trampoline& inTrampoline);

	// # Sinks
	// =======================
	extern RE::TESTopic* forceGreetTopic;

	void RegisterSinks();

	// # Data
	// =======================
	void LoadData(RE::TESDataHandler* inDataHandler);
}
