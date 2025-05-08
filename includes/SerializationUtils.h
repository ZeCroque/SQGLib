#pragma once

#include <cstdint>

#include <unordered_set>

namespace SQG
{
	bool espFound = false;
	uint32_t lastFormId = 0;  // last mod
	uint32_t firstFormId = 0;  // last mod
	uint32_t dynamicModId = 0;

	void ReadFirstFormIdFromESP()
	{
	    const auto dataHandler = RE::TESDataHandler::GetSingleton();

	    auto id = dataHandler->LookupForm(0x800, "Dynamic Persistent Forms.esp");

	    if (id != 0) {
	        espFound = true;
	    }

	    firstFormId = id->formID + 1;
	    lastFormId = firstFormId;
	    dynamicModId = (firstFormId >> 24) & 0xff;
	}

	template <class T> void copyComponent(RE::TESForm* from, RE::TESForm* to)
	{
	    auto fromT = from->As<T>();
	    auto toT = to->As<T>();
	    if (fromT && toT) {
	        toT->CopyComponent(fromT);
	    }
	}
}

class FormRecord {
    private:
     FormRecord() {

    }
    FormRecord(RE::TESForm *_actualForm) {

        actualForm = _actualForm;
    }
    public:

    static FormRecord *CreateNew(RE::TESForm *_actualForm, RE::FormType formType, RE::FormID formId) {
        if (!_actualForm) {
            return nullptr;
        }
        auto result = new FormRecord(_actualForm);
        result->formType = formType;
        result->formId = formId;
        return result;
    }
    static FormRecord *CreateReference(RE::TESForm *_actualForm) {
        if (!_actualForm) {
            return nullptr;
        }
        auto result = new FormRecord(_actualForm);
        result->formId = _actualForm->GetFormID();
        return result;
    }
    static FormRecord *CreateDeleted(RE::FormID formId) {
        auto result = new FormRecord();
        result->formId = formId;
        result->deleted = true;
        return result;
    }
    void UndeleteReference(RE::TESForm *_actualForm) {
        if (!_actualForm) {
            return;
        }
        deleted = false;
        actualForm = _actualForm;
        formId = _actualForm->GetFormID();
        return;
    }
    void Undelete(RE::TESForm *_actualForm, RE::FormType _formType) {
        if (!_actualForm) {
            return;
        }
        actualForm = _actualForm;
        formType = _formType;
        deleted = false;
    }


    bool Match(RE::TESForm *matchForm) {
        if (!matchForm) {
            return false;
        }
        return matchForm->GetFormID() == formId;
    }

