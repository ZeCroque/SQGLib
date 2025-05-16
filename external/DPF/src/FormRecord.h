#pragma once

namespace DPF
{
	class FormRecord {
	public:
		FormRecord() = default;

	    static FormRecord CreateNew(RE::TESForm *_actualForm, RE::FormType formType, RE::FormID formId);
	    static FormRecord CreateDeleted(RE::FormID formId);
	    void Undelete(RE::TESForm *_actualForm, RE::FormType _formType);
	    bool HasModel();

	    RE::TESForm *baseForm = nullptr;
	    RE::TESForm *actualForm = nullptr;
	    RE::TESForm *modelForm = nullptr;
	    bool deleted = false;
	    RE::FormType formType;
	    RE::FormID formId;
	};
}
