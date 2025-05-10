#pragma once

#include "FileSystem.h"
#include "FormRecordSerializer.h"

void Init()
{
    firstFormId = lastFormId = ReadFirstFormIdFromESP();
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