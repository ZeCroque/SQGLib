#pragma once

namespace SQG
{
	namespace QuestEngine
	{
		void RegisterHooks(SKSE::Trampoline& inTrampoline);

		// # Sinks
		// =======================

		// ## Quest stage end callbacks
		// =======================
		void RegisterSinks();
	}
}