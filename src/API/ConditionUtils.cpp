#include "SQG/API/ConditionUtils.h"

namespace SQG
{
	RE::TESConditionItem* CreateCondition(void* inParameter, const REX::EnumSet<RE::FUNCTION_DATA::FunctionID, std::uint16_t>& inFunction, RE::CONDITION_ITEM_DATA::OpCode inOpCode, RE::CONDITION_ITEM_DATA::GlobalOrFloat inComparisonValue)
	{
		auto* result = new RE::TESConditionItem();
		result->data.dataID = std::numeric_limits<std::uint32_t>::max();
		result->data.functionData.function = inFunction;
		result->data.functionData.params[0] = inParameter;
		result->data.flags.opCode = inOpCode;
		result->data.comparisonValue = inComparisonValue;
		result->next = nullptr;
		return result;
	}
}
