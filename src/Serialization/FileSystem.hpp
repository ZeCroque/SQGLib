#pragma once

template <class T>
void StreamWrapper::Write(T value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <class T>
T StreamWrapper::Read() {
    T result;
    if (stream.read(reinterpret_cast<char*>(&result), sizeof(T))) {
        return result;
    }
    return T();
}

template <class Derived> template<class T>
void Serializer<Derived>::WriteTarget(T value) {
    static_cast<Derived*>(this)->template WriteImplementation<T>(value);
}

template <class Derived> template<class T>
T Serializer<Derived>::ReadSource() {
    return static_cast<Derived*>(this)->template ReadImplementation<T>();
}

template<class T> Serializer<T>::~Serializer() {
        while (!ctx.empty()) {
            delete ctx.top();
            ctx.pop();    
        }
    }

template <class Derived> template<class T>
    void Serializer<Derived>::Write(T value) {
        if (!ctx.empty()) {
            ctx.top()->Write<T>(value);
        } else {
            WriteTarget<T>(value);
        }
    }

template <class Derived> template<class T>
    T Serializer<Derived>::Read() {
        if (!ctx.empty()) {
            return ctx.top()->Read<T>();
        } else {
            return ReadSource<T>();
        }
    }

     template<class T> void Serializer<T>::StartWritingSection() {
        ctx.push(new StreamWrapper());
    }

    template<class T> void Serializer<T>::finishWritingSection() {
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

    template<class T> void Serializer<T>::startReadingSection() {

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

    template<class T> void Serializer<T>::finishReadingSection() {
        if (ctx.size() > 0) {
            auto top = ctx.top();
            ctx.pop();
            delete top;
        }
    }

	template <class Derived> template<class T>
    T* Serializer<Derived>::ReadFormRef() {
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

    template<class T> RE::TESForm* Serializer<T>::ReadFormRef() {
        auto item = ReadFormId();
        if (item == 0) {
            return nullptr;
        }
        return RE::TESForm::LookupByID(item);
    }

    template<class T> void Serializer<T>::WriteFormRef(RE::TESForm* item) {
        if (item) {
            WriteFormId(item->GetFormID());
        } else {
            WriteFormId(0);
        }
    }

    template<class T> RE::FormID Serializer<T>::ReadFormId() {
        const auto dataHandler = RE::TESDataHandler::GetSingleton();
        char fileRef = Read<char>();

        if (fileRef == 0) {
            return 0;
        }

        if (fileRef == 1) {
            uint32_t dynamicId = Read<uint32_t>();
            return dynamicId + (dynamicModId << 24);
        }
        else if(fileRef == 2){
            std::string fileName = ReadString();
            uint32_t localId = Read<uint32_t>();
            auto formId = dataHandler->LookupForm(localId, fileName)->formID;
            return formId;
        }

        return 0;


    }

    template<class T> void Serializer<T>::WriteFormId(RE::FormID formId) {
        if (formId == 0) {
            Write<char>(0); 
            return;
        }

        const auto dataHandler = RE::TESDataHandler::GetSingleton();

        auto modId = (formId >> 24) & 0xff;

        if (modId == dynamicModId) {
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

    template<class T> std::string Serializer<T>::ReadString() {
        size_t arrayLength = Read<uint32_t>();
        char* buf = new char[arrayLength+1];
        for (size_t i = 0; i < arrayLength; i++) {
            buf[i] = Read<char>();
        }
        buf[arrayLength] = '\0';
        std::string result = buf;
        delete buf;
        return result;
    }

    template<class T> void Serializer<T>::WriteString(const char* string) {
        size_t arrayLength = strlen(string);
        Write<uint32_t>(static_cast<uint32_t>(arrayLength));
        for (size_t i = 0; i < arrayLength; i++) {
            char item = string[i];
            Write<char>(item);
        }
    }

    template <class T>
    T FileWriter::ReadImplementation() {
        return T();
    }

    template <class T>
    void FileWriter::WriteImplementation(T value) {
        if (fileStream.is_open()) {
            fileStream.write(reinterpret_cast<const char*>(&value), sizeof(T));
        } else {
        }
    }

    template <class T>
    void FileReader::WriteImplementation(T) {}
    template <class T>
    T FileReader::ReadImplementation() {
        T value;
        if (fileStream.is_open()) {
            fileStream.read(reinterpret_cast<char*>(&value), sizeof(T));
            return value;
        } else {
        }
        return T();
    }