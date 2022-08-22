#include "PCH.h"

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

RE::TESQuest* referenceQuest = nullptr;
RE::TESQuest* generatedQuest = nullptr;
RE::TESQuest* editedQuest = nullptr;
RE::TESQuest* selectedQuest = nullptr;
RE::TESObjectREFR* targetForm = nullptr;
RE::TESObjectREFR* activator = nullptr;
RE::TESPackage* travelPackage = nullptr;

std::string GenerateQuest(RE::StaticFunctionTag*)
{
	//Create quest if needed
	//=======================
	if(generatedQuest)
	{
		return "Quest yet generated.";
	}
	auto* questFormFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESQuest>();
	selectedQuest = generatedQuest = questFormFactory->Create();

	//Parametrize quest
	//=======================
	generatedQuest->SetFormEditorID("SQGTestQuest");
	generatedQuest->fullName = "00_SQG_POC";
	generatedQuest->InitItem(); //Initializes formFlags
	generatedQuest->formFlags |= RE::TESForm::RecordFlags::kFormRetainsID | RE::TESForm::RecordFlags::kPersistent;
	generatedQuest->AddChange(RE::TESForm::ChangeFlags::kCreated); //Seems to save the whole quest data and hence supersede the others (except for stages and objective for some reason)
	generatedQuest->data.flags.set(RE::QuestFlag::kRunOnce);

	//Add stages
	//=======================
	auto* waitingLogEntries = new RE::TESQuestStageItem[2];
	std::memset(waitingLogEntries, 0, 2 * sizeof(RE::TESQuestStageItem));  // NOLINT(bugprone-undefined-memory-manipulation)

	generatedQuest->initialStage = new RE::TESQuestStage();
	std::memset(generatedQuest->initialStage, 0, sizeof(RE::TESQuestStage));
	generatedQuest->initialStage->data.index = 10;
	generatedQuest->initialStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kStartUpStage);
	generatedQuest->initialStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kKeepInstanceDataFromHereOn);
	generatedQuest->initialStage->questStageItem = waitingLogEntries + 1;
	generatedQuest->initialStage->questStageItem->owner = generatedQuest;
	generatedQuest->initialStage->questStageItem->owningStage = generatedQuest->initialStage;
	generatedQuest->initialStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff}; //TODO create real quest log entry

	generatedQuest->otherStages = new RE::BSSimpleList<RE::TESQuestStage*>();
	auto* lastQuestStage = new RE::TESQuestStage();
	lastQuestStage->data.index = 20;
	lastQuestStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kShutDownStage);
	lastQuestStage->questStageItem = waitingLogEntries;
	lastQuestStage->questStageItem->owner = generatedQuest;
	lastQuestStage->questStageItem->owningStage = lastQuestStage;
	lastQuestStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff};
	lastQuestStage->questStageItem->data = 1; //Means "Last stage"
	generatedQuest->otherStages->emplace_front(lastQuestStage);

	//Add objectives
	//=======================
	auto* questObjective = new RE::BGSQuestObjective();
	questObjective->index = 10;
	questObjective->displayText = "First objective";
	questObjective->ownerQuest = generatedQuest;
	questObjective->initialized = true; //Seems to be unused and never set by the game. Setting it in case because it is on data from CK.
	generatedQuest->objectives.push_front(questObjective);

	//Add aliases
	//=======================
	auto* rawCreatedAlias = new char[sizeof(RE::BGSRefAlias)];
	std::memcpy(rawCreatedAlias, referenceQuest->aliases[0], sizeof(RE::BGSRefAlias));  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess) //TODO relocate vtable and copy it from here instead of using reference quest
	auto* createdAlias = reinterpret_cast<RE::BGSRefAlias*>(rawCreatedAlias); 
	createdAlias->aliasID = 0;
	createdAlias->aliasName = "SQGTestAliasTarget";
	createdAlias->fillType = RE::BGSBaseAlias::FILL_TYPE::kForced;
	createdAlias->owningQuest = selectedQuest;
	createdAlias->fillData.forced = RE::BGSRefAlias::ForcedFillData{targetForm->CreateRefHandle()};
	selectedQuest->aliasAccessLock.LockForWrite();
	selectedQuest->aliases.push_back(createdAlias);
	selectedQuest->aliasAccessLock.UnlockForWrite();

	//Add target //TODO Investigate further this part as it is very likely bugged and related to the memory corruption and/or crashes
	//=======================
	const auto target = new RE::TESQuestTarget[3](); 
	target->alias = 0; 
	auto* firstObjective = *selectedQuest->objectives.begin();
	firstObjective->targets = new RE::TESQuestTarget*;
	std::memset(&target[1], 0, 2 * sizeof(RE::TESQuestTarget*));  // NOLINT(bugprone-undefined-memory-manipulation)
	*firstObjective->targets = target;
	firstObjective->numTargets = 1;

	//Sets some additional variables (pad24, pad22c and questObjective's pad04 among others)
	//=======================
	generatedQuest->InitializeData();

	//Add script logic
	//===========================
	auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	auto* policy = scriptMachine->GetObjectHandlePolicy();

	const auto selectedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, selectedQuest);
	//TODO!! try to call script compiler from c++ before loading custom script
	RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
	scriptMachine->CreateObjectWithProperties("SQGDebug", 1, questCustomScriptObject); //TODO defensive pattern against return value;
	scriptMachine->BindObject(questCustomScriptObject, selectedQuestHandle, false);
	policy->PersistHandle(selectedQuestHandle); //TODO check if useful

	RE::BSTSmartPointer<RE::BSScript::Object> referenceAliasBaseScriptObject;
	scriptMachine->CreateObject("SQGQuestTargetScript", referenceAliasBaseScriptObject);
	const auto questAliasHandle = selectedQuestHandle & 0x0000FFFFFFFF;
	scriptMachine->BindObject(referenceAliasBaseScriptObject, questAliasHandle, false);
	policy->PersistHandle(questAliasHandle); //TODO check if useful

	RE::BSScript::Variable propertyValue;
	propertyValue.SetObject(referenceAliasBaseScriptObject);
	scriptMachine->SetPropertyValue(questCustomScriptObject, "SQGTestAliasTarget", propertyValue);
	questCustomScriptObject->initialized = true; //Required when creating object with properties

	//Notify success
	//===========================
	std::ostringstream ss;
    ss << std::hex << generatedQuest->formID;
	const auto formId = ss.str();
	SKSE::log::debug("Generated quest with formID: {0}"sv, formId);
	return "Generated quest with formID: " + formId;
}

