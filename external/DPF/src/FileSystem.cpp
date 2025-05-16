#include "FileSystem.h"

#include "SKSETmp.h"

namespace DPF
{
	uint32_t dynamicModId = 0;

	int ReadFirstFormIdFromESP(const RE::FormID inFormId, const std::string& inPluginName)
	{
	    const auto dataHandler = RE::TESDataHandler::GetSingleton();

	    auto id = dataHandler->LookupForm(inFormId, inPluginName);

	    if (!id){
	        //To make sure data stay intact when reloading without quitting, a valid mod index must be provided. Here we fallback to FF which is regular dynamic forms: it will cause issues.
	        assert(true);
	        return 0xFF >> 24;
	    }

	    dynamicModId = (id->formID >> 24) & 0xff;
	    return id->formID + 1;
	}

	void StreamWrapper::Clear() {
	    stream.str("");
	    stream.clear();
	    stream.seekg(0);
	}

	void StreamWrapper::SeekBeginning() {
	    stream.seekg(0);
	}

	uint32_t StreamWrapper::Size() {
	    std::streampos currentPosition = stream.tellg();
	    stream.seekg(0, std::ios::end);
	    size_t size = stream.tellg();
	    stream.seekg(currentPosition);
	    return static_cast<uint32_t>(size);
	}

	void StreamWrapper::WriteDown(std::function<void(uint32_t)> const& start, std::function<void(char)> const& step) {
	    auto size = Size();
	    start(size);
	    SeekBeginning();
	    for (size_t i = 0; i < size; i++) {
	        step(static_cast<char>(stream.get()));
	    }
	    Clear();
	}

	void StreamWrapper::ReadOut(std::function<uint32_t()> start, std::function<char()> const& step) {
	    Clear();
	    uint32_t arrayLength = start();
	    for (size_t i = 0; i < arrayLength; i++) {
	        stream.put(step());
	    }
	    SeekBeginning();
	}

	FileWriter::FileWriter(const std::string& filename, std::ios_base::openmode _Mode) {
	    fileStream.open(MakeSavePath(filename), _Mode);
	    if (!fileStream.is_open()) {
	    }
	}
	FileWriter::~FileWriter() {
	    if (fileStream.is_open()) {
	        fileStream.close();
	    }
	}

	bool FileWriter::IsOpen() { return fileStream.is_open(); }

	FileReader::FileReader(const std::string& filename, std::ios_base::openmode _Mode) {
	    fileStream.open(MakeSavePath(filename), _Mode);
	    if (!fileStream.is_open()) {
	    }
	}
	FileReader::~FileReader() {
	    if (fileStream.is_open()) {
	        fileStream.close();
	    }
	}

	bool FileReader::IsOpen() { return fileStream.is_open(); }

	void Delete(std::string name)
	{
	    std::remove(MakeSavePath(name).c_str());
	}
}