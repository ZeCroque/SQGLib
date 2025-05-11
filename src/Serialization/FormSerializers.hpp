#pragma once

#include "Serialization/FormRecord.h"

REGISTER_SERIALIZERS(Quest, Name)

template<class T> void NameSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form) {
    auto name = form->As<RE::TESFullName>()->fullName.c_str();
    serializer->WriteString(name);
}
template<class T> void NameSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form) {
    auto name = serializer->ReadString();
    auto named = form->As<RE::TESFullName>();
    if (named) {
        named->fullName = name;
    }
}
template<class T> bool NameSerializer<T>::Condition(RE::TESForm* form) { return form->As<RE::TESFullName>(); }


template<class T> void QuestSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form) {
    auto sourceQuest = form->As<RE::TESQuest>();
    if (sourceQuest) 
    {
         serializer->WriteString(sourceQuest->GetFormEditorID());
    }
}
template<class T> void QuestSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form) {
    auto target = form->As<RE::TESQuest>();
    if (target) 
    {
        target->SetFormEditorID(serializer->ReadString().c_str());
    }
}
template<class T> bool QuestSerializer<T>::Condition(RE::TESForm* form) { return form->As<RE::TESQuest>(); }

template <typename T> void StoreEachFormData(Serializer<T>* serializer, FormRecord& elem) {

    if (!serializer) {
        return;
    }
    auto& filters = GetFilters<T>();

    serializer->Write<uint32_t>(static_cast<uint32_t>(filters.size()));

    for (auto& filter : filters) {
        serializer->StartWritingSection();
        if (elem.actualForm && filter->Condition(elem.actualForm)) {
            serializer->Write<char>(1);
            filter->Serialize(serializer, elem.actualForm);
        } else {
            serializer->Write<char>(0);
        }
        serializer->finishWritingSection();
    }
}

template <typename T> void RestoreEachFormData(Serializer<T>* serializer, FormRecord& elem) {

     if (!serializer) {
         return;
     }

    auto& filters = GetFilters<T>();

     auto size = serializer->Read<uint32_t>();
     uint32_t i = 0;

     for (auto& filter : filters) {
         if (i >= size) {
             break;
         }
         serializer->startReadingSection();
         auto kind = serializer->Read<char>();
         if (kind == 1 && elem.actualForm) {
            filter->Deserialize(serializer, elem.actualForm);
         }
         serializer->finishReadingSection();
         ++i;
     }
 }