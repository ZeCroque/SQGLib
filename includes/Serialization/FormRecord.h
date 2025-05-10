#pragma once

class FormRecord {
    public:
	FormRecord() = default;

    static FormRecord CreateNew(RE::TESForm *_actualForm, RE::FormType formType, RE::FormID formId) {
        FormRecord result;
        result.actualForm = _actualForm;
        result.formType = formType;
        result.formId = formId;
        return result;
    }
    static FormRecord CreateDeleted(RE::FormID formId) {
        FormRecord result;
        result.formId = formId;
        result.deleted = true;
        return result;
    }
    void Undelete(RE::TESForm *_actualForm, RE::FormType _formType) {
        if (!_actualForm) {
            return;
        }
        actualForm = _actualForm;
        formType = _formType;
        deleted = false;
    }

    bool HasModel() {
        return modelForm != nullptr; 
    }
    RE::TESForm *baseForm = nullptr;
    RE::TESForm *actualForm = nullptr;
    RE::TESForm *modelForm = nullptr;
    bool deleted = false;
    RE::FormType formType;
    RE::FormID formId;

};
