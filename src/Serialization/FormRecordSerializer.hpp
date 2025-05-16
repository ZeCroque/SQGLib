#pragma once

#include <Serialization/FormSerializers.h>
#include "Serialization/FormRecord.h"
#include "Serialization/Model.h"
#include "FormCreator.h"

template <typename T>
void StoreFormRecordData(Serializer<T>* serializer, FormRecord& instance) {
    serializer->StartWritingSection();
    serializer->Write<char>(instance.deleted ? 1 : 0);
    if (!instance.deleted) {
        StoreEachFormData(serializer, instance);
    }
    serializer->finishWritingSection();
}

template <typename T>
void StoreFormRecord(Serializer<T>* serializer, FormRecord& instance) {

    serializer->StartWritingSection();

    serializer->Write<char>(instance.deleted ? 1 : 0);

    if (!instance.deleted) {

        serializer->WriteFormRef(instance.baseForm);
        serializer->WriteFormRef(instance.modelForm);
        serializer->WriteFormId(instance.formId);
        
    } else {
        serializer->WriteFormId(instance.formId);
    }

    serializer->finishWritingSection();
}

template <typename T>
void StoreAllFormRecords(Serializer<T>* serializer) {

    size_t sizeData = formData.size();

    serializer->Write<uint32_t>(static_cast<uint32_t>(sizeData));

    std::ranges::for_each((formData | std::views::values), [&](FormRecord& item) {
        StoreFormRecord(serializer, item);
        return true;
    });

	std::ranges::for_each((formData | std::views::values), [&](FormRecord& item) {
        StoreFormRecordData(serializer, item);
        return true;
    });
}

template <typename T>
void RestoreFormRecordData(Serializer<T>* serializer, FormRecord& instance) {

    serializer->startReadingSection();

    auto deleted = serializer->Read<char>();

    if (deleted == 1) {
        serializer->finishReadingSection();
        return;
    }

    if (!instance.actualForm) {
        serializer->finishReadingSection();
        return; 
    }

    RestoreEachFormData(serializer, instance);

    serializer->finishReadingSection();
}

template <typename T>
bool RestoreFormRecord(Serializer<T>* serializer, uint32_t i) {
    serializer->startReadingSection();
    auto deleted = serializer->Read<char>();

    if (deleted == 1) {
        auto formId = serializer->ReadFormId();
        serializer->finishReadingSection();

        if (formData.contains(formId)) {
            formData[formId].deleted = true;
        } else {
            formData[formId] = FormRecord::CreateDeleted(formId);
        }
        
        return false;
    }

    auto baseForm = serializer->ReadFormRef();
    auto modelForm = serializer->ReadFormRef();
    auto id = serializer->ReadFormId();
    serializer->finishReadingSection();
    if(!formData.contains(id))
    {
        AddForm(baseForm, id);
    }
    else if(auto& instance = formData[id]; instance.formType != baseForm->GetFormType() || instance.deleted)
    {
	    auto factory = RE::IFormFactory::GetFormFactoryByType(baseForm->GetFormType());
        RE::TESForm* current = factory->Create();
        current->SetFormID(id, false);
        instance.Undelete(current, baseForm->GetFormType());
    }

    return true;
}

template <typename T>
bool RestoreAllFormRecords(Serializer<T>* serializer) {

    bool formRecordCreated = false;

    uint32_t sizeData = serializer->Read<uint32_t>();

    for (uint32_t i = 0; i < sizeData; i++) {
        if (RestoreFormRecord(serializer, i)) {
            formRecordCreated = true;
        }
    }

    for (auto instance : formData | std::views::values) {
        RestoreFormRecordData(serializer, instance);
    }

    return formRecordCreated;
}