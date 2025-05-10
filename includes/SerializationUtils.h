#pragma once

#include <cstdint>
#include <shlobj_core.h>

#include <unordered_set>

namespace SQG
{
    RE::TESQuest* deserializedQuest = nullptr;
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

#ifndef __SKSE_VERSION_H__
#define __SKSE_VERSION_H__

// these have to be macros so they can be used in the .rc
#define SKSE_VERSION_INTEGER		2
#define SKSE_VERSION_INTEGER_MINOR	2
#define SKSE_VERSION_INTEGER_BETA	6
#define SKSE_VERSION_VERSTRING		"0, 2, 2, 6"
#define SKSE_VERSION_PADDEDSTRING	"0001"
#define SKSE_VERSION_RELEASEIDX		72

#define MAKE_EXE_VERSION_EX(major, minor, build, sub)	((((major) & 0xFF) << 24) | (((minor) & 0xFF) << 16) | (((build) & 0xFFF) << 4) | ((sub) & 0xF))
#define MAKE_EXE_VERSION(major, minor, build)			MAKE_EXE_VERSION_EX(major, minor, build, 0)

#define GET_EXE_VERSION_MAJOR(a)	(((a) & 0xFF000000) >> 24)
#define GET_EXE_VERSION_MINOR(a)	(((a) & 0x00FF0000) >> 16)
#define GET_EXE_VERSION_BUILD(a)	(((a) & 0x0000FFF0) >> 4)
#define GET_EXE_VERSION_SUB(a)		(((a) & 0x0000000F) >> 0)

#define RUNTIME_TYPE_BETHESDA	0
#define RUNTIME_TYPE_GOG		1
#define RUNTIME_TYPE_EPIC		2

#define RUNTIME_VERSION_1_1_47	MAKE_EXE_VERSION(1, 1, 47)			// 0x010102F0	initial version released on steam, has edit-and-continue enabled
#define RUNTIME_VERSION_1_1_51	MAKE_EXE_VERSION(1, 1, 51)			// 0x01010330	initial version released on steam, has edit-and-continue enabled
#define RUNTIME_VERSION_1_2_36	MAKE_EXE_VERSION(1, 2, 36)			// 0x01020240	edit-and-continue disabled
#define RUNTIME_VERSION_1_2_39	MAKE_EXE_VERSION(1, 2, 39)			// 0x01020270	edit-and-continue disabled
#define RUNTIME_VERSION_1_3_5	MAKE_EXE_VERSION(1, 3, 5)			// 0x01030050	
#define RUNTIME_VERSION_1_3_9	MAKE_EXE_VERSION(1, 3, 9)			// 0x01030090	
#define RUNTIME_VERSION_1_4_2	MAKE_EXE_VERSION(1, 4, 2)			// 0x01040020	
#define RUNTIME_VERSION_1_5_3	MAKE_EXE_VERSION(1, 5, 3)			// 0x01050030	creation club
#define RUNTIME_VERSION_1_5_16	MAKE_EXE_VERSION(1, 5, 16)			// 0x01050100	creation club cleanup (thanks)
#define RUNTIME_VERSION_1_5_23	MAKE_EXE_VERSION(1, 5, 23)			// 0x01050170	creation club
#define RUNTIME_VERSION_1_5_39	MAKE_EXE_VERSION(1, 5, 39)			// 0x01050270	creation club
#define RUNTIME_VERSION_1_5_50	MAKE_EXE_VERSION(1, 5, 50)			// 0x01050320	creation club
#define RUNTIME_VERSION_1_5_53	MAKE_EXE_VERSION(1, 5, 53)			// 0x01050350	creation club
#define RUNTIME_VERSION_1_5_62	MAKE_EXE_VERSION(1, 5, 62)			// 0x010503E0	creation club
#define RUNTIME_VERSION_1_5_73	MAKE_EXE_VERSION(1, 5, 73)			// 0x01050490	creation club
#define RUNTIME_VERSION_1_5_80	MAKE_EXE_VERSION(1, 5, 80)			// 0x01050500	creation club - no code or data differences
#define RUNTIME_VERSION_1_5_97	MAKE_EXE_VERSION(1, 5, 97)			// 0x01050610	creation club
#define RUNTIME_VERSION_1_6_317	MAKE_EXE_VERSION(1, 6, 317)			// 0x010613D0	anniversary edition
#define RUNTIME_VERSION_1_6_318	MAKE_EXE_VERSION(1, 6, 318)			// 0x010613E0
#define RUNTIME_VERSION_1_6_323	MAKE_EXE_VERSION(1, 6, 323)			// 0x01061430
#define RUNTIME_VERSION_1_6_342	MAKE_EXE_VERSION(1, 6, 342)			// 0x01061560
#define RUNTIME_VERSION_1_6_353	MAKE_EXE_VERSION(1, 6, 353)			// 0x01061610
#define RUNTIME_VERSION_1_6_629	MAKE_EXE_VERSION(1, 6, 629)			// 0x01062750	to be hotfixed
#define RUNTIME_VERSION_1_6_640	MAKE_EXE_VERSION(1, 6, 640)			// 0x01062800	the hotfix
#define RUNTIME_VERSION_1_6_659_GOG	MAKE_EXE_VERSION_EX(1, 6, 659, RUNTIME_TYPE_GOG)
																	// 0x01062931
#define RUNTIME_VERSION_1_6_678_EPIC	MAKE_EXE_VERSION_EX(1, 6, 678, RUNTIME_TYPE_EPIC)
																	// 0x01062A62
#define RUNTIME_VERSION_1_6_1130	MAKE_EXE_VERSION(1, 6, 1130)	// 0x010646A0	creations patch
#define RUNTIME_VERSION_1_6_1170	MAKE_EXE_VERSION(1, 6, 1170)	// 0x01064920
#define RUNTIME_VERSION_1_6_1170_GOG	MAKE_EXE_VERSION_EX(1, 6, 1170, RUNTIME_TYPE_GOG)
																	// 0x01064921	same version number as steam, possible problem

#define PACKED_SKSE_VERSION		MAKE_EXE_VERSION(SKSE_VERSION_INTEGER, SKSE_VERSION_INTEGER_MINOR, SKSE_VERSION_INTEGER_BETA)

// information about the state of the game at the time of release
#define SKSE_TARGETING_BETA_VERSION	0
#define CURRENT_RELEASE_RUNTIME		RUNTIME_VERSION_1_6_1170
#define CURRENT_RELEASE_SKSE_STR	"2.2.6"

#if GET_EXE_VERSION_SUB(RUNTIME_VERSION) == RUNTIME_TYPE_BETHESDA
#define SAVE_FOLDER_NAME "Skyrim Special Edition"
#elif GET_EXE_VERSION_SUB(RUNTIME_VERSION) == RUNTIME_TYPE_GOG
#define SAVE_FOLDER_NAME "Skyrim Special Edition GOG"
#elif GET_EXE_VERSION_SUB(RUNTIME_VERSION) == RUNTIME_TYPE_EPIC
#define SAVE_FOLDER_NAME "Skyrim Special Edition EPIC"
#else
#error unknown runtime type
#endif

#endif /* __SKSE_VERSION_H__ */


const char * kSavegamePath = "\\My Games\\" SAVE_FOLDER_NAME "\\";

std::string MakeSavePath(std::string name)
{
	char	path[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, path);

	std::string	result = path;
	result += kSavegamePath;
	RE::Setting* localSavePath = RE::GetINISetting("sLocalSavePath:General");
	if(localSavePath && (localSavePath->GetType() == RE::Setting::Type::kString))
		result += localSavePath->data.s;
	else
		result += "Saves\\";

	result += "\\";
	result += name;
    return result;
}

class FileWriter : public Serializer<FileWriter> {
private:
    std::ofstream fileStream;

public:
    FileWriter(const std::string& filename, std::ios_base::openmode _Mode = std::ios_base::out) {
        fileStream.open(MakeSavePath(filename), _Mode);
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
        fileStream.open(MakeSavePath(filename), _Mode);
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
        SQG::deserializedQuest = reinterpret_cast<RE::TESQuest*>(instance->actualForm);
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

void SaveCache(std::string name) {

    auto fileWriter = new FileWriter(name, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!fileWriter->IsOpen()) {
        return;
    }

    StoreAllFormRecords(fileWriter);

    delete fileWriter;

}

void LoadCache(std::string name) {
    auto fileReader = new FileReader(name, std::ios::in | std::ios::binary);

    if (!fileReader->IsOpen()) {
        return;
    }
    RestoreAllFormRecords(fileReader);

    UpdateId();

    delete fileReader;

}