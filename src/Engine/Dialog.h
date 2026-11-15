#pragma once

namespace SQG::Engine::Dialog
{
	// # Common
	// =======================
	void Init();

	// # Hooks
	// =======================
	void RegisterHooks(SKSE::Trampoline& inTrampoline);

	// # Sinks
	// =======================
	void RegisterSinks();
}