namespace RE
{
	struct TESQuestStageEvent
	{
	public:
		// members
		void* unkPtr;	// 00 //TODO check what it is (maybe QuestStageItem*?)
		FormID	formID;	// 04
		std::uint16_t  targetStage; // 08
	};
	static_assert(sizeof(TESQuestStageEvent) == 0x10);
}

class QuestStageEventSink final : public RE::BSTEventSink<RE::TESQuestStageEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestStageEvent* a_event, RE::BSTEventSource<RE::TESQuestStageEvent>* a_eventSource) override
	{
		auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* policy = scriptMachine->GetObjectHandlePolicy();

		const auto* updatedQuest = RE::TESForm::LookupByID<RE::TESQuest>(a_event->formID);
		if(updatedQuest != generatedQuest) //TODO find a proper way to bypass unwanted events
		{
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto updatedQuestHandle = policy->GetHandleForObject(RE::FormType::Quest, updatedQuest);
		RE::BSTSmartPointer<RE::BSScript::Object> questCustomScriptObject;
		scriptMachine->FindBoundObject(updatedQuestHandle, "SQGDebug", questCustomScriptObject);

		const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() +  a_event->targetStage / 10 - 1;
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
		scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);

		return RE::BSEventNotifyControl::kStop;
	}
};

namespace RE
{
	struct TESQuestInitEvent
	{
	public:
		// members
		FormID	formID;	// 00
	};
	static_assert(sizeof(TESQuestInitEvent) == 0x4);
}

enum class ObjectType
{
	kNONE = 0,
	kActivators = 1,
	kArmor = 2,
	kBooks = 3,
	kContainers = 4,
	kDoors = 5,
	kIngredients = 6,
	kLights = 7,
	kMiscellaneous = 8,
	kFlora = 9,
	kFurniture = 10,
	kAnyWeapons = 11,
	kAmmo = 12,
	kKeys = 13,
	kAlchemy = 14,
	kFood = 15,
	kCombatWearable = 16,
	kWearable = 17,
	kNONEWeapons = 18,
	kMeleeWeapons = 19,
	kRangedWeapons = 20,
	kAnySpells = 21,
	kTargetSpells = 22,
	kTouchSpells = 23,
	kSelfSpells  = 24,
	kAnyActors = 25,
	kBeds = 26,
	kChairs = 27,
	kShouts = 28
};

