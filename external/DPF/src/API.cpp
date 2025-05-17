#include "DPF/API.h"

#include "FormCreator.h"
#include "FileSystem.h"
#include "FormRecordSerializer.h"

namespace DPF
{
	void Init(RE::FormID inFormId, const std::string& inPluginName)
	{
	    firstFormId = lastFormId = ReadFirstFormIdFromESP(inFormId, inPluginName);
	}

	void SaveCache(const SKSE::MessagingInterface::Message* inMessage) {
		std::string name = static_cast<char*>(inMessage->data);
		name = name.append(".sqg");
	    FileWriter fileWriter(name, std::ios::out | std::ios::binary | std::ios::trunc);

	    if (!fileWriter.IsOpen()) {
	        return;
	    }

	    StoreAllFormRecords(&fileWriter);
	}

	void LoadCache(const SKSE::MessagingInterface::Message* inMessage) {
		std::string name = static_cast<char*>(inMessage->data);
		name = name.substr(0, name.size() - 3).append("sqg");
		FileReader fileReader(name, std::ios::in | std::ios::binary);

	    if (!fileReader.IsOpen()) {
	        return;
	    }
	    RestoreAllFormRecords(&fileReader);

	    UpdateId();
	}

	void DeleteCache(const SKSE::MessagingInterface::Message* inMessage) {
		std::string name = static_cast<char*>(inMessage->data);
		name = name.append(".sqg");
	    Delete(name);
	}

	RE::TESForm* CreateForm(RE::TESForm* inModelForm)
	{
		return AddForm(inModelForm);
	}
}