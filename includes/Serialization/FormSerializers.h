#pragma once

#include "FileSystem.h"
#include "FormRecord.h"

template <typename T> class FormSerializer {
public:
    virtual void Serialize(Serializer<T>*, RE::TESForm*) = 0;
    virtual void Deserialize(Serializer<T>*, RE::TESForm*) = 0;
    virtual bool Condition(RE::TESForm*) = 0;
    virtual ~FormSerializer() = default;
};

template <typename T>
class NameSerializer : public FormSerializer<T> {
public:
    void Serialize(Serializer<T>* serializer, RE::TESForm* form) override {
        auto name = form->As<RE::TESFullName>()->fullName.c_str();
        serializer->WriteString(name);
    }
    void Deserialize(Serializer<T>* serializer, RE::TESForm* form) override {
        auto name = serializer->ReadString();
        auto named = form->As<RE::TESFullName>();
        if (named) {
            named->fullName = name;
        }
    }
    bool Condition(RE::TESForm* form) override { return form->As<RE::TESFullName>(); }
};

template <typename T>
class QuestSerializer : public FormSerializer<T> {
public:
    void Serialize(Serializer<T>* serializer, RE::TESForm* form) override {
        auto sourceQuest = form->As<RE::TESQuest>();
        if (sourceQuest) 
        {
             serializer->WriteString(sourceQuest->GetFormEditorID());
        }
    }
    void Deserialize(Serializer<T>* serializer, RE::TESForm* form) override {
        auto target = form->As<RE::TESQuest>();
        if (target) 
        {
            target->SetFormEditorID(serializer->ReadString().c_str());
        }
    }
    bool Condition(RE::TESForm* form) override { return form->As<RE::TESQuest>(); }
};

#define GetFilters() \
    filters.push_back(std::make_unique <NameSerializer<T>>());\
    filters.push_back(std::make_unique <QuestSerializer<T>>());

template <typename T>
void StoreEachFormData(Serializer<T>* serializer, FormRecord& elem) {

    if (!serializer) {
        return;
    }

    std::vector<std::unique_ptr<FormSerializer<T>>> filters;

    GetFilters();

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

template <typename T>
void RestoreEachFormData(Serializer<T>* serializer, FormRecord& elem) {

     if (!serializer) {
         return;
     }

     std::vector<std::unique_ptr<FormSerializer<T>>> filters;

     GetFilters();

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