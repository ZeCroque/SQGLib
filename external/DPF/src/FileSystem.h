#pragma once

extern uint32_t dynamicModId;

int ReadFirstFormIdFromESP(RE::FormID inFormId, const std::string& inPluginName);

class StreamWrapper {
    private:
    std::stringstream stream;
    public:
        void Clear();
        void SeekBeginning();
        template <class T>
        void Write(T value);
        template <class T>
        T Read();
        uint32_t Size();

        void WriteDown(std::function<void(uint32_t)> const& start, std::function<void(char)> const& step);
        void ReadOut(std::function<uint32_t()> start, std::function<char()> const& step);
};



template <typename Derived>
class Serializer {
private:
    std::stack<StreamWrapper*> ctx;

    template <class T> void WriteTarget(T value);
    template <class T> T ReadSource();

protected:
    bool error = false;
public:

    ~Serializer();

    template <class T>
    void Write(T value);
    
    template <class T>
    T Read();

    void StartWritingSection();

    void finishWritingSection();

    void startReadingSection();

    void finishReadingSection();

    template<class T>
    T* ReadFormRef();

    RE::TESForm* ReadFormRef();

    void WriteFormRef(RE::TESForm* item);

    RE::FormID ReadFormId();

    void WriteFormId(RE::FormID formId);

    std::string ReadString();

    void WriteString(const char* string);
};

class FileWriter : public Serializer<FileWriter> {
private:
    std::ofstream fileStream;

public:
    FileWriter(const std::string& filename, std::ios_base::openmode _Mode = std::ios_base::out);
    ~FileWriter();
    bool IsOpen();

    template <class T>
    T ReadImplementation();

    template <class T>
    void WriteImplementation(T value);
};

class FileReader : public Serializer<FileReader> {
private:
    std::ifstream fileStream;

public:
    FileReader(const std::string& filename, std::ios_base::openmode _Mode = std::ios_base::in);
    ~FileReader();
    bool IsOpen();

    template <class T>
    void WriteImplementation(T);
    template <class T>
    T ReadImplementation();
};

void Delete(std::string name);

#include "FileSystem.hpp"