#include "Script.h"

#include "papyrus/PapyrusCompilationContext.h"

namespace SQG
{
	namespace ScriptEngine
	{
		RE::BSTSmartPointer<Store> Store::store;
		std::ifstream Store::fileStream;
		RE::BSTSmartPointer<RE::BSScript::IStore> Store::baseStore;

		std::size_t Store::GetSize() const
		{
			//if(readingCustomScript)
			//{
			//	std::ifstream file("SQGDebug.pex", std::ios::binary | std::ios::ate);
			//	return file.tellg();
			//}
			return baseStore->GetSize();
		}
		std::size_t Store::GetPosition() const
		{
			//if(readingCustomScript)
			//{
			//	return fileStream.tellg();
			//}
			return baseStore->GetPosition();
		}

		RE::BSStorageDefs::ErrorCode Store::Seek(std::size_t a_offset, RE::BSStorageDefs::SeekMode a_seekMode) const
		{
			if(readingCustomScript)
			{
				///Unsupported
				return static_cast<RE::BSStorageDefs::ErrorCode>(8);
			}
			return baseStore->Seek(a_offset, a_seekMode);
		}

		RE::BSStorageDefs::ErrorCode Store::Read(std::size_t a_numBytes, std::byte* a_bytes) const
		{
			if(readingCustomScript)
			{
				//fileStream.read(reinterpret_cast<char*>(a_bytes), a_numBytes);
				auto buf = reinterpret_cast<char*>(a_bytes);

				for(auto i = 0; i < a_numBytes; ++i)
				{
					buf[i] = it.data()[index];
					++index;
					if(index == it.size())
					{
						index = 0;
						++it;
					}
				}
				return RE::BSStorageDefs::ErrorCode();
			}
			return baseStore->Read(a_numBytes, a_bytes);
		}

		RE::BSStorageDefs::ErrorCode Store::Write(std::size_t a_numBytes, const std::byte* a_bytes)
		{
			if(readingCustomScript)
			{
				///Unsupported
				return static_cast<RE::BSStorageDefs::ErrorCode>(8);
			}
			return baseStore->Write(a_numBytes, a_bytes);
		}

		bool Store::Open(const char* a_fileName)
		{
			if(!std::strcmp(a_fileName, "SQGDebug"))
			{
				it = caprica::papyrus::PapyrusCompilationNode::pexFiles.begin()->get()->strm.begin();
				//fileStream.open("SQGDebug.pex", std::ios::binary);
				//path = "data\\SCRIPTS\\SQGDebug.pex";
				readingCustomScript = true;
				return true;
			}
			return baseStore->Open(a_fileName);
		}

		void Store::Close()
		{
			if(readingCustomScript)
			{
				//fileStream.close();
				readingCustomScript = false;
				return;
			}
			baseStore->Close();
		}

		const RE::BSFixedString& Store::GetRelPath()
		{
			//if(readingCustomScript)
			//{
			//	return path;
			//}
			return baseStore->GetRelPath();
		}

		bool Store::HasOpenFile()
		{
			//if(readingCustomScript)
			//{
			//	return fileStream.is_open();
			//}
			return baseStore->HasOpenFile();
		}

		bool Store::FileIsGood()
		{
			//if(readingCustomScript)
			//{
			//	return fileStream.is_open();
			//}
			return baseStore->FileIsGood();
		}

		void Store::Unk_0B()
		{
			baseStore->Unk_0B();
		}

		void Store::Init()
		{
			if(const auto vm = RE::SkyrimVM::GetSingleton(); vm && vm->scriptStore)
			{
				baseStore = vm->scriptStore;
				GetCustomStore();
				vm->scriptLoader.SetScriptStore(store);
			}
		}

		Store* Store::GetCustomStore()
		{
			if(!store)
			{
				store = RE::make_smart<Store>();
			}
			return store.get();
		}
	}
}
