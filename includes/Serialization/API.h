#pragma once

#include "FileSystem.h"
#include "FormRecordSerializer.h"

void Init()
{
    firstFormId = lastFormId = ReadFirstFormIdFromESP();
}

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