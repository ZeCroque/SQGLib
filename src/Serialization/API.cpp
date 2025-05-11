#include "Serialization/API.h"

#include "Serialization/FileSystem.h"
#include "Serialization/FormRecordSerializer.h"

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
}

void DeleteCache(std::string name) {
    Delete(name);
}