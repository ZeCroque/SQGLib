#include "Serialization/FormRecord.h"

FormRecord FormRecord::CreateNew(RE::TESForm *_actualForm, RE::FormType formType, RE::FormID formId) {
    FormRecord result;
    result.actualForm = _actualForm;
    result.formType = formType;
    result.formId = formId;
    return result;
}

FormRecord FormRecord::CreateDeleted(RE::FormID formId) {
    FormRecord result;
    result.formId = formId;
    result.deleted = true;
    return result;
}
void FormRecord::Undelete(RE::TESForm *_actualForm, RE::FormType _formType) {
    if (!_actualForm) {
        return;
    }
    actualForm = _actualForm;
    formType = _formType;
    deleted = false;
}

bool FormRecord::HasModel() {
    return modelForm != nullptr; 
}