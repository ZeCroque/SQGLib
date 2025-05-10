#pragma once

#include "FormCreator.h"

#include "Serialization/FormRecord.h"
#include "Serialization/Model.h"

template <class T> void CopyComponent(RE::TESForm* from, RE::TESForm* to)
{
    auto fromT = from->As<T>();
    auto toT = to->As<T>();
    if (fromT && toT) {
        toT->CopyComponent(fromT);
    }
}

//Copies component based on form type
void ApplyPattern(RE::TESForm* baseForm, RE::TESForm* newForm)
{
    newForm->Copy(baseForm);
    CopyComponent<RE::TESFullName>(baseForm, newForm);
}

RE::TESForm* CreateForm(RE::TESForm* baseItem, RE::FormID formId)
{
    RE::TESForm* result = nullptr;
    EachFormData([&](FormRecord* item) {
        if (item->deleted) {
            auto factory = RE::IFormFactory::GetFormFactoryByType(baseItem->GetFormType());
            result = factory->Create();
            result->SetFormID(item->formId, false);
            item->Undelete(result, baseItem->GetFormType());
            item->baseForm = baseItem;
            ApplyPattern(baseItem, result);
            return false;
        }
        return true;
    });

    if (result) {
        return result;
    }

    auto factory = RE::IFormFactory::GetFormFactoryByType(baseItem->GetFormType());
	result = factory->Create();

    RE::FormID newFormId;
    if(formId > 0)
    {
		newFormId = formId;   
    }
    else
    {
        newFormId = lastFormId;
	    ++lastFormId;
    }
	result->SetFormID(newFormId, false);
    auto slot = FormRecord::CreateNew(result, baseItem->GetFormType(), newFormId);
    slot->baseForm = baseItem;
    ApplyPattern(baseItem, result);
    formData[newFormId] = slot;
    return result;
}