struct ObjectTypeWrapper
{
	SKSE::stl::enumeration<ObjectType, std::int8_t> objectType;		// 00
	std::uint8_t pad01;												// 01
	std::uint16_t unk02;											// 02
	std::uint32_t unk04;											// 04
};
static_assert(sizeof(ObjectTypeWrapper) == 0x8);

class PackageTarget
{
public:
	union Data
	{
		RE::TESForm* object;			//Target's class or keyword
		RE::ObjectRefHandle refHandle;	//Reference to target
		ObjectTypeWrapper objectType;	//Target's object type, only usable with TargetSelectors
	};

	// members
	std::uint64_t unk00;    // 00
	Data data;				// 08
	std::uint64_t unk10;    // 10
};
static_assert(sizeof(PackageTarget) == 0x18);

class BGSPackageDataTargetSelector : public RE::BGSPackageDataPointerTemplate<RE::IPackageData, PackageTarget>
{
public:
	inline static constexpr auto RTTI = RE::RTTI_BGSPackageDataTargetSelector;

	~BGSPackageDataTargetSelector() override;  // 00

	void Unk_06(void) override;									// 06
	void Unk_07(void) override;									// 07
	void LoadBuffer(RE::BGSLoadFormBuffer* a_buf) override;		// 08
	void Unk_09(void) override;									// 09
	bool GetDataAsString(RE::BSString* a_dst) const override;	// 0A
	void Unk_0C(void) override;									// 0C
};
static_assert(sizeof(BGSPackageDataTargetSelector) == 0x18);

class BGSPackageDataSingleRef : public RE::BGSPackageDataPointerTemplate<RE::IPackageData, PackageTarget>
{
public:
	inline static constexpr auto RTTI = RE::RTTI_BGSPackageDataRef;

	~BGSPackageDataSingleRef() override;  // 00

	void Unk_06(void) override;									// 06
	void Unk_07(void) override;									// 07
	void LoadBuffer(RE::BGSLoadFormBuffer* a_buf) override;		// 08
	void Unk_09(void) override;									// 09
	bool GetDataAsString(RE::BSString* a_dst) const override;	// 0A
	void Unk_0C(void) override;									// 0C
};
static_assert(sizeof(BGSPackageDataSingleRef) == 0x18);


