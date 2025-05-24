#include <SQG/API/PackageUtils.h>
#include <SQG/API/DialogUtils.h>
#include <SQG/API/QuestUtils.h>
#include <SQG/API/ConditionUtils.h>
#include <DPF/API.h>

#include "Engine/Dialog.h"
#include "Engine/Package.h"
#include "Engine/Quest.h"
#include "Engine/Script.h"

#include <common/FakeScripts.h>
#include <papyrus/PapyrusCompilationContext.h>
#include <common/parser/CapricaUserFlagsParser.h>

// # Papyrus
// =======================
RE::TESQuest* generatedQuest = nullptr;
RE::TESQuest* selectedQuest = nullptr;
RE::TESObjectREFR* targetForm = nullptr;
RE::TESObjectREFR* activator = nullptr;
RE::TESObjectREFR* targetActivator = nullptr;

void FillQuestWithGeneratedData(RE::TESQuest* inQuest)
{
	//Parametrize quest
	//=======================
	inQuest->SetFormEditorID("SQGTestQuest");
	inQuest->fullName = "00_SQG_POC";

	//Add stages
	//=======================
	SQG::AddQuestStage(inQuest, 10, SQG::QuestStageType::Startup, "My boss told me to kill a man named \"Gibier\"", 0);
	SQG::AddQuestStage(inQuest, 11);
	SQG::AddQuestStage(inQuest, 45, SQG::QuestStageType::Shutdown, "I decided to spare Gibier.", 6);
	SQG::AddQuestStage(inQuest, 40, SQG::QuestStageType::Shutdown, "Gibier is dead.", 5);
	SQG::AddQuestStage(inQuest, 35, SQG::QuestStageType::Default, "When I told Gibier I was going to kill him he asked for a last will. I refused.", 4);
	SQG::AddQuestStage(inQuest, 32, SQG::QuestStageType::Default, "Gibier has done his last will. The time has came for him to die.", 3);
	SQG::AddQuestStage(inQuest, 30);
	SQG::AddQuestStage(inQuest, 20);
	SQG::AddQuestStage(inQuest, 15, SQG::QuestStageType::Default, "When I told Gibier I was going to kill him he asked for a last will. I let him do what he wanted but advised him to not do anything inconsiderate.", 2);
	SQG::AddQuestStage(inQuest, 12, SQG::QuestStageType::Default, "I spoke with Gibier, whom told me my boss was a liar and begged me to spare him. I need to decide what to do.", 1);

	//Add objectives
	//=======================
	SQG::AddObjective(inQuest, 10, "Kill Gibier", {0});
	SQG::AddObjective(inQuest, 12, "(Optional) Spare Gibier");
	SQG::AddObjective(inQuest, 15, "(Optional) Let Gibier do his last will");

	//Add aliases
	//=======================
	SQG::AddRefAlias(inQuest, 0, "SQGTestAliasTarget", targetForm);

	//Add packages
	//=======================

	auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	auto* policy = scriptMachine->GetObjectHandlePolicy();

	//ACQUIRE PACKAGE
	//=============================
	{
		std::unordered_map<std::string, SQG::PackageData> packageDataMap;
		RE::PackageTarget::Target targetData{};
		targetData.objType = RE::PACKAGE_OBJECT_TYPE::kWEAP;
		packageDataMap["Target Criteria"] = SQG::PackageData(RE::PackageTarget::Type::kObjectType, targetData);
		packageDataMap["Num to acquire"] = SQG::PackageData({.i = 2}); 
		packageDataMap["AllowStealing"] = SQG::PackageData({.b = true});
		auto* customAcquirePackage = SQG::CreatePackageFromTemplate
		(
			SQG::PackageEngine::acquirePackage,
			generatedQuest,
			packageDataMap, 
			{SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, {.f = 15.f})});

		const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, customAcquirePackage);
		RE::BSTSmartPointer<RE::BSScript::Object> packageCustomScriptObject;
		scriptMachine->CreateObject("PF_SQGAcquirePackage", packageCustomScriptObject);
		scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);

		SQG::AddAliasPackage(inQuest, targetForm, customAcquirePackage, "PF_SQGAcquirePackage");
	}

	//ACTIVATE PACKAGE
	//=============================
	{
		std::unordered_map<std::string, SQG::PackageData> packageDataMap;
		RE::PackageTarget::Target targetData{};
		targetData.handle = targetActivator->CreateRefHandle();
		packageDataMap["Target"] = SQG::PackageData(RE::PackageTarget::Type::kNearReference, targetData);
		auto* customActivatePackage = SQG::CreatePackageFromTemplate
		(
			SQG::PackageEngine::activatePackage, 
			generatedQuest, 
			packageDataMap, 
			{SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, {.f = 20.f})}
		);

		const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, customActivatePackage);
		RE::BSTSmartPointer<RE::BSScript::Object> packageCustomScriptObject;
		scriptMachine->CreateObject("PF_SQGActivatePackage", packageCustomScriptObject);
		scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);

		SQG::AddAliasPackage(inQuest, targetForm, customActivatePackage, "PF_SQGActivatePackage");
	}

	//TRAVEL PACKAGE
	//=============================
	{
		std::unordered_map<std::string, SQG::PackageData> packageDataMap;
		packageDataMap["Place to Travel"] = SQG::PackageData(RE::PackageLocation::Type::kNearReference, {.refHandle = activator->CreateRefHandle()}, 0);
		auto* customTravelPackage = SQG::CreatePackageFromTemplate
		(
			SQG::PackageEngine::travelPackage, 
			generatedQuest, 
			packageDataMap,
			{SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, {.f = 30.f})}
		);

		const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, customTravelPackage);
		RE::BSTSmartPointer<RE::BSScript::Object> packageCustomScriptObject;
		scriptMachine->CreateObject("PF_SQGTravelPackage", packageCustomScriptObject);
		scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);

		SQG::AddAliasPackage(inQuest, targetForm, customTravelPackage, "PF_SQGTravelPackage");
	}

	//Add dialogs
	//===========================

	auto* checkSpeakerCondition = SQG::CreateCondition(targetForm->data.objectReference, RE::FUNCTION_DATA::FunctionID::kGetIsID, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, {.f = 1.f}); //todo check directly targetForm
	auto* aboveStage0Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kGreaterThan,{.f = 0.f});
	auto* aboveStage10Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kGreaterThan, {.f = 10.f});
	auto* underStage11Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kLessThan,{.f = 11.f});
	auto* underStage12Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kLessThan, {.f = 12.f});
	auto* aboveStage12Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kGreaterThanOrEqualTo, {.f = 12.f});
	auto* underStage15Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kLessThan, {.f = 15.f});
	
	auto* wonderEntry = SQG::AddDialogTopic(inQuest, targetForm, "I've not decided yet. I'd like to hear your side of the story.");
	wonderEntry->AddAnswer
	(
		"Thank you very much, you'll see that I don't diserve this cruelty. Your boss is a liar.",
		"",
		{checkSpeakerCondition, aboveStage0Condition, underStage11Condition},
		11,
		4
	);
	wonderEntry->AddAnswer
	(
		"Your boss is a liar.",
		"Tell me again about the reasons of the contract",
		{checkSpeakerCondition, aboveStage10Condition, underStage15Condition}
	);
	auto* moreWonderEntry = SQG::AddDialogTopic(inQuest, targetForm, "What did he do ?", wonderEntry);
	moreWonderEntry->AddAnswer
	(
		"He lied, I'm the good one in the story.",
		"",
		{},
		12,
		0
	);

	auto* attackEntry = SQG::AddDialogTopic(inQuest, targetForm, "As you guessed. Prepare to die !");
	attackEntry->AddAnswer
	(
		"Wait a minute ! Could I have a last will please ?",
		"",
		{checkSpeakerCondition, aboveStage0Condition, underStage12Condition},
		11,
		4
	);
	attackEntry->AddAnswer
	(
		"Wait a minute ! Could I have a last will please ?",
		"I can't believe my boss would lie. Prepare to die !",
		{checkSpeakerCondition, aboveStage0Condition, underStage15Condition}
	);
	auto* lastWillYesEntry = SQG::AddDialogTopic(inQuest, targetForm, "Yes, of course, proceed but don't do anything inconsiderate.", attackEntry);
	lastWillYesEntry->AddAnswer
	(						
		"Thank you for your consideration",
		"",
		{},
		15,
		1
	);
	auto* lastWillNoEntry = SQG::AddDialogTopic(inQuest, targetForm, "No, I came here for business, not charity.", attackEntry);
	lastWillNoEntry->AddAnswer
	(
		"Your lack of dignity is a shame.",
		"",
		{},
		35,
		2
	);

	auto* spareEntry = SQG::AddDialogTopic(inQuest, targetForm, "Actually, I decided to spare you.");
	spareEntry->AddAnswer
	(
		"You're the kindest. I will make sure to hide myself from the eyes of your organization.",
		"",
		{checkSpeakerCondition, aboveStage12Condition, underStage15Condition},
		45,
		3
	);

	SQG::AddForceGreet(inQuest, targetForm, "So you came here to kill me, right ?", {underStage11Condition});
	SQG::AddHelloTopic(targetForm, "What did you decide then ?");
}

