#pragma once

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
inline void ApplyPattern(RE::TESForm* baseForm, RE::TESForm* newForm)
{
    newForm->Copy(baseForm);
    CopyComponent<RE::TESFullName>(baseForm, newForm);
}

template<class T> T* CreateForm(T* baseItem, RE::FormID formId)
{
    static_assert(std::is_base_of_v<RE::TESForm, T>);

	RE::TESForm* result = nullptr;
    if(std::ranges::any_of(formData | std::views::values, [&](FormRecord& item) {
        if (item.deleted) {
            auto factory = RE::IFormFactory::GetFormFactoryByType(baseItem->GetFormType());
            result = factory->Create();
            result->SetFormID(item.formId, false);
            item.Undelete(result, baseItem->GetFormType());
            item.baseForm = baseItem;
            ApplyPattern(baseItem, result);
            return true;
        }
        return false;
    }))
	{
        return result->As<T>();
    }

    auto factory = RE::IFormFactory::GetFormFactoryByType(baseItem->GetFormType());
	result = factory->Create();

    if(formId > 0)
    {
		result->SetFormID(formId, false);  
    }

    auto slot = FormRecord::CreateNew(result, baseItem->GetFormType(), result->formID);
    slot.baseForm = baseItem;
    ApplyPattern(baseItem, result);
    formData[result->formID] = slot;
    return result->As<T>();
}