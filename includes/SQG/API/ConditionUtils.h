#pragma once

namespace DPF
{
	class FileWriter;
	class FileReader;
}

namespace SQG
{
	RE::TESConditionItem* CreateCondition(void* inParameter, const REX::EnumSet<RE::FUNCTION_DATA::FunctionID, std::uint16_t>& inFunction, RE::CONDITION_ITEM_DATA::OpCode inOpCode, RE::CONDITION_ITEM_DATA::GlobalOrFloat inComparisonValue);

	bool CheckCondition(RE::TESObjectREFR* inCaller, const std::list<RE::TESConditionItem*>& inConditions);

	RE::TESConditionItem* DeserializeCondition(DPF::FileReader* inSerializer);

	void SerializeCondition(DPF::FileWriter* inSerializer, const RE::TESConditionItem* inCondition);
}
