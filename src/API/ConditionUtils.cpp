#include "SQG/API/ConditionUtils.h"

#include "SQG/API/Data.h"

namespace SQG
{
	RE::TESConditionItem* CreateCondition(void* inParameter, const REX::EnumSet<RE::FUNCTION_DATA::FunctionID, std::uint16_t>& inFunction, const RE::CONDITION_ITEM_DATA::OpCode inOpCode, const RE::CONDITION_ITEM_DATA::GlobalOrFloat inComparisonValue)
	{
		auto* result = DataManager::GetSingleton()->conditions.emplace_back(std::make_unique<RE::TESConditionItem>()).get();
		result->data.dataID = std::numeric_limits<std::uint32_t>::max();
		result->data.functionData.function = inFunction;
		result->data.functionData.params[0] = inParameter;
		result->data.flags.opCode = inOpCode;
		result->data.comparisonValue = inComparisonValue;
		result->next = nullptr;
		return result;
	}

	RE::TESConditionItem* CloneCondition(const RE::TESConditionItem* inCondition)
	{
		auto* result = new RE::TESConditionItem();
		result->data = inCondition->data;
		result->next = nullptr;
		return result;
	}

	bool CheckCondition(RE::TESObjectREFR* inCaller, const std::list<RE::TESConditionItem*>& inConditions)
	{
		RE::TESCondition condition;
		RE::TESConditionItem** conditionItemHolder = &condition.head;
		for(const auto* conditionItem : inConditions)
		{
			*conditionItemHolder = CloneCondition(conditionItem);
			conditionItemHolder = &((*conditionItemHolder)->next);
		}
		*conditionItemHolder = nullptr;

		return condition(inCaller, inCaller);
	}
}