class QuestInitEventSink : public RE::BSTEventSink<RE::TESQuestInitEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestInitEvent* a_event, RE::BSTEventSource<RE::TESQuestInitEvent>* a_eventSource) override
	{
		if(const auto* updatedQuest = RE::TESForm::LookupByID<RE::TESQuest>(a_event->formID); updatedQuest == generatedQuest) //TODO find a proper way to bypass unwanted events
		{
			if(const auto* aliasInstancesList = reinterpret_cast<RE::ExtraAliasInstanceArray*>(targetForm->extraList.GetByType(RE::ExtraDataType::kAliasInstanceArray)))
			{
				aliasInstancesList->lock.LockForRead();
				for(auto* aliasInstanceData : aliasInstancesList->aliases)
				{
					if(aliasInstanceData->quest == generatedQuest)
					{
						auto* instancedPackages = new RE::BSTArray<RE::TESPackage*>(); //Done in two time to deal with constness
						aliasInstanceData->instancedPackages = instancedPackages;

						auto* packageFormFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESPackage>();
						auto* package = packageFormFactory->Create();
						instancedPackages->push_back(package);
						package->ownerQuest = selectedQuest;
						package->procedureType = travelPackage->procedureType;
						package->packData.packType = RE::PACKAGE_PROCEDURE_TYPE::kPackage;
						package->packData.interruptOverrideType = RE::PACK_INTERRUPT_TARGET::kSpectator;
						package->packData.maxSpeed = RE::PACKAGE_DATA::PreferredSpeed::kRun;
						package->packData.foBehaviorFlags.set(RE::PACKAGE_DATA::InterruptFlag::kHellosToPlayer, RE::PACKAGE_DATA::InterruptFlag::kRandomConversations, RE::PACKAGE_DATA::InterruptFlag::kObserveCombatBehaviour, RE::PACKAGE_DATA::InterruptFlag::kGreetCorpseBehaviour, RE::PACKAGE_DATA::InterruptFlag::kReactionToPlayerActions, RE::PACKAGE_DATA::InterruptFlag::kFriendlyFireComments, RE::PACKAGE_DATA::InterruptFlag::kAggroRadiusBehavior, RE::PACKAGE_DATA::InterruptFlag::kAllowIdleChatter, RE::PACKAGE_DATA::InterruptFlag::kWorldInteractions); 

						std::memcpy(package->data, travelPackage->data, sizeof(RE::TESCustomPackageData)); // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess)
						auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(package->data);
						customPackageData->templateParent = travelPackage;

						for(auto i = 0; i < customPackageData->data.dataSize; ++i)
						{
							auto* packageData = customPackageData->data.data[i];

							if(packageData->GetTypeName() == "Location")
							{
								auto castedPackageData = reinterpret_cast<RE::BGSPackageDataLocation*>(packageData - 1);
								castedPackageData->pointer->locType = RE::PackageLocation::Type::kNearReference;
								castedPackageData->pointer->rad = 0;
								castedPackageData->pointer->data.refHandle = activator->CreateRefHandle();
							}
							//TODO other types...
							//TargetSelector and Ref can both take Any Object : ObjectMatching Id or Object matching type
							//Note the logic for Location/TargetSelector/Ref can both take direct ref and keyword ref
						}

						instancedPackages->push_back(package);
					}
				}
				aliasInstancesList->lock.UnlockForRead();
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}
};


std::string SwapSelectedQuest(RE::StaticFunctionTag*)
{
	selectedQuest = selectedQuest == referenceQuest ? generatedQuest : referenceQuest;
	return "Selected " + std::string(selectedQuest ? selectedQuest->GetFullName() : "nullptr");
}

void StartQuest(RE::TESQuest* inQuest)
{
	inQuest->Start();
	auto* storyTeller = RE::BGSStoryTeller::GetSingleton();
	storyTeller->BeginStartUpQuest(inQuest);
}

void StartSelectedQuest(RE::StaticFunctionTag*)
{
	if(selectedQuest)
	{
		StartQuest(selectedQuest);
	}
}

void DraftDebugFunction(RE::StaticFunctionTag*)
{
	//TODO!!! Add packages according to reference quest and do package fragment logic
	//TODO!! debug nvidia exception on close
}	

bool RegisterFunctions(RE::BSScript::IVirtualMachine* inScriptMachine)
{
	inScriptMachine->RegisterFunction("GenerateQuest", "SQGLib", GenerateQuest);
	inScriptMachine->RegisterFunction("SwapSelectedQuest", "SQGLib", SwapSelectedQuest);
	inScriptMachine->RegisterFunction("StartSelectedQuest", "SQGLib", StartSelectedQuest);
	inScriptMachine->RegisterFunction("DraftDebugFunction", "SQGLib", DraftDebugFunction);
	return true;
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
			if (message->type == SKSE::MessagingInterface::kDataLoaded)
			{
				auto* dataHandler = RE::TESDataHandler::GetSingleton();
				selectedQuest = editedQuest = reinterpret_cast<RE::TESQuest*>(dataHandler->LookupForm(RE::FormID{0x003e36}, "SQGLib.esp"));
				referenceQuest = reinterpret_cast<RE::TESQuest*>(dataHandler->LookupForm(RE::FormID{0x003371}, "SQGLib.esp"));
				targetForm = reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x00439A}, "SQGLib.esp"));
				activator =  reinterpret_cast<RE::TESObjectREFR*>(dataHandler->LookupForm(RE::FormID{0x001885}, "SQGLib.esp"));  
				travelPackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0016FAA});  //TODO parse a package template descriptor map

				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new QuestStageEventSink());
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new QuestInitEventSink());
			}
			else if(message->type == SKSE::MessagingInterface::kPostLoadGame)
			{
				StartQuest(referenceQuest);
			}
		})
	)
	{
	    return false;
	}
	
	return true;
}