void AttachScriptsToQuest(const RE::TESQuest* inQuest)
{
	//Add script logic
	//===========================
	auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	auto* policy = scriptMachine->GetObjectHandlePolicy();

	const auto selectedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, inQuest);
	//TODO!! try to call script compiler from c++ before loading custom script
	RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
	scriptMachine->CreateObjectWithProperties("SQGDebug", 1, questCustomScriptObject);
	scriptMachine->BindObject(questCustomScriptObject, selectedQuestHandle, false);
	SQG::questsData[inQuest->formID].script = questCustomScriptObject.get();

	RE::BSTSmartPointer<RE::BSScript::Object> referenceAliasBaseScriptObject;
	scriptMachine->CreateObject("SQGQuestTargetScript", referenceAliasBaseScriptObject);
	const auto questAliasHandle = selectedQuestHandle & 0x0000FFFFFFFF;
	scriptMachine->BindObject(referenceAliasBaseScriptObject, questAliasHandle, false);

	RE::BSScript::Variable propertyValue;
	propertyValue.SetObject(referenceAliasBaseScriptObject);
	scriptMachine->SetPropertyValue(questCustomScriptObject, "SQGTestAliasTarget", propertyValue);
	questCustomScriptObject->initialized = true; //Required when creating object with properties

	RE::BSTSmartPointer<RE::BSScript::Object> dialogFragmentsCustomScriptObject;
	scriptMachine->CreateObject("SQGTopicFragments", dialogFragmentsCustomScriptObject);
	scriptMachine->BindObject(dialogFragmentsCustomScriptObject, selectedQuestHandle, false);
}

std::string GenerateQuest(RE::StaticFunctionTag*)
{
	//Create quest if needed
	//=======================
	if(generatedQuest)
	{
		return "Quest yet generated.";
	}

	selectedQuest = generatedQuest = SQG::CreateQuest();

	FillQuestWithGeneratedData(generatedQuest);
	AttachScriptsToQuest(generatedQuest);

	//Notify success
	//===========================
	std::ostringstream ss;
    ss << std::hex << generatedQuest->formID;
	const auto formId = ss.str();
	SKSE::log::debug("Generated quest with formID: {0}"sv, formId);
	return "Generated quest with formID: " + formId;
}

RE::TESQuest* GetSelectedQuest(RE::StaticFunctionTag*)
{
	return selectedQuest;
}

void StartSelectedQuest(RE::StaticFunctionTag*)
{
	if(selectedQuest)
	{
		selectedQuest->Start();
	}
}

std::string SwapSelectedQuest(RE::StaticFunctionTag*)
{
	selectedQuest = selectedQuest == SQG::QuestEngine::referenceQuest ? generatedQuest : SQG::QuestEngine::referenceQuest;
	return "Selected " + std::string(selectedQuest ? selectedQuest->GetFullName() : "nullptr");
}

caprica::papyrus::PapyrusCompilationNode* getNode(const caprica::papyrus::PapyrusCompilationNode::NodeType& nodeType,
                                                  caprica::CapricaJobManager* jobManager,
                                                  const std::filesystem::path& baseOutputDir,
                                                  const std::filesystem::path& curDir,
                                                  const std::filesystem::path& absBaseDir,
                                                  const std::filesystem::path& fileName,
                                                  time_t lastModTime,
                                                  size_t fileSize,
                                                  bool strictNS) {
  std::filesystem::path cur;
  if (curDir == ".")
    cur = "";
  else
    cur = curDir;
  std::filesystem::path sourceFilePath = absBaseDir / cur / fileName;
  std::filesystem::path filenameToDisplay = cur / fileName;

  auto node = new caprica::papyrus::PapyrusCompilationNode(jobManager,
                                                           nodeType,
                                                           std::move(filenameToDisplay.string()),
                                                           std::move(baseOutputDir.string()),
                                                           std::move(sourceFilePath.string()),
                                                           lastModTime,
                                                           fileSize,
                                                           strictNS);
  return node;
}

caprica::papyrus::PapyrusCompilationNode* getNode(const caprica::papyrus::PapyrusCompilationNode::NodeType& nodeType,
                                                  caprica::CapricaJobManager* jobManager,
                                                  const std::filesystem::path& baseOutputDir,
                                                  const std::filesystem::path& curDir,
                                                  const std::filesystem::path& absBaseDir,
                                                  const WIN32_FIND_DATA& data,
                                                  bool strictNS) {
  const auto lastModTime = [](FILETIME ft) -> time_t {
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    return ull.QuadPart / 10000000ULL - 11644473600ULL;
  }(data.ftLastWriteTime);
  const auto fileSize = [](DWORD low, DWORD high) {
    ULARGE_INTEGER ull;
    ull.LowPart = low;
    ull.HighPart = high;
    return ull.QuadPart;
  }(data.nFileSizeLow, data.nFileSizeHigh);
  return getNode(nodeType,
                 jobManager,
                 baseOutputDir,
                 curDir,
                 absBaseDir,
                 data.cFileName,
                 lastModTime,
                 fileSize,
                 strictNS);
}

static bool gBaseFound = true;

bool addSingleFile(const caprica::IInputFile& input,
                   const std::filesystem::path& baseOutputDir,
                   caprica::CapricaJobManager* jobManager,
                   caprica::papyrus::PapyrusCompilationNode::NodeType nodeType) {
  // Get the file size and last modified time using std::filesystem
  std::error_code ec;

  const auto relPath = input.resolved_relative();
  const auto namespaceDir = relPath.parent_path();
  const auto absPath = input.resolved_absolute();
  const auto filename = absPath.filename();
  const auto absBaseDir = input.resolved_absolute_basedir();
  auto lastModTime = std::filesystem::last_write_time(absPath, ec);
  if (ec) {
    std::cout << "An error occurred while trying to get the last modified time of '" << absPath << "'!" << std::endl;
    return false;
  }
  auto fileSize = std::filesystem::file_size(absPath, ec);
  if (ec) {
    std::cout << "An error occurred while trying to get the file size of '" << absPath << "'!" << std::endl;
    return false;
  }

  auto namespaceName = caprica::FSUtils::pathToObjectName(namespaceDir);

  auto node = getNode(nodeType,
                      jobManager,
                      baseOutputDir,
                      namespaceDir,
                      absBaseDir,
                      filename,
                      lastModTime.time_since_epoch().count(),
                      fileSize,
                      !input.requiresRemap());
  caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents(
          namespaceName,
          caprica::caseless_unordered_identifier_ref_map<caprica::papyrus::PapyrusCompilationNode *>{
                  {caprica::identifier_ref(node->baseName), node}
          });
  return true;
}

