#pragma once

#include "API.h"

namespace SQG
{
	SQG_API RE::TESConditionItem* CreateCondition(void* inParameter, const REX::EnumSet<RE::FUNCTION_DATA::FunctionID, std::uint16_t>& inFunction, RE::CONDITION_ITEM_DATA::OpCode inOpCode, RE::CONDITION_ITEM_DATA::GlobalOrFloat inComparisonValue);

	SQG_API bool CheckCondition(RE::TESObjectREFR* inCaller, const std::list<RE::TESConditionItem*>& inConditions);
}