    bool Match(RE::FormID matchForm) {
        return matchForm == formId;
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

std::vector<FormRecord*> formData;
std::vector<FormRecord*> formRef;

void AddFormData(FormRecord* item) {
    if (!item) {
        return;
    }
    formData.push_back(item);
}

void AddFormRef(FormRecord* item) {
    if (!item) {
        return;
    }
    formRef.push_back(item);
}

void EachFormRef(std::function<bool(FormRecord*)> const& iteration) {
    for (auto item : formRef) {
        if (!iteration(item)) {
            return;
        }
    }
    return;
}

void EachFormData(std::function<bool(FormRecord*)> const& iteration) {
    for (auto item : formData) {
        if (!iteration(item)) {
            return;
        }
    }
    return;
}


static void applyPattern(FormRecord* instance) {

    auto baseForm = instance->baseForm;
    auto newForm = instance->actualForm;

    newForm->Copy(baseForm);
    SQG::copyComponent<RE::TESFullName>(baseForm, newForm);
}

class StreamWrapper {
    private:
    std::stringstream stream;
    public:
        void Clear() {
            stream.str("");
            stream.clear();
            stream.seekg(0);
        }
        void SeekBeginning() {
            stream.seekg(0);
        }
        template <class T>
        void Write(T value) {
            stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }
        template <class T>
        T Read() {
            T result;
            if (stream.read(reinterpret_cast<char*>(&result), sizeof(T))) {
                return result;
            }
            return T();
        }
        uint32_t Size() {
            std::streampos currentPosition = stream.tellg();
            stream.seekg(0, std::ios::end);
            size_t size = stream.tellg();
            stream.seekg(currentPosition);
            return static_cast<uint32_t>(size);
        }

        void WriteDown(std::function<void(uint32_t)> const& start, std::function<void(char)> const& step) {
            auto size = Size();
            start(size);
            SeekBeginning();
            for (size_t i = 0; i < size; i++) {
                step(static_cast<char>(stream.get()));
            }
            Clear();
        }
        void ReadOut(std::function<uint32_t()> start, std::function<char()> const& step) {
            Clear();
            uint32_t arrayLength = start();
            for (size_t i = 0; i < arrayLength; i++) {
                stream.put(step());
            }
            SeekBeginning();
        }
};

template <typename Derived>
class Serializer {
private:
    std::stack<StreamWrapper*> ctx;

    template <class T>
    void WriteTarget(T value) {
        static_cast<Derived*>(this)->template WriteImplementation<T>(value);
    }
    template <class T>
    T ReadSource() {
        return static_cast<Derived*>(this)->template ReadImplementation<T>();
    }

protected:
    bool error = false;
public:

    ~Serializer() {
        while (!ctx.empty()) {
            delete ctx.top();
            ctx.pop();      
        }
    }

    template <class T>
    void Write(T value) {
        if (!ctx.empty()) {
            ctx.top()->Write<T>(value);
        } else {
            WriteTarget<T>(value);
        }
    }
    template <class T>
    T Read() {
        if (!ctx.empty()) {
            return ctx.top()->Read<T>();
        } else {
            return ReadSource<T>();
        }
    }

    void StartWritingSection() {
        ctx.push(new StreamWrapper());
    }

    void finishWritingSection() {
        if (ctx.size() == 1) {
            auto body = ctx.top();
            ctx.pop();
            body->WriteDown(
                [&](uint32_t size) { WriteTarget<uint32_t>(size); }, 
                [&](char item) { WriteTarget<char>(item); }
            );
            delete body;
        } else if (ctx.size() > 1) {
            auto body = ctx.top();
            ctx.pop();
            body->WriteDown(
                [&](uint32_t size) { ctx.top()->Write<uint32_t>(size); }, 
                [&](char item) { ctx.top()->Write<char>(item); }
            );
            delete body;
        }
    }

    void startReadingSection() {

        if (ctx.size() == 0) {
            ctx.push(new StreamWrapper());
            ctx.top()->ReadOut(
                [&]() { return ReadSource<uint32_t>(); }, 
                [&]() { return ReadSource<char>(); }
            );
        } else {
            auto parent = ctx.top();
            ctx.push(new StreamWrapper());
            ctx.top()->ReadOut(
                [&]() { return parent->Read<uint32_t>(); }, 
                [&]() { return parent->Read<char>(); }
            );
        }
    }

    void finishReadingSection() {
        if (ctx.size() > 0) {
            auto top = ctx.top();
            ctx.pop();
            delete top;
        }
    }
    template<class T>
    T* ReadFormRef() {
        auto item = ReadFormId();
        if (item == 0) {
            return nullptr;
        }
        auto form = RE::TESForm::LookupByID(item);
        if (!form) {
            return nullptr;
        }
        return form->As<T>();
    }

    RE::TESForm* ReadFormRef() {
        auto item = ReadFormId();
        if (item == 0) {
            return nullptr;
        }
        return RE::TESForm::LookupByID(item);
    }

    void WriteFormRef(RE::TESForm* item) {
        if (item) {
            WriteFormId(item->GetFormID());
        } else {
            WriteFormId(0);
        }
    }

    RE::FormID ReadFormId() {
        const auto dataHandler = RE::TESDataHandler::GetSingleton();
        char fileRef = Read<char>();

        if (fileRef == 0) {
            return 0;
        }

        if (fileRef == 1) {
            uint32_t dynamicId = Read<uint32_t>();
            return dynamicId + (SQG::dynamicModId << 24);
        }
        else if(fileRef == 2){
            std::string fileName = ReadString();
            uint32_t localId = Read<uint32_t>();
            auto formId = dataHandler->LookupForm(localId, fileName)->formID;
            return formId;
        }

        return 0;


    }

    void WriteFormId(RE::FormID formId) {
        if (formId == 0) {
            Write<char>(0); 
            return;
        }

        const auto dataHandler = RE::TESDataHandler::GetSingleton();

        auto modId = (formId >> 24) & 0xff;

        if (modId == SQG::dynamicModId) {
            auto localId = formId & 0xFFFFFF;
            Write<char>(1);
            Write<uint32_t>(localId);
        }
        else if (modId == 0xfe) {
            auto lightId = (formId >> 12) & 0xFFF;
            auto file = dataHandler->LookupLoadedLightModByIndex(lightId);
            if (file) {
                auto localId = formId & 0xFFF;
                std::string fileName = file->fileName;
                Write<char>(2);
                WriteString(fileName.c_str());
                Write<uint32_t>(localId);
            } else {
                Write<char>(0);
            }
        } 
        else {
            auto file = dataHandler->LookupLoadedModByIndex(modId);
            if (file) {
                auto localId = formId & 0xFFFFFF;
                std::string fileName = file->fileName;
                Write<char>(2);
                WriteString(fileName.c_str());
                Write<uint32_t>(localId);
            } else {
                Write<char>(0);
            }
        }


    }

    char* ReadString() {
        size_t arrayLength = Read<uint32_t>();
        char* result = new char[arrayLength+1];
        for (size_t i = 0; i < arrayLength; i++) {
            result[i] = Read<char>();
        }
        result[arrayLength] = '\0';
        return result;
    }

    void WriteString(const char* string) {
        size_t arrayLength = strlen(string);
        Write<uint32_t>(static_cast<uint32_t>(arrayLength));
        for (size_t i = 0; i < arrayLength; i++) {
            char item = string[i];
            Write<char>(item);
        }
    }
};

class SaveDataSerializer : public Serializer<SaveDataSerializer> {
private:
    SKSE::SerializationInterface* a_intfc;
public:
    SaveDataSerializer(SKSE::SerializationInterface* _a_intfc) { a_intfc = _a_intfc; }

    template <class T>
    void WriteImplementation(T item) {
        a_intfc->WriteRecordData(item);
    }
    template <class T>
    T ReadImplementation() {
        T item;
        auto success = a_intfc->ReadRecordData(item);
        if (!success) {
            error = true;
        }
        return item;
    }
};

class FileWriter : public Serializer<FileWriter> {
private:
    std::ofstream fileStream;

public:
    FileWriter(const std::string& filename, std::ios_base::openmode _Mode = std::ios_base::out) {
        fileStream.open(filename, _Mode);
        if (!fileStream.is_open()) {
        }
    }
    ~FileWriter() {
        if (fileStream.is_open()) {
            fileStream.close();
        }
    }

    bool IsOpen() { return fileStream.is_open(); }

    template <class T>
    T ReadImplementation() {
        return T();
    }

    template <class T>
    void WriteImplementation(T value) {
        if (fileStream.is_open()) {
            fileStream.write(reinterpret_cast<const char*>(&value), sizeof(T));
        } else {
        }
    }
};

class FileReader : public Serializer<FileReader> {
private:
    std::ifstream fileStream;

public:
    FileReader(const std::string& filename, std::ios_base::openmode _Mode = std::ios_base::in) {
        fileStream.open(filename, _Mode);
        if (!fileStream.is_open()) {
        }
    }
    ~FileReader() {
        if (fileStream.is_open()) {
            fileStream.close();
        }
    }

    bool IsOpen() { return fileStream.is_open(); }

    template <class T>
    void WriteImplementation(T) {}
    template <class T>
    T ReadImplementation() {
        T value;
        if (fileStream.is_open()) {
            fileStream.read(reinterpret_cast<char*>(&value), sizeof(T));
            return value;
        } else {
        }
        return T();
    }
};

template <typename T>
void StoreAllFormRecords(Serializer<T>* serializer) {

    size_t sizeData = formData.size();
    size_t sizeRef = formRef.size();

    serializer->Write<uint32_t>(static_cast<uint32_t>(sizeData));
    serializer->Write<uint32_t>(static_cast<uint32_t>(sizeRef));

    EachFormData([&](FormRecord* instance) {
        StoreFormRecord(serializer, instance, false);
        return true;
    });

    EachFormData([&](FormRecord* elem) {
        StoreFormRecordData(serializer, elem);
        return true;
    });

    EachFormRef([&](FormRecord* instance) {
        StoreFormRecord(serializer, instance, true);
        return true;
    });


    EachFormRef([&](FormRecord* elem) {
        StoreFormRecordData(serializer, elem);
        return true;
    });
}
template <typename T>
bool RestoreAllFormRecords(Serializer<T>* serializer) {

    bool formRecordCreated = false;

    uint32_t sizeData = serializer->Read<uint32_t>();
    uint32_t sizeRef = serializer->Read<uint32_t>();

    for (uint32_t i = 0; i < sizeData; i++) {
        if (RestoreFormRecord(serializer, i, false)) {
            formRecordCreated = true;
        }
    }

    for (uint32_t i = 0; i < sizeData; ++i) {

        auto instance = formData[i];
        RestoreFormRecordData(serializer, instance);
        int z = 42;
    }


    for (uint32_t i = 0; i < sizeRef; i++) {

        if (RestoreFormRecord(serializer, i, true)) {
            formRecordCreated = true;
        }
    }

    for (uint32_t i = 0; i < sizeRef; ++i) {

        auto instance = formRef[i];
        RestoreFormRecordData(serializer, instance);
    }



    return formRecordCreated;
}



template <typename T>
static void StoreFormRecord(Serializer<T>* serializer, FormRecord* instance, bool reference) {

    serializer->StartWritingSection();

    serializer->Write<char>(instance->deleted ? 1 : 0);

    if (!instance->deleted) {
        if (reference) {
            serializer->WriteFormRef(instance->actualForm);
            serializer->WriteFormRef(instance->modelForm);
        } else {
            serializer->WriteFormRef(instance->baseForm);
            serializer->WriteFormRef(instance->modelForm);
            serializer->WriteFormId(instance->formId);
        }
    } else {
        serializer->WriteFormId(instance->formId);
    }

    serializer->finishWritingSection();
}

template <typename T>
static void StoreFormRecordData(Serializer<T>* serializer, FormRecord* instance) {
    serializer->StartWritingSection();
    serializer->Write<char>(instance->deleted ? 1 : 0);
    if (!instance->deleted) {
        StoreEachFormData(serializer, instance);
    }
    serializer->finishWritingSection();
}



template <typename T>
static bool RestoreFormRecord(Serializer<T>* serializer, uint32_t i, bool reference) {
    FormRecord* instance = nullptr;
    bool createdRecord = false;
    serializer->startReadingSection();
    auto deleted = serializer->Read<char>();

    if (deleted == 1) {
        auto formId = serializer->ReadFormId();
        serializer->finishReadingSection();
        if (reference) {
            if (i < formRef.size()) {
                instance = formRef[i];
                instance->deleted = true;
            } else {
                auto deletedInstance = FormRecord::CreateDeleted(formId);
                AddFormRef(deletedInstance);
            }
        } 
        else
        {
            if (i < formData.size()) {
                instance = formData[i];
                instance->deleted = true;
            } else {
                auto deletedInstance = FormRecord::CreateDeleted(formId);
                AddFormData(deletedInstance);
            }
        }
        return false;
    }

    if (reference) {
        if (i < formRef.size()) {
            instance = formRef[i];
        }
        createdRecord = RestoreModifiedItem(serializer, instance);
    } else {
        if (i < formData.size()) {
            instance = formData[i];
        }
        createdRecord = RestoreCreatedItem(serializer, instance);
    }

    return createdRecord;

}
template <typename T>
static bool RestoreModifiedItem(Serializer<T>* serializer, FormRecord* instance) {

    bool createdRecord = false;
    auto actualForm = serializer->ReadFormRef();
    auto modelForm = serializer->ReadFormRef();
    serializer->finishReadingSection();

    if (!actualForm) {
        if (!instance) {
            instance = FormRecord::CreateDeleted(0);
            AddFormRef(instance);
        } else {
            instance->deleted = true;
        }
        return false;
    }


    if (!instance) {
        instance = FormRecord::CreateReference(actualForm);
        instance->modelForm = modelForm;
        AddFormRef(instance);
        createdRecord = true;
    }


    instance->modelForm = modelForm;
    instance->deleted = false;
    instance->actualForm = actualForm;

    applyPattern(instance);


    return createdRecord;
}
template <typename T>
static bool RestoreCreatedItem(Serializer<T>* serializer, FormRecord* instance) {
    bool createdRecord = false;
    auto baseForm = serializer->ReadFormRef();
    auto modelForm = serializer->ReadFormRef();
    auto id = serializer->ReadFormId();
    serializer->finishReadingSection();

    if (!baseForm) {
        if (!instance) {
            instance = FormRecord::CreateDeleted(id);
            AddFormData(instance);
        } else {
            instance->deleted = true;
        }
        return false;
    }

    if (instance && (instance->formType != baseForm->GetFormType() || instance->deleted)) {
        if (instance->actualForm) {
            instance->actualForm->SetDelete(true);
        }
        auto factory = RE::IFormFactory::GetFormFactoryByType(baseForm->GetFormType());
        RE::TESForm* current = factory->Create();
        current->SetFormID(id, false);
        instance->Undelete(current, baseForm->GetFormType());
        createdRecord = true;
    }
    else if (!instance) {
        auto factory = RE::IFormFactory::GetFormFactoryByType(baseForm->GetFormType());
        RE::TESForm* current = factory->Create();
        current->SetFormID(id, false);
        instance = FormRecord::CreateNew(current, baseForm->GetFormType(), id);
        createdRecord = true;
        AddFormData(instance);
    }

    instance->baseForm = baseForm;
    instance->modelForm = modelForm;
    instance->formId = id;


    applyPattern(instance);


    return createdRecord;
}

template <typename T>
static void RestoreFormRecordData(Serializer<T>* serializer, FormRecord* instance) {

    serializer->startReadingSection();

    auto deleted = serializer->Read<char>();

    if (deleted == 1) {
        serializer->finishReadingSection();
        return;
    }

    if (!instance->actualForm) {
        serializer->finishReadingSection();
        return; 
    }

    RestoreEachFormData(serializer, instance);

    serializer->finishReadingSection();
}

void incrementLastFormID() {
    ++SQG::lastFormId;
}

void UpdateId() {
    EachFormData([&](FormRecord* item) {
        if (item->formId > SQG::lastFormId) {
	        SQG::lastFormId = item->formId;
        }
        return true;
    });
    incrementLastFormID();
}

template <typename T>
class FormSerializer {
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
        auto* name = serializer->ReadString();
        auto named = form->As<RE::TESFullName>();
        if (named && name) {
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
            target->SetFormEditorID(serializer->ReadString());
        }
    }
    bool Condition(RE::TESForm* form) override { return form->As<RE::TESQuest>(); }
};

#define GetFilters() \
    filters.push_back(std::make_unique <NameSerializer<T>>());\
    filters.push_back(std::make_unique <QuestSerializer<T>>());

template <typename T>
void StoreEachFormData(Serializer<T>* serializer, FormRecord* elem) {

    if (!elem || !serializer) {
        return;
    }

    std::vector<std::unique_ptr<FormSerializer<T>>> filters;

    GetFilters();

    serializer->Write<uint32_t>(static_cast<uint32_t>(filters.size()));

    for (auto& filter : filters) {
        serializer->StartWritingSection();
        if (elem->actualForm && filter->Condition(elem->actualForm)) {
            serializer->Write<char>(1);
            filter->Serialize(serializer, elem->actualForm);
        } else {
            serializer->Write<char>(0);
        }
        serializer->finishWritingSection();
    }
}

template <typename T>
void RestoreEachFormData(Serializer<T>* serializer, FormRecord* elem) {

     if (!elem || !serializer) {
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
         if (kind == 1 && elem->actualForm) {
            filter->Deserialize(serializer, elem->actualForm);
         }
         serializer->finishReadingSection();
         ++i;
     }
 }

std::mutex callbackMutext;

void LoadCache();
void SaveCache();

static void SaveCallback(SKSE::SerializationInterface* a_intfc) {
    std::lock_guard<std::mutex> lock(callbackMutext);
    try {
        if (!a_intfc->OpenRecord('ARR_', 1)) {
        } else {
            auto serializer = new SaveDataSerializer(a_intfc);
            StoreAllFormRecords(serializer);
        }
        SaveCache();
    } catch (const std::exception&) {
    }
}

static void LoadCallback(SKSE::SerializationInterface* a_intfc) {
    std::lock_guard<std::mutex> lock(callbackMutext);
    try {

        uint32_t type;
        uint32_t version;
        uint32_t length;
        bool refreshGame = false;

        while (a_intfc->GetNextRecordInfo(type, version, length)) {
            switch (type) {
                case 'ARR_': {
                    auto serializer = new SaveDataSerializer(a_intfc);
                    refreshGame = RestoreAllFormRecords(serializer);
                    delete serializer;
                } break;
                default:
                    break;
            }
        }
        if (refreshGame) {
            SaveCache();
            UpdateId();
            RE::PlayerCharacter::GetSingleton()->KillDying();
        }


    } catch (const std::exception&) {
    }
}
void SaveCache() {

    auto fileWriter = new FileWriter("DynamicPersistentFormsCache.bin", std::ios::out | std::ios::binary | std::ios::trunc);

    if (!fileWriter->IsOpen()) {
        return;
    }

    StoreAllFormRecords(fileWriter);

    delete fileWriter;

}

void LoadCache() {
    auto fileReader = new FileReader("DynamicPersistentFormsCache.bin", std::ios::in | std::ios::binary);

    if (!fileReader->IsOpen()) {
        return;
    }
    RestoreAllFormRecords(fileReader);

    UpdateId();

    delete fileReader;

}