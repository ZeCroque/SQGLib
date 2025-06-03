#include <SQG/API/PackageUtils.h>
#include <SQG/API/DialogUtils.h>
#include <SQG/API/QuestUtils.h>
#include <SQG/API/ConditionUtils.h>
#include <DPF/API.h>

#include "Engine/Dialog.h"
#include "Engine/Package.h"
#include "Engine/Quest.h"
#include "Engine/Script.h"
#include "SQG/API/ScriptUtils.h"

// # Papyrus
// =======================
RE::TESObjectREFR* activator = nullptr;
RE::TESObjectREFR* targetActivator = nullptr;
int generatedQuestIndex = 0;

std::vector<RE::TESQuest*> generatedQuests;
std::vector<RE::TESObjectREFR*> targets;

void FillQuestWithGeneratedData(RE::TESQuest* inQuest, RE::TESObjectREFR* inTarget)
{
	//Parametrize quest
	//=======================
	auto questIndex = std::format("{:02}", generatedQuestIndex);
	inQuest->SetFormEditorID(("SQGDebug" + questIndex).c_str());
	inQuest->fullName = questIndex + "_SQG_POC";
	++generatedQuestIndex;

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
	SQG::AddRefAlias(inQuest, 0, "SQGTestAliasTarget", inTarget);

	//Add script logic
	//===========================
	std::ifstream ifs;
	for(std::filesystem::path scriptPath = "./Data/SKSE/Plugins/SQGLib/"; const auto& file : std::filesystem::directory_iterator{scriptPath})
	{
		ifs.open(file, std::ifstream::ate);
		auto size = ifs.tellg();
		ifs.close();
		char* buf = new char[size];
		std::memset(buf, 0, size);
		ifs.open(file);
		ifs.read(buf, size);
		ifs.close();

		for(auto i = 0; i < size; ++i)
		{
			if(buf[i] == 'Z')
			{
				buf[i] = questIndex[0];
				buf[i + 1] = questIndex[1];
				break;
			}
		}
		SQG::AddScript(file.path().stem().string() + questIndex, buf);
		delete[] buf;
	}

	auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	auto* policy = scriptMachine->GetObjectHandlePolicy();

	const auto selectedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, inQuest);
	RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
	scriptMachine->CreateObjectWithProperties("SQGDebug" + questIndex, 1, questCustomScriptObject);
	scriptMachine->BindObject(questCustomScriptObject, selectedQuestHandle, false);
	SQG::questsData[inQuest->formID].script = questCustomScriptObject.get();

	RE::BSTSmartPointer<RE::BSScript::Object> referenceAliasBaseScriptObject;
	scriptMachine->CreateObject("SQGQuestTargetScript" + questIndex, referenceAliasBaseScriptObject);
	const auto questAliasHandle = selectedQuestHandle & 0x0000FFFFFFFF;
	scriptMachine->BindObject(referenceAliasBaseScriptObject, questAliasHandle, false);

	RE::BSScript::Variable propertyValue;
	propertyValue.SetObject(referenceAliasBaseScriptObject);
	scriptMachine->SetPropertyValue(questCustomScriptObject, ("SQGTestAliasTarget" + questIndex).c_str(), propertyValue);
	questCustomScriptObject->initialized = true; //Required when creating object with properties

	RE::BSTSmartPointer<RE::BSScript::Object> dialogFragmentsCustomScriptObject;
	scriptMachine->CreateObject("TF_SQGDebug" + questIndex, dialogFragmentsCustomScriptObject);
	scriptMachine->BindObject(dialogFragmentsCustomScriptObject, selectedQuestHandle, false);

	//Add packages
	//=======================

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
			inQuest,
			packageDataMap, 
			{SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, {.f = 15.f})});

		const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, customAcquirePackage);
		RE::BSTSmartPointer<RE::BSScript::Object> packageCustomScriptObject;
		scriptMachine->CreateObject("PF_SQGAcquirePackage" + questIndex, packageCustomScriptObject);
		scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);

		SQG::AddAliasPackage(inQuest, inTarget, customAcquirePackage, "PF_SQGAcquirePackage" + questIndex);
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
			inQuest, 
			packageDataMap, 
			{SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, {.f = 20.f})}
		);

		const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, customActivatePackage);
		RE::BSTSmartPointer<RE::BSScript::Object> packageCustomScriptObject;
		scriptMachine->CreateObject("PF_SQGActivatePackage" + questIndex, packageCustomScriptObject);
		scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);

		SQG::AddAliasPackage(inQuest, inTarget, customActivatePackage, "PF_SQGActivatePackage" + questIndex);
	}

	//TRAVEL PACKAGE
	//=============================
	{
		std::unordered_map<std::string, SQG::PackageData> packageDataMap;
		packageDataMap["Place to Travel"] = SQG::PackageData(RE::PackageLocation::Type::kNearReference, {.refHandle = activator->CreateRefHandle()}, 0);
		auto* customTravelPackage = SQG::CreatePackageFromTemplate
		(
			SQG::PackageEngine::travelPackage, 
			inQuest, 
			packageDataMap,
			{SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, {.f = 30.f})}
		);

		const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, customTravelPackage);
		RE::BSTSmartPointer<RE::BSScript::Object> packageCustomScriptObject;
		scriptMachine->CreateObject("PF_SQGTravelPackage" + questIndex, packageCustomScriptObject);
		scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);

		SQG::AddAliasPackage(inQuest, inTarget, customTravelPackage, "PF_SQGTravelPackage" + questIndex);
	}

	//Add dialogs
	//===========================

	auto* checkSpeakerCondition = SQG::CreateCondition(inTarget->data.objectReference, RE::FUNCTION_DATA::FunctionID::kGetIsID, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, {.f = 1.f}); //todo check directly targetForm
	auto* aboveStage0Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kGreaterThan,{.f = 0.f});
	auto* aboveStage10Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kGreaterThan, {.f = 10.f});
	auto* underStage11Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kLessThan,{.f = 11.f});
	auto* underStage12Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kLessThan, {.f = 12.f});
	auto* aboveStage12Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kGreaterThanOrEqualTo, {.f = 12.f});
	auto* underStage15Condition = SQG::CreateCondition(inQuest, RE::FUNCTION_DATA::FunctionID::kGetStage, RE::CONDITION_ITEM_DATA::OpCode::kLessThan, {.f = 15.f});
	
	auto* wonderEntry = SQG::AddDialogTopic(inQuest, inTarget, "I've not decided yet. I'd like to hear your side of the story.");
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
	auto* moreWonderEntry = SQG::AddDialogTopic(inQuest, inTarget, "What did he do ?", wonderEntry);
	moreWonderEntry->AddAnswer
	(
		"He lied, I'm the good one in the story.",
		"",
		{},
		12,
		0
	);

	auto* attackEntry = SQG::AddDialogTopic(inQuest, inTarget, "As you guessed. Prepare to die !");
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
	auto* lastWillYesEntry = SQG::AddDialogTopic(inQuest, inTarget, "Yes, of course, proceed but don't do anything inconsiderate.", attackEntry);
	lastWillYesEntry->AddAnswer
	(						
		"Thank you for your consideration",
		"",
		{},
		15,
		1
	);
	auto* lastWillNoEntry = SQG::AddDialogTopic(inQuest, inTarget, "No, I came here for business, not charity.", attackEntry);
	lastWillNoEntry->AddAnswer
	(
		"Your lack of dignity is a shame.",
		"",
		{},
		35,
		2
	);

	auto* spareEntry = SQG::AddDialogTopic(inQuest, inTarget, "Actually, I decided to spare you.");
	spareEntry->AddAnswer
	(
		"You're the kindest. I will make sure to hide myself from the eyes of your organization.",
		"",
		{checkSpeakerCondition, aboveStage12Condition, underStage15Condition},
		45,
		3
	);

	SQG::AddForceGreet(inQuest, inTarget, "So you came here to kill me, right ?", {underStage11Condition});
	SQG::AddHelloTopic(inTarget, "What did you decide then ?");
}