bool addFilesFromDirectory(const caprica::IInputFile& input,
                           const std::filesystem::path& baseOutputDir,
                           caprica::CapricaJobManager* jobManager,
                           caprica::papyrus::PapyrusCompilationNode::NodeType nodeType,
                           const std::string& startingNS = "") {
  // Blargle flargle.... Using the raw Windows API is 5x
  // faster than boost::filesystem::recursive_directory_iterator,
  // at 40ms vs. 200ms for the boost solution, and the raw API
  // solution also gets us the relative paths, absolute paths,
  // last write time, and file size, all without any extra processing.
  if (!input.exists()) {
    std::cout << "ERROR: Input file '" << input.get_unresolved_path() << "' was not found!" << std::endl;
    return false;
  }
  auto abspath = input.resolved_absolute();
  auto absBaseDir = input.resolved_absolute_basedir();
  auto subdir = input.resolved_relative();
  if (subdir == ".")
    subdir = "";
  bool recursive = input.isRecursive();
  std::vector<std::filesystem::path> dirs {};
  dirs.push_back(subdir);
  auto baseDirMap = caprica::caseless_unordered_identifier_ref_map<bool>{};
  auto l_startNS = startingNS;
  if (l_startNS == "") {
    if (caprica::conf::Papyrus::game > caprica::GameID::Skyrim) {
      if (input.requiresRemap())
        l_startNS = "!!temp" + abspath.filename().string() + std::to_string(rand());
    }
  }
  const auto DOTDOT = std::string_view("..");
  const auto DOT = std::string_view(".");

  while (dirs.size()) {
    HANDLE hFind;
    WIN32_FIND_DATA data;
    auto curDir = dirs.back();
    dirs.pop_back();
    auto curSearchPattern = absBaseDir / curDir / "*";
    caprica::caseless_unordered_identifier_ref_map<caprica::papyrus::PapyrusCompilationNode *> namespaceMap{};
    namespaceMap.reserve(8000);

    hFind = FindFirstFileA(curSearchPattern.string().c_str(), &data);
    if (hFind == INVALID_HANDLE_VALUE) {
      std::cout << "An error occurred while trying to iterate the files in '" << curSearchPattern << "'!" << std::endl;
      return false;
    }

    do {
      std::string_view filenameRef = data.cFileName;
      if (filenameRef != DOT && filenameRef != DOTDOT) {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          if (recursive)
            dirs.push_back(curDir / data.cFileName);
        } else {
          auto ext = caprica::FSUtils::extensionAsRef(filenameRef);
          bool skip = false;

          switch (nodeType) {
            case caprica::papyrus::PapyrusCompilationNode::NodeType::PapyrusCompile:
            case caprica::papyrus::PapyrusCompilationNode::NodeType::PapyrusImport:
              if (!caprica::pathEq(ext, ".psc"))
                skip = true;
              break;
            case caprica::papyrus::PapyrusCompilationNode::NodeType::PasReflection:
            case caprica::papyrus::PapyrusCompilationNode::NodeType::PasCompile:
              if (!caprica::pathEq(ext, ".pas"))
                skip = true;
              break;
            case caprica::papyrus::PapyrusCompilationNode::NodeType::PexReflection:
            case caprica::papyrus::PapyrusCompilationNode::NodeType::PexDissassembly:
              if (!caprica::pathEq(ext, ".pex"))
                skip = true;
              break;
            default:
              skip = true;
              break;
          }
          if (!skip) {
	          caprica::papyrus::PapyrusCompilationNode* node =
                getNode(nodeType, jobManager, baseOutputDir, curDir, absBaseDir, data, !input.requiresRemap());

            namespaceMap.emplace(caprica::identifier_ref(node->baseName), node);
            if (!gBaseFound && !baseDirMap.empty()) {
              // get the filename without the extension using substr assuming that the last 4 characters are the
              // extension
              if (baseDirMap.count(node->baseName) != 0)
                baseDirMap[node->baseName] = true;
            }
          }
        }
      }
    } while (FindNextFileA(hFind, &data));
    FindClose(hFind);

    if (caprica::conf::Papyrus::game > caprica::GameID::Skyrim) {
      if (!namespaceMap.empty()) {
        // check that all the values in the base script dir map are true
        if (!gBaseFound && !baseDirMap.empty()) {
          bool allTrue = true;
          for (auto& pair : baseDirMap) {
            if (!pair.second) {
              allTrue = false;
              break;
            }
          }
          if (allTrue && namespaceMap.size() >= 0) {
            // if it's true, then this is the base dir and the namespace should be root
            gBaseFound = true;
            l_startNS = "";
          }
        }
        auto curDirNS = caprica::FSUtils::pathToObjectName(curDir);
        auto namespaceName = l_startNS.empty() ? curDirNS : (l_startNS + ":" + curDirNS);
        caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents(namespaceName, std::move(namespaceMap));
      }
    } else {
      caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents("", std::move(namespaceMap));
    }
    // We don't repopulate it because it's either in the root of whatever was imported or it's not in this import at all
    baseDirMap.clear();
  }
  return true;
}

