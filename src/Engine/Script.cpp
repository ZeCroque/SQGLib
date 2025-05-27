#include "Script.h"

#include "common/FakeScripts.h"
#include "common/parser/CapricaUserFlagsParser.h"

namespace SQG
{
	namespace ScriptEngine
	{
		// # Common
		// =======================
		std::map<std::string, caprica::papyrus::PapyrusCompilationNode*> compiledScripts;

		// # Script Store
		// =======================
		RE::BSTSmartPointer<Store> Store::store;
		RE::BSTSmartPointer<RE::BSScript::IStore> Store::baseStore;

		std::size_t Store::GetSize() const
		{
			if(readingCustomScript)
			{
				auto& data = compiledScripts[scriptName]->getData();
				std::size_t size = 0;
				for(auto it = data.begin(); it != data.end(); ++it)
				{
					size += it.size();
				}
				return size;
			}
			return baseStore->GetSize();
		}

		std::size_t Store::GetPosition() const
		{
			if(readingCustomScript)
			{
				return chainedPoolPos * 1024 * 4 + index;
			}
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
				const auto buf = reinterpret_cast<char*>(a_bytes);
				for(std::size_t i = 0; i < a_numBytes; ++i)
				{
					buf[i] = chainedPoolIt.data()[index];
					++index;
					if(index == chainedPoolIt.size())
					{
						index = 0;
						++chainedPoolIt;
						++chainedPoolPos;
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
			if(compiledScripts.contains(a_fileName))
			{
				chainedPoolIt = compiledScripts[a_fileName]->getData().begin();
				chainedPoolPos = 0;
				index = 0;
				scriptName = a_fileName;
				fakePath = "data\\SCRIPTS\\" + scriptName + ".pex";
				readingCustomScript = true;
				return true;
			}
			return baseStore->Open(a_fileName);
		}

		void Store::Close()
		{
			if(readingCustomScript)
			{
				readingCustomScript = false;
				return;
			}
			baseStore->Close();
		}

		const RE::BSFixedString& Store::GetRelPath()
		{
			if(readingCustomScript)
			{
				return fakePath;
			}
			return baseStore->GetRelPath();
		}

		bool Store::HasOpenFile()
		{
			if(readingCustomScript)
			{
				return true;
			}
			return baseStore->HasOpenFile();
		}

		bool Store::FileIsGood()
		{
			if(readingCustomScript)
			{
				return true;
			}
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

		// # Caprica
		// =======================
		bool AddFilesFromDirectory(const caprica::IInputFile& inFile, const std::filesystem::path& inOutputDir, caprica::CapricaJobManager* inJobManager, caprica::papyrus::PapyrusCompilationNode::NodeType inNodeType)
		{
			// Blargle flargle.... Using the raw Windows API is 5x
			// faster than boost::filesystem::recursive_directory_iterator,
			// at 40ms vs. 200ms for the boost solution, and the raw API
			// solution also gets us the relative paths, absolute paths,
			// last write time, and file size, all without any extra processing.
			if(!inFile.exists()) 
			{
				return false;
			}

			auto absBaseDir = inFile.resolved_absolute_basedir();
			auto subDir = inFile.resolved_relative();
			if(subDir == ".")
			{
				subDir = "";
			}
			bool recursive = inFile.isRecursive();
			std::vector<std::filesystem::path> dirs {};
			dirs.push_back(subDir);

			while(!dirs.empty()) 
			{
				HANDLE hFind;
				WIN32_FIND_DATA data;
				auto curDir = dirs.back();
				dirs.pop_back();
				auto curSearchPattern = absBaseDir / curDir / "*";
				caprica::caseless_unordered_identifier_ref_map<caprica::papyrus::PapyrusCompilationNode *> namespaceMap{};
				namespaceMap.reserve(8000);

				hFind = FindFirstFileA(curSearchPattern.string().c_str(), &data);
				if(hFind == INVALID_HANDLE_VALUE) 
				{
					return false;
				}

				do 
				{
					if(std::string_view filenameRef = data.cFileName; filenameRef != "." && filenameRef != "..") 
					{
						if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
						{
							if(recursive)
							{
								dirs.push_back(curDir / data.cFileName);
							}
						}
						else
						{
							ULARGE_INTEGER ull;
							ull.LowPart = data.ftLastWriteTime.dwLowDateTime;
							ull.HighPart = data.ftLastWriteTime.dwHighDateTime;
							std::time_t lastModTime = ull.QuadPart / 10000000ULL - 11644473600ULL;

							ull.LowPart = data.nFileSizeLow;
							ull.HighPart = data.nFileSizeHigh;
							auto fileSize = ull.QuadPart;

							const std::filesystem::path cur = curDir == "." ? "" : curDir;

							caprica::papyrus::PapyrusCompilationNode* node =  new caprica::papyrus::PapyrusCompilationNode
							(
								inJobManager,
								inNodeType,
								(cur / data.cFileName).string(),
								inOutputDir.string(),
								(absBaseDir / cur / data.cFileName).string(),
								lastModTime,
								fileSize,
								false
							);
							namespaceMap.emplace(caprica::identifier_ref(node->baseName), node);
						}
					}
				} while(FindNextFileA(hFind, &data));
				FindClose(hFind);

				caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents("", std::move(namespaceMap));
			}
			return true;
		}


		bool HandleImports(const std::vector<caprica::ImportDir>& inDirs, caprica::CapricaJobManager* inJobManager)
		{
			// Skyrim hacks; we need to import Skyrim's fake scripts into the global namespace first.
			static const std::unordered_set FAKE_SKYRIM_SCRIPTS_SET = 
			{
				"fake://skyrim/__ScriptObject.psc",
				"fake://skyrim/DLC1SCWispWallScript.psc"
			};
			caprica::caseless_unordered_identifier_ref_map<caprica::papyrus::PapyrusCompilationNode *> tempMap{};
			for(auto &fake_script: FAKE_SKYRIM_SCRIPTS_SET) 
			{
				auto basename = caprica::FSUtils::filenameAsRef(fake_script);
				auto node = new caprica::papyrus::PapyrusCompilationNode
				(
					inJobManager,
					caprica::papyrus::PapyrusCompilationNode::NodeType::PapyrusImport,
					std::string(basename),
					"",
					std::string(fake_script),
					0,
					caprica::FakeScripts::getSizeOfFakeScript(fake_script, caprica::conf::Papyrus::game),
					false
				);
				tempMap.emplace(caprica::identifier_ref(node->baseName), node);
			}
			caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents("", std::move(tempMap));

			return std::ranges::all_of(inDirs, [inJobManager](auto& dir)
			{
				return AddFilesFromDirectory(dir, "", inJobManager, caprica::papyrus::PapyrusCompilationNode::NodeType::PapyrusImport);
			});
		}

		void ParseUserFlags(const std::string& flagsPath)
		{
		  caprica::CapricaReportingContext reportingContext{flagsPath};
		  caprica::parser::CapricaUserFlagsParser(reportingContext, flagsPath).parseUserFlags(caprica::conf::Papyrus::userFlagsDefinition);
		}

		caprica::CapricaJobManager jobManager{};
		void Init()
		{
			caprica::conf::Papyrus::game = caprica::GameID::Skyrim;
			caprica::conf::Papyrus::importDirectories.emplace_back(caprica::FSUtils::canonical(R"(.\Data\Scripts\Source)"), false);
			if(!HandleImports(caprica::conf::Papyrus::importDirectories, &jobManager)) 
			{
				return;
			}
			ParseUserFlags(R"(.\Data\Scripts\Source\TESV_Papyrus_Flags.flg)");
			caprica::papyrus::PapyrusCompilationContext::awaitRead();

			Store::Init();
		}

		void  AddScript(const std::string& inStringName, const std::string_view& inData)
		{
			auto node = new caprica::papyrus::PapyrusCompilationNode
			(
                  &jobManager,
                  inStringName,
                  inData
			);
			compiledScripts[inStringName] = node;

			caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents
			(
				"",
				caprica::caseless_unordered_identifier_ref_map<caprica::papyrus::PapyrusCompilationNode *>{
			    {caprica::identifier_ref(node->baseName), node}
			});
		    caprica::papyrus::PapyrusCompilationContext::awaitCompile();
			caprica::papyrus::PapyrusCompilationContext::removeFromNamespace(node->baseName);
		}
	}
}