std::string GenerateQuest(RE::StaticFunctionTag*, RE::TESObjectREFR* inTarget)
{
	targets.push_back(inTarget);

	if(generatedQuestIndex > 99)
	{
		return "Can't generate more than 100 quests";
	}

	const auto generatedQuest = SQG::CreateQuest();
	generatedQuests.push_back(generatedQuest);
	FillQuestWithGeneratedData(generatedQuest, inTarget);
	generatedQuest->Start();

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
	return nullptr;
}

void StartSelectedQuest(RE::StaticFunctionTag*)
{
}

std::string SwapSelectedQuest(RE::StaticFunctionTag*)
{
	return "Selected nullptr";
}

void DraftDebugFunction(RE::StaticFunctionTag*)
{
	int z = 42;
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
				DPF::Init(0x800, "SQGLib.esp", "sqg");

				SQG::ScriptEngine::Init();

				SQG::QuestEngine::LoadData(dataHandler);
				SQG::DialogEngine::LoadData(dataHandler);
				SQG::PackageEngine::LoadData();

				SQG::QuestEngine::RegisterSinks();
				SQG::DialogEngine::RegisterSinks();
				SQG::PackageEngine::RegisterSinks();

				activator =  reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x001885}, "SQGLib.esp"));  
				targetActivator = reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x008438}, "SQGLib.esp"));
			}
			else if(message->type == SKSE::MessagingInterface::kPreLoadGame)
			{
				SQG::questsData.clear();
				SQG::dialogTopicsData.clear();
				SQG::forceGreetAnswers.clear();
				SQG::helloAnswers.clear();
				SQG::ScriptEngine::compiledScripts.clear();
				SQG::PackageEngine::packagesFragmentName.clear();
				generatedQuests.clear();
				targets.clear();

				if(const auto serializer = DPF::LoadCache(message))
				{
					const auto size = serializer->Read<size_t>();
					for(auto i = 0; i < size; ++i)
					{
						const auto formId = serializer->Read<RE::FormID>();
						auto* quest = DPF::GetForm<RE::TESQuest>(formId);
						SQG::questsData[formId].quest = quest;

						const auto stagesCount = serializer->Read<size_t>();
						for(auto j = 0; j < stagesCount; ++j)
						{
							const auto data = serializer->Read<RE::QUEST_STAGE_DATA>();
							const auto type = data.flags.all(RE::QUEST_STAGE_DATA::Flag::kStartUpStage) ? SQG::QuestStageType::Startup : (data.flags.all(RE::QUEST_STAGE_DATA::Flag::kShutDownStage) ? SQG::QuestStageType::Shutdown : SQG::QuestStageType::Default);

							std::string logEntry;
							if(serializer->Read<bool>())
							{
								logEntry = serializer->ReadString();
							}

							int fragmentIndex = -1;
							if(serializer->Read<bool>())
							{
								fragmentIndex = serializer->Read<int>();
							}

							SQG::AddQuestStage(quest, data.index, type, logEntry, fragmentIndex);
						}

						const auto objectiveCount = serializer->Read<size_t>();
						for(auto j = 0; j < objectiveCount; ++j)
						{
							const auto index = serializer->Read<uint16_t>();

							const auto text = serializer->ReadString();

							const auto numTargets = serializer->Read<uint32_t>();
							std::list<uint8_t> targets;
							for(auto i = 0; i < numTargets; ++i)
							{
								targets.emplace_back(serializer->Read<uint8_t>());
							}

							SQG::AddObjective(quest, index, text, targets);
						}

						auto aliasesPackagesDataCount = serializer->Read<size_t>();
						for(auto i = 0; i < aliasesPackagesDataCount; ++i)
						{
							auto aliasId = serializer->ReadFormId();
							auto aliasesPackagesDataListCount = serializer->Read<size_t>();
							for(auto j = 0; j < aliasesPackagesDataListCount; ++j)
							{
								auto* package = reinterpret_cast<RE::TESPackage*>(serializer->ReadFormRef());
								auto* packageTemplate = reinterpret_cast<RE::TESPackage*>(serializer->ReadFormRef());
								SQG::FillPackageWithTemplate(package, packageTemplate, quest);

								const auto fragmentName = serializer->ReadString();

								SQG::DeserializePackageData(serializer, package);

								SQG::questsData[formId].aliasesPackagesData[aliasId].push_back({package, fragmentName});
							}
						}
					}

					const auto scriptCount = serializer->Read<size_t>();
					for(auto i = 0; i < scriptCount; ++i)
					{
						auto stringName = serializer->ReadString();
						const auto node = SQG::ScriptEngine::compiledScripts[stringName] = new caprica::papyrus::PapyrusCompilationNode(nullptr, stringName, "");
						node->createPexWriter();

						while(const auto heapSize = serializer->Read<size_t>())
						{
							for (size_t j = 0; j < heapSize; ++j) 
							{
					            node->getData().make<char>(serializer->Read<char>());
					        }
						}
					}

					const auto speakersCount = serializer->Read<size_t>();
					for(auto i = 0; i < speakersCount; ++i)
					{
						SQG::DeserializeDialogTopic(serializer, SQG::dialogTopicsData[serializer->ReadFormId()]);
					}

					auto forceGreetAnswersCount = serializer->Read<size_t>();
					for(int i = 0; i < forceGreetAnswersCount; ++i)
					{
						auto speakerFormId = serializer->ReadFormId();
						SQG::forceGreetAnswers[speakerFormId] = SQG::DeserializeAnswer(serializer, nullptr);
					}

					auto helloAnswersCount = serializer->Read<size_t>();
					for(auto i = 0; i < helloAnswersCount; ++i)
					{
						auto speakerFormId = serializer->ReadFormId();
						auto answersCount = serializer->Read<size_t>();
						for(auto j = 0; j < answersCount; ++j)
						{
							SQG::helloAnswers[speakerFormId].emplace_back(SQG::DeserializeAnswer(serializer, nullptr));
						}
					}
				}
				generatedQuestIndex = SQG::questsData.size();
			}
			else if(message->type == SKSE::MessagingInterface::kPostLoadGame)
			{
				auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				auto* policy = scriptMachine->GetObjectHandlePolicy();

				for(auto& [formId, data] : SQG::questsData)
				{
					const auto questHandle = policy->GetHandleForObject(RE::FormType::Quest, data.quest);
					RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
					scriptMachine->FindBoundObject(questHandle, data.quest->GetFormEditorID(), questCustomScriptObject);
					SQG::questsData[formId].script = questCustomScriptObject.get();

					SQG::PackageEngine::InitAliasPackages(formId);
				}

			}
			else if(message->type == SKSE::MessagingInterface::kSaveGame)
			{
				auto* serializer = DPF::SaveCache(message, true);

				serializer->Write<size_t>(SQG::questsData.size());
				for(auto& [formId, data] : SQG::questsData)
				{
					serializer->Write<RE::FormID>(formId);

					serializer->Write<size_t>(data.stages.size());
					for(const auto& stage : data.stages)
					{
						serializer->Write<RE::QUEST_STAGE_DATA>(stage->data);

						const auto hasLogEntry = stage->questStageItem != nullptr;
						serializer->Write<bool>(hasLogEntry);
						if(hasLogEntry)
						{
							serializer->WriteString(data.logEntries[stage->data.index].c_str());
						}

						if(auto it = SQG::questsData[formId].stagesToFragmentIndex.find(stage->data.index); it != SQG::questsData[formId].stagesToFragmentIndex.end())
						{
							serializer->Write<bool>(true);
							serializer->Write<int>(it->second);
						}
						else
						{
							serializer->Write(false);
						}
					}

					serializer->Write<size_t>(data.objectives.size());
					for(const auto& objective : data.objectives)
					{
						serializer->Write<uint16_t>(objective->objective.index);
						serializer->WriteString(objective->objective.displayText.c_str());
						serializer->Write<uint32_t>(objective->objective.numTargets);
						for(auto i = 0; i < objective->objective.numTargets; ++i)
						{
							serializer->Write<uint8_t>(objective->objective.targets[i]->alias);
						}
					}

					serializer->Write<size_t>(data.aliasesPackagesData.size());
					for(auto& [aliasId, aliasPackageDataList] : data.aliasesPackagesData)
					{
						serializer->WriteFormId(aliasId);
						serializer->Write<size_t>(aliasPackageDataList.size());
						for(auto& aliasPackageData : aliasPackageDataList)
						{
							serializer->WriteFormRef(aliasPackageData.package);
							serializer->WriteFormRef(reinterpret_cast<RE::TESCustomPackageData*>(aliasPackageData.package->data)->templateParent);
							serializer->WriteString(aliasPackageData.fragmentScriptName.c_str());
							SQG::SerializePackageData(serializer, aliasPackageData.package);
						}
					}
				}

				serializer->Write<size_t>(SQG::ScriptEngine::compiledScripts.size());
				for(const auto& [scriptName, node] : SQG::ScriptEngine::compiledScripts)
				{
					serializer->WriteString(scriptName.c_str());
					node->getPexWriter()->applyToBuffers([&](const char* scriptData, size_t size)
					{
						serializer->Write<size_t>(size);
				        for (size_t i = 0; i < size; ++i) 
						{
				            serializer->Write<char>(scriptData[i]);
				        }
					});
					serializer->Write<size_t>(0);
				}

				serializer->Write<size_t>(SQG::dialogTopicsData.size());
				for(const auto& [speakerFormId, data] : SQG::dialogTopicsData)
				{
					serializer->WriteFormId(speakerFormId);
					SQG::SerializeDialogTopic(serializer, data);
				}

				serializer->Write<size_t>(SQG::forceGreetAnswers.size());
				for(auto& [speakerFormId, answer] : SQG::forceGreetAnswers)
				{
					serializer->WriteFormId(speakerFormId);
					SQG::SerializeAnswer(serializer, answer);
				}

				serializer->Write<size_t>(SQG::helloAnswers.size());
				for(auto& [speakerFormId, answers] : SQG::helloAnswers)
				{
					serializer->WriteFormId(speakerFormId);
					serializer->Write<size_t>(answers.size());
					for(auto& answer : answers)
					{
						SQG::SerializeAnswer(serializer, answer);
					}
				}

				serializer->Close();
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