static const std::unordered_set FAKE_SKYRIM_SCRIPTS_SET = {
        "fake://skyrim/__ScriptObject.psc",
        "fake://skyrim/DLC1SCWispWallScript.psc",
};
bool handleImports(const std::vector<caprica::ImportDir>& f, caprica::CapricaJobManager* jobManager) {
  // Skyrim hacks; we need to import Skyrim's fake scripts into the global namespace first.
	caprica::caseless_unordered_identifier_ref_map<caprica::papyrus::PapyrusCompilationNode *> tempMap{};
	for (auto &fake_script: FAKE_SKYRIM_SCRIPTS_SET) {
	  auto basename = caprica::FSUtils::filenameAsRef(fake_script);
	  auto node =
	      new caprica::papyrus::PapyrusCompilationNode(jobManager,
	                                                   caprica::papyrus::PapyrusCompilationNode::NodeType::PapyrusImport,
	                                                   std::string(basename),
	                                                   "",
	                                                   std::string(fake_script),
	                                                   0,
	                                                   caprica::FakeScripts::getSizeOfFakeScript(fake_script, caprica::conf::Papyrus::game),
	                                                   false);
	  tempMap.emplace(caprica::identifier_ref(node->baseName), node);
	}
	caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents("", std::move(tempMap));
  
  std::cout << "Importing files..." << std::endl;
  int i = 0;
  for (auto& dir : f) {
    std::string ns = "";
    if (!addFilesFromDirectory(dir, "", jobManager, caprica::papyrus::PapyrusCompilationNode::NodeType::PapyrusImport, ns))
      return false;
  }
  return true;
}

void parseUserFlags(std::string &&flagsPath) {
  caprica::CapricaReportingContext reportingContext{flagsPath};
  auto parser = new caprica::parser::CapricaUserFlagsParser(reportingContext, flagsPath);
  parser->parseUserFlags(caprica::conf::Papyrus::userFlagsDefinition);
  delete parser;
}

void DraftDebugFunction(RE::StaticFunctionTag*)
{
    caprica::CapricaJobManager jobManager{};
    caprica::conf::Papyrus::game = caprica::GameID::Skyrim;

    std::string d("H:\\Games\\SkyrimSE\\Data\\Scripts\\Source");
    caprica::conf::Papyrus::importDirectories.reserve(caprica::conf::Papyrus::importDirectories.size() + 1);
    // check if string contains `;`
    std::istringstream f(d);
    std::string sd;
    while (getline(f, sd, ';')) {
      if (!std::filesystem::exists(sd)) {
        std::cout << "Unable to find the import directory '" << sd << "'!" << std::endl;
        return;
      }
      caprica::conf::Papyrus::importDirectories.emplace_back(caprica::FSUtils::canonical(sd), false);
      if (!caprica::conf::General::quietCompile) {
        std::cout << "Adding import directory: " << caprica::conf::Papyrus::importDirectories.back().get_unresolved_path()
                  << std::endl;
      }
    }

    if (!std::filesystem::exists(d)) {
      std::cout << "Unable to find the import directory '" << d << "'!" << std::endl;
      return;
    }
    caprica::conf::Papyrus::importDirectories.emplace_back(caprica::FSUtils::canonical(d), false);
    if (!caprica::conf::General::quietCompile) {
      std::cout << "Adding import directory: " << caprica::conf::Papyrus::importDirectories.back().get_unresolved_path()
                << std::endl;
    }

     if (!handleImports(caprica::conf::Papyrus::importDirectories, &jobManager)) {
      std::cout << "Import failed!" << std::endl;
      return;
    }


    parseUserFlags("H:\\Games\\SkyrimSE\\Data\\Scripts\\Source\\TESV_Papyrus_Flags.flg");

    caprica::conf::General::inputFiles.reserve(caprica::conf::General::inputFiles.size() + 1);
    caprica::conf::General::inputFiles.emplace_back(std::make_shared<caprica::InputFile>("D:\\tmp\\CapricaCompileScripts\\Caprica\\SQGDebug.psc", true));


    const std::filesystem::path baseOutputDir("D:\\tmp\\CapricaCompileScripts\\Caprica");
    caprica::conf::General::outputDirectory = baseOutputDir;

    for (auto& input : caprica::conf::General::inputFiles) {
      if (!input->resolve()) {
        std::cout << "Unable to locate input file '" << input->get_unresolved_path() << "'." << std::endl;
        return;
      }
        if(!addSingleFile(*input,
                                baseOutputDir,
                                &jobManager,
                                caprica::papyrus::PapyrusCompilationNode::NodeType::PapyrusCompile)) {
        std::cout << "Unable to add input file '" << input->get_unresolved_path() << "'." << std::endl;
        return;
      }
    }

    jobManager.startup((uint32_t) std::thread::hardware_concurrency());
    caprica::papyrus::PapyrusCompilationContext::RenameImports(&jobManager);
    caprica::papyrus::PapyrusCompilationContext::doCompile(&jobManager);
}	

