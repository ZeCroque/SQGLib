#pragma once

#include "API.h"

namespace SQG
{
	// The returned condition is owned by the library (valid until the next savegame load), don't free it. It can be shared by several dialog answers but must not be linked directly into a game's condition chain: use CloneCondition for that
	SQG_API RE::TESConditionItem* CreateCondition(void* inParameter, const REX::EnumSet<RE::FUNCTION_DATA::FunctionID, std::uint16_t>& inFunction, RE::CONDITION_ITEM_DATA::OpCode inOpCode, RE::CONDITION_ITEM_DATA::GlobalOrFloat inComparisonValue);

	// Creates a standalone copy (not chained to anything) of the given condition, meant to be linked into a chain owned by the game (like the conditions of a package, that the game frees itself). Parameters are copied as is, so conditions with a string parameter aren't supported
	SQG_API RE::TESConditionItem* CloneCondition(const RE::TESConditionItem* inCondition);

	SQG_API bool CheckCondition(RE::TESObjectREFR* inCaller, const std::list<RE::TESConditionItem*>& inConditions);
}
