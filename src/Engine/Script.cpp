#include "Script.h"

#include "SQG/API/Data.h"
#include "common/FakeScripts.h"
#include "common/parser/CapricaUserFlagsParser.h"

namespace SQG::Engine::Script
{
	// # Script Store
	// =======================
	Store* Store::store;
	RE::BSScript::IStore* Store::baseStore;

	std::size_t Store::GetSize() const
	{
		if(readingCustomScript)
		{
			const auto& data = DataManager::GetSingleton()->compiledScripts[scriptName]->getData();
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

	RE::BSStorageDefs::ErrorCode Store::Seek(const std::size_t inOffset, const RE::BSStorageDefs::SeekMode inSeekMode) const
	{
		if(readingCustomScript)
		{
			///Unsupported
			return static_cast<RE::BSStorageDefs::ErrorCode>(8);
		}
		return baseStore->Seek(inOffset, inSeekMode);
	}

	RE::BSStorageDefs::ErrorCode Store::Read(const std::size_t inNumBytes, std::byte* inBytes) const
	{
		if(readingCustomScript)
		{
			const auto buf = reinterpret_cast<char*>(inBytes);
			for(std::size_t i = 0; i < inNumBytes; ++i)
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
		return baseStore->Read(inNumBytes, inBytes);
	}

	RE::BSStorageDefs::ErrorCode Store::Write(const std::size_t inNumBytes, const std::byte* inBytes)
	{
		if(readingCustomScript)
		{
			///Unsupported
			return static_cast<RE::BSStorageDefs::ErrorCode>(8);
		}
		return baseStore->Write(inNumBytes, inBytes);
	}

	bool Store::Open(const char* inFileName)
	{
		if(auto* dataManager = DataManager::GetSingleton(); dataManager->compiledScripts.contains(inFileName))
		{
			chainedPoolIt = dataManager->compiledScripts[inFileName]->getData().begin();
			chainedPoolPos = 0;
			index = 0;
			scriptName = inFileName;
			fakePath = "data\\SCRIPTS\\" + scriptName + ".pex";
			readingCustomScript = true;
			return true;
		}
		return baseStore->Open(inFileName);
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
			baseStore = vm->scriptStore.get();
			const auto customStore = RE::make_smart<Store>();
			vm->scriptLoader.SetScriptStore(customStore);
			store = customStore.get();
		}
	}

	// # Caprica
	// =======================
	namespace
	{
		bool AddFilesFromDirectory(const caprica::IInputFile& inFile, const std::filesystem::path& inOutputDir, caprica::CapricaJobManager* inJobManager, caprica::papyrus::PapyrusCompilationNode::NodeType inNodeType)
		{
			// Blargle flargle.... Using the raw Windows API is 5x
			// faster than boost::filesystem::recursive_directory_iterator,
			// at 40ms vs. 200ms for the boost solution, and the raw API
			// solution also gets us the relative paths, absolute paths,
			// last write time, and file size, all without any extra processing.
			if (!inFile.exists())
			{
				return false;
			}

			auto absBaseDir = inFile.resolved_absolute_basedir();
			auto subDir = inFile.resolved_relative();
			if (subDir == ".")
			{
				subDir = "";
			}
			bool recursive = inFile.isRecursive();
			std::vector<std::filesystem::path> dirs{};
			dirs.push_back(subDir);

			while (!dirs.empty())
			{
				HANDLE hFind;
				WIN32_FIND_DATA data;
				auto curDir = dirs.back();
				dirs.pop_back();
				auto curSearchPattern = absBaseDir / curDir / "*";
				caprica::caseless_unordered_identifier_ref_map<caprica::papyrus::PapyrusCompilationNode*> namespaceMap{};
				namespaceMap.reserve(8000);

				hFind = FindFirstFileA(curSearchPattern.string().c_str(), &data);
				if (hFind == INVALID_HANDLE_VALUE)
				{
					return false;
				}

				do
				{
					if (std::string_view filenameRef = data.cFileName; filenameRef != "." && filenameRef != "..")
					{
						if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							if (recursive)
							{
								dirs.push_back(curDir / data.cFileName);
							}
						}
						else
						{
							ULARGE_INTEGER ull;
							ull.LowPart = data.ftLastWriteTime.dwLowDateTime;
							ull.HighPart = data.ftLastWriteTime.dwHighDateTime;
							std::time_t lastModTime = static_cast<time_t>(ull.QuadPart / 10000000ULL - 11644473600ULL);

							ull.LowPart = data.nFileSizeLow;
							ull.HighPart = data.nFileSizeHigh;
							auto fileSize = ull.QuadPart;

							const std::filesystem::path cur = curDir == "." ? "" : curDir;

							caprica::papyrus::PapyrusCompilationNode* node = new caprica::papyrus::PapyrusCompilationNode
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
				} while (FindNextFileA(hFind, &data));
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
			caprica::caseless_unordered_identifier_ref_map<caprica::papyrus::PapyrusCompilationNode*> tempMap{};
			for (auto& fakeScript : FAKE_SKYRIM_SCRIPTS_SET)   // NOLINT(bugprone-nondeterministic-pointer-iteration-order)
			{
				auto basename = caprica::FSUtils::filenameAsRef(fakeScript);
				auto node = new caprica::papyrus::PapyrusCompilationNode
				(
					inJobManager,
					caprica::papyrus::PapyrusCompilationNode::NodeType::PapyrusImport,
					std::string(basename),
					"",
					std::string(fakeScript),
					0,
					caprica::FakeScripts::getSizeOfFakeScript(fakeScript, caprica::conf::Papyrus::game),
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
			caprica::CapricaReportingContext reportingContext{ flagsPath };
			caprica::parser::CapricaUserFlagsParser(reportingContext, flagsPath).parseUserFlags(caprica::conf::Papyrus::userFlagsDefinition);
		}

		caprica::CapricaJobManager jobManager{};
	}

	void Init()
	{
		constexpr std::string_view flagsName = "TESV_Papyrus_Flags.flg";
		std::string scriptPath = "./Data/Source/Scripts/";
		auto flagPath = scriptPath + flagsName.data();
		if(!std::filesystem::exists(flagPath))
		{
			scriptPath = "./Data/Scripts/Source/";
			flagPath = scriptPath + flagsName.data();
			if (!std::filesystem::exists(flagPath))
			{
				SKSE::stl::report_and_fail("Missing vanilla source scripts.");
			}
		}

		caprica::conf::Papyrus::game = caprica::GameID::Skyrim;
		caprica::conf::Papyrus::importDirectories.emplace_back(caprica::FSUtils::canonical(scriptPath), false);
		if(!HandleImports(caprica::conf::Papyrus::importDirectories, &jobManager)) 
		{
			return;
		}
		ParseUserFlags(flagPath);
		caprica::papyrus::PapyrusCompilationContext::awaitRead();

		Store::Init();
	}

	void AddScript(const std::string& inStringName, const std::string_view& inData)
	{
		auto node = new caprica::papyrus::PapyrusCompilationNode
		(
			&jobManager,
			inStringName,
			inData
		);
		DataManager::GetSingleton()->compiledScripts[inStringName] = node;

		caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents
		(
			"",
			caprica::caseless_unordered_identifier_ref_map<caprica::papyrus::PapyrusCompilationNode *>
			{
				{caprica::identifier_ref(node->baseName), node}
			}
		);
		caprica::papyrus::PapyrusCompilationContext::awaitCompile();
		caprica::papyrus::PapyrusCompilationContext::removeFromNamespace(node->baseName);
	}
}
