#include "DPF/API.h"

#include "FormCreator.h"
#include "FileSystem.h"
#include "FormRecordSerializer.h"

void Init(RE::FormID inFormId, const std::string& inPluginName)
{
    firstFormId = lastFormId = ReadFirstFormIdFromESP(inFormId, inPluginName);
}

void SaveCache(std::string name) {

    FileWriter fileWriter(name, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!fileWriter.IsOpen()) {
        return;
    }

    StoreAllFormRecords(&fileWriter);
}

void LoadCache(std::string name) {
    FileReader fileReader(name, std::ios::in | std::ios::binary);

    if (!fileReader.IsOpen()) {
        return;
    }
    RestoreAllFormRecords(&fileReader);

    UpdateId();
}

void DeleteCache(std::string name) {
    Delete(name);
}

RE::TESForm* CreateForm(RE::TESForm* inModelForm)
{
	return AddForm(inModelForm);
}