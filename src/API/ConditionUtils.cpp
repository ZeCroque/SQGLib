#include "SQG/API/ConditionUtils.h"

#include "DPF/FileSystem.h"

namespace SQG
{
	RE::TESConditionItem* CreateCondition(void* inParameter, const REX::EnumSet<RE::FUNCTION_DATA::FunctionID, std::uint16_t>& inFunction, const RE::CONDITION_ITEM_DATA::OpCode inOpCode, const RE::CONDITION_ITEM_DATA::GlobalOrFloat inComparisonValue)
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

	bool CheckCondition(RE::TESObjectREFR* inCaller, const std::list<RE::TESConditionItem*>& inConditions)
	{
		RE::TESCondition condition;
		RE::TESConditionItem** conditionItemHolder = &condition.head;
		for(const auto* conditionItem : inConditions)
		{
			*conditionItemHolder = new RE::TESConditionItem();
			(*conditionItemHolder)->data = conditionItem->data;
			conditionItemHolder = &((*conditionItemHolder)->next);
		}
		*conditionItemHolder = nullptr;

		return condition(inCaller, inCaller);
	}

	RE::TESConditionItem* DeserializeCondition(DPF::FileReader* inSerializer)
	{
		const auto function = inSerializer->Read<REX::EnumSet<RE::FUNCTION_DATA::FunctionID, std::uint16_t>>();
		const auto param = inSerializer->ReadFormRef();
		const auto opCode = inSerializer->Read<RE::CONDITION_ITEM_DATA::OpCode>();
		const auto comparisonValue = inSerializer->Read<RE::CONDITION_ITEM_DATA::GlobalOrFloat>();

		return CreateCondition(param, function, opCode, comparisonValue);
	}

	void SerializeCondition(DPF::FileWriter* inSerializer, const RE::TESConditionItem* inCondition)
	{
		inSerializer->Write<REX::EnumSet<RE::FUNCTION_DATA::FunctionID, std::uint16_t>>(inCondition->data.functionData.function);
		inSerializer->WriteFormRef(static_cast<RE::TESForm*>(inCondition->data.functionData.params[0]));
		inSerializer->Write<RE::CONDITION_ITEM_DATA::OpCode>(inCondition->data.flags.opCode);
		inSerializer->Write<RE::CONDITION_ITEM_DATA::GlobalOrFloat>(inCondition->data.comparisonValue);
	}
}
