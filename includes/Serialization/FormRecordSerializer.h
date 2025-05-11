#pragma once

template<class T> class Serializer;
class FormRecord;

template <typename T> static void StoreFormRecordData(Serializer<T>* serializer, FormRecord& instance);

template <typename T> static void StoreFormRecord(Serializer<T>* serializer, FormRecord& instance);

template <typename T> void StoreAllFormRecords(Serializer<T>* serializer);

template <typename T> static void RestoreFormRecordData(Serializer<T>* serializer, FormRecord& instance);

template <typename T> static bool RestoreFormRecord(Serializer<T>* serializer, uint32_t i);

template <typename T> bool RestoreAllFormRecords(Serializer<T>* serializer);

#include "Serialization/FormRecordSerializer.hpp"