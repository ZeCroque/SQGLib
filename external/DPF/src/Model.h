#pragma once

namespace DPF
{
	class FormRecord;

	extern uint32_t lastFormId;  // last mod
	extern uint32_t firstFormId;  // last mod

	extern std::map<RE::FormID, FormRecord> formData;

	void UpdateId();
}