bool RegisterFunctions(RE::BSScript::IVirtualMachine* inScriptMachine)
{
	inScriptMachine->RegisterFunction("GenerateQuest", "SQGLib", GenerateQuest);
	inScriptMachine->RegisterFunction("SwapSelectedQuest", "SQGLib", SwapSelectedQuest);
	inScriptMachine->RegisterFunction("GetSelectedQuest", "SQGLib", GetSelectedQuest);
	inScriptMachine->RegisterFunction("StartSelectedQuest", "SQGLib", StartSelectedQuest);
	inScriptMachine->RegisterFunction("DraftDebugFunction", "SQGLib", DraftDebugFunction);
	return true;
}

// # SKSE
// =======================

void InitializeLog()
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = SKSE::log::log_directory();
	if (!path) 
	{
		util::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format("{}.log"sv, Plugin::NAME);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
	constexpr auto level = spdlog::level::trace;
#else
	constexpr auto level = spdlog::level::info;
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(level);
	log->flush_on(level);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);
}

// ReSharper disable once CppInconsistentNaming
extern "C" DLLEXPORT bool SKSEPlugin_Query(const SKSE::QueryInterface* inQueryInterface, SKSE::PluginInfo* outPluginInfo)
{
	if (inQueryInterface->IsEditor()) 
	{
		SKSE::log::critical("This plugin is not designed to run in Editor\n"sv);
		return false;
	}

	if (const auto runtimeVersion = inQueryInterface->RuntimeVersion(); runtimeVersion < SKSE::RUNTIME_1_5_97) 
	{
		SKSE::log::critical("Unsupported runtime version %s!\n"sv, runtimeVersion.string());
		return false;
	}

	outPluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	outPluginInfo->name = Plugin::NAME.data();
	outPluginInfo->version = Plugin::VERSION[0];

	return true;
}

// ReSharper disable once CppInconsistentNaming
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* inLoadInterface)
{
#ifndef NDEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	InitializeLog();
	SKSE::log::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());

	Init(inLoadInterface);

	if (const auto papyrusInterface = SKSE::GetPapyrusInterface(); !papyrusInterface || !papyrusInterface->Register(RegisterFunctions)) 
	{
		return false;
	}

	if(const auto messaging = SKSE::GetMessagingInterface(); !messaging 
		|| !messaging->RegisterListener([](SKSE::MessagingInterface::Message* message)
		{
			if (auto* dataHandler = RE::TESDataHandler::GetSingleton(); message->type == SKSE::MessagingInterface::kDataLoaded)
			{
				DPF::Init(0x800, "SQGLib.esp");

				SQG::ScriptEngine::Store::Init();

				SQG::QuestEngine::LoadData(dataHandler);
				SQG::DialogEngine::LoadData(dataHandler);
				SQG::PackageEngine::LoadData();

				SQG::QuestEngine::RegisterSinks();
				SQG::DialogEngine::RegisterSinks();
				SQG::PackageEngine::RegisterSinks();

				selectedQuest = SQG::QuestEngine::referenceQuest;
				targetForm = reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x00439A}, "SQGLib.esp"));
				activator =  reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x001885}, "SQGLib.esp"));  
				targetActivator = reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x008438}, "SQGLib.esp"));
			}
			else if(message->type == SKSE::MessagingInterface::kPreLoadGame)
			{
				DPF::LoadCache(message);
			}
			else if(message->type == SKSE::MessagingInterface::kSaveGame)
			{
				DPF::SaveCache(message);
			}
			else if(message->type == SKSE::MessagingInterface::kDeleteGame)
			{
				DPF::DeleteCache(message);
			}
		})
	)
	{
	    return false;
	}

	SKSE::AllocTrampoline(1<<10);
	auto& trampoline = SKSE::GetTrampoline();
	SQG::QuestEngine::RegisterHooks(trampoline);
	SQG::DialogEngine::RegisterHooks(trampoline);

	return true;
}
