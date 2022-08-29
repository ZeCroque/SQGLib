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
RE::TESPackage* customTravelPackage = nullptr;

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
	auto* logEntries = new RE::TESQuestStageItem[4];
	std::memset(logEntries, 0, 4 * sizeof(RE::TESQuestStageItem));  // NOLINT(bugprone-undefined-memory-manipulation)

	generatedQuest->initialStage = new RE::TESQuestStage();
	std::memset(generatedQuest->initialStage, 0, sizeof(RE::TESQuestStage));
	generatedQuest->initialStage->data.index = 10;
	generatedQuest->initialStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kStartUpStage);
	generatedQuest->initialStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kKeepInstanceDataFromHereOn);
	generatedQuest->initialStage->questStageItem = logEntries + 3;
	generatedQuest->initialStage->questStageItem->owner = generatedQuest;
	generatedQuest->initialStage->questStageItem->owningStage = generatedQuest->initialStage;
	generatedQuest->initialStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff}; //TODO create real quest log entry

	generatedQuest->otherStages = new RE::BSSimpleList<RE::TESQuestStage*>();

	auto* questStage = new RE::TESQuestStage();
	questStage->data.index = 40;
	questStage->data.flags.set(RE::QUEST_STAGE_DATA::Flag::kShutDownStage);
	questStage->questStageItem = logEntries + 2;
	questStage->questStageItem->owner = generatedQuest;
	questStage->questStageItem->owningStage = questStage;
	questStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff};
	questStage->questStageItem->data = 1; //Means "Last stage"
	generatedQuest->otherStages->emplace_front(questStage);

	questStage = new RE::TESQuestStage();
	questStage->data.index = 30;
	questStage->questStageItem = logEntries + 1;
	questStage->questStageItem->owner = generatedQuest;
	questStage->questStageItem->owningStage = questStage;
	questStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff};
	generatedQuest->otherStages->emplace_front(questStage);

	questStage = new RE::TESQuestStage();
	questStage->data.index = 20;
	questStage->questStageItem = logEntries;
	questStage->questStageItem->owner = generatedQuest;
	questStage->questStageItem->owningStage = questStage;
	questStage->questStageItem->logEntry  = RE::BGSLocalizedStringDL{0xffffffff};
	generatedQuest->otherStages->emplace_front(questStage);

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

		if(a_event->targetStage == 10)
		{
			const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter();
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}
		else if(a_event->targetStage == 40)
		{
			const auto* methodInfo = questCustomScriptObject->type->GetMemberFuncIter() + 1;
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(updatedQuestHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
		}

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

//TargetSelector and Ref can both take Any Object : ObjectMatching Id or Object matching type
//Note the logic for Location/TargetSelector/Ref can both take direct ref and keyword ref

class BGSPackageDataTargetSelector : public RE::BGSPackageDataPointerTemplate<RE::IPackageData, RE::PackageTarget>
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

class BGSPackageDataSingleRef : public RE::BGSPackageDataPointerTemplate<RE::IPackageData, RE::PackageTarget>
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

union PackageData
{
	using PackageNativeData = RE::BGSNamedPackageData<RE::IPackageData>::Data;

	PackageData();
	PackageData(RE::PackageLocation::Type inType, const RE::PackageLocation::Data& inData, std::uint32_t inRadius);
	PackageData(std::int8_t inType, const RE::PackageTarget::Target& inData);
	explicit PackageData(const RE::BGSNamedPackageData<RE::IPackageData>::Data& inData);
	~PackageData();
	PackageData& operator=(const PackageData& inOther);

	RE::PackageTarget targetData;
	RE::PackageLocation locationData;
	PackageNativeData nativeData{};	
};

PackageData::PackageData()
{
	std::memset(this, 0, sizeof(PackageData));  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess)
}

PackageData::PackageData(RE::PackageLocation::Type inType, const RE::PackageLocation::Data& inData, std::uint32_t inRadius) : PackageData()
{
	locationData.locType = inType;
	locationData.data = inData;
	locationData.rad = inRadius;
}

PackageData::PackageData(const std::int8_t inType, const RE::PackageTarget::Target& inData) : PackageData()
{
	targetData.targType = inType;
	targetData.target = inData;
}

PackageData::PackageData(const RE::BGSNamedPackageData<RE::IPackageData>::Data& inData) : PackageData()
{
	nativeData = inData;
}

PackageData::~PackageData()  // NOLINT(modernize-use-equals-default)
{
}

PackageData& PackageData::operator=(const PackageData& inOther)
{
	std::memcpy(this, &inOther, sizeof(PackageData));  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess)
	return *this;
}

RE::TESPackage* CreatePackageFromTemplate(RE::TESPackage* inPackageTemplate, RE::TESQuest* inOwnerQuest)
{
	auto* packageFormFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESPackage>();
	auto* package = packageFormFactory->Create();

	package->ownerQuest = inOwnerQuest;
	package->procedureType = inPackageTemplate->procedureType;
	package->packData.packType = RE::PACKAGE_PROCEDURE_TYPE::kPackage;
	package->packData.interruptOverrideType = RE::PACK_INTERRUPT_TARGET::kSpectator;
	package->packData.maxSpeed = RE::PACKAGE_DATA::PreferredSpeed::kRun;
	package->packData.foBehaviorFlags.set(RE::PACKAGE_DATA::InterruptFlag::kHellosToPlayer, RE::PACKAGE_DATA::InterruptFlag::kRandomConversations, RE::PACKAGE_DATA::InterruptFlag::kObserveCombatBehaviour, RE::PACKAGE_DATA::InterruptFlag::kGreetCorpseBehaviour, RE::PACKAGE_DATA::InterruptFlag::kReactionToPlayerActions, RE::PACKAGE_DATA::InterruptFlag::kFriendlyFireComments, RE::PACKAGE_DATA::InterruptFlag::kAggroRadiusBehavior, RE::PACKAGE_DATA::InterruptFlag::kAllowIdleChatter, RE::PACKAGE_DATA::InterruptFlag::kWorldInteractions); 

	std::memcpy(package->data, inPackageTemplate->data, sizeof(RE::TESCustomPackageData)); // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess)
	auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(package->data);
	customPackageData->templateParent = inPackageTemplate;

	return package;
}

void FillPackageData(const RE::TESPackage* outPackage, const std::unordered_map<std::string, PackageData>& inPackageDataMap) //TODO test if we can generalize the package location enum
{
	const auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(outPackage->data);
	for(const auto& [name, packageData] : inPackageDataMap)
	{
		int count = 0;
		for(auto& nameMapData : customPackageData->nameMap->nameMap)
		{
			if(nameMapData.name.c_str() == name)
			{
				const auto packageDataTypeName = customPackageData->data.data[count]->GetTypeName();

				if(packageDataTypeName == "Location")
				{
					const auto* bgsPackageDataLocation = reinterpret_cast<RE::BGSPackageDataLocation*>(customPackageData->data.data[count] - 1);
					bgsPackageDataLocation->pointer->locType = packageData.locationData.locType;
					bgsPackageDataLocation->pointer->rad = packageData.locationData.rad;
					bgsPackageDataLocation->pointer->data = packageData.locationData.data;
				}
				else if(packageDataTypeName == "Ref")
				{
					const auto* bgsPackageDataSingleRef = reinterpret_cast<BGSPackageDataSingleRef*>(customPackageData->data.data[count]);
					bgsPackageDataSingleRef->pointer->targType = packageData.targetData.targType;
					bgsPackageDataSingleRef->pointer->target = packageData.targetData.target;
				}
				else if(packageDataTypeName == "TargetSelector")
				{
					const auto* bgsPackageDataTargetSelector = reinterpret_cast<BGSPackageDataTargetSelector*>(customPackageData->data.data[count]);
					bgsPackageDataTargetSelector->pointer->targType = packageData.targetData.targType;
					bgsPackageDataTargetSelector->pointer->target = packageData.targetData.target;
				}
				else if(packageDataTypeName == "Bool")
				{
					const auto bgsPackageDataBool = reinterpret_cast<RE::BGSPackageDataBool*>(customPackageData->data.data[count]);
					bgsPackageDataBool->data.b = packageData.nativeData.b;
				}
				else if(packageDataTypeName == "Int")
				{
					const auto bgsPackageDataInt = reinterpret_cast<RE::BGSNamedPackageData<RE::IPackageData>*>(customPackageData->data.data[count]);
					bgsPackageDataInt->data.i = packageData.nativeData.i;
				}
				else if(packageDataTypeName == "Float")
				{
					const auto bgsPackageDataFloat = reinterpret_cast<RE::BGSNamedPackageData<RE::IPackageData>*>(customPackageData->data.data[count]);
					bgsPackageDataFloat->data.f = packageData.nativeData.f;
				}
	
				RE::BSString result;
				customPackageData->data.data[count]->GetDataAsString(&result);
				SKSE::log::debug("package-data[{0}]-as-string:{1}"sv, packageDataTypeName, result);
			}
			++count;
		}
	}
}

struct PackageConditionDescriptor
{
	RE::FUNCTION_DATA::FunctionID functionId;
	RE::TESForm* functionCaller;
	RE::CONDITION_ITEM_DATA::OpCode opCode;
	bool useGlobal;
	RE::CONDITION_ITEM_DATA::GlobalOrFloat data;
	bool isOr;
};

void FillPackageCondition(RE::TESPackage* inPackage, const std::list<PackageConditionDescriptor>& packageConditionDescriptors)
{
	RE::TESConditionItem** conditionItemHolder = &inPackage->packConditions.head;
	for(auto& packageConditionDescriptor : packageConditionDescriptors)
	{
		auto conditionItem = *conditionItemHolder = new RE::TESConditionItem();
		conditionItem->data.dataID = std::numeric_limits<std::uint32_t>::max();
		std::memset(reinterpret_cast<void*>(&inPackage->packConditions.head->data.functionData.function), 0, sizeof(inPackage->packConditions.head->data.functionData.function.underlying()));

		conditionItem->data.functionData.function.set(packageConditionDescriptor.functionId);
		conditionItem->data.functionData.params[0] = packageConditionDescriptor.functionCaller;
		conditionItem->data.flags.opCode = packageConditionDescriptor.opCode;
		conditionItem->data.flags.global = packageConditionDescriptor.useGlobal;
		conditionItem->data.comparisonValue = packageConditionDescriptor.data;
		conditionItem->data.flags.isOR = packageConditionDescriptor.isOr;

		conditionItemHolder = &conditionItem->next;
	}

}

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
						customTravelPackage = CreatePackageFromTemplate(travelPackage, generatedQuest);


						std::unordered_map<std::string, PackageData> packageDataMap;
						RE::PackageLocation::Data locationData{};
						locationData.refHandle = activator->CreateRefHandle();
						packageDataMap["Place to Travel"] = PackageData(RE::PackageLocation::Type::kNearReference, locationData, 0);
						FillPackageData(customTravelPackage, packageDataMap);

						/*std::list<PackageConditionDescriptor> packageConditionList;
						RE::CONDITION_ITEM_DATA::GlobalOrFloat conditionItemData{};
						conditionItemData.f = 20.f;
						packageConditionList.emplace_back(RE::FUNCTION_DATA::FunctionID::kGetStage, generatedQuest, RE::CONDITION_ITEM_DATA::OpCode::kEqualTo, false, conditionItemData, false);
						FillPackageCondition(package, packageConditionList);*/

						auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
						auto* policy = scriptMachine->GetObjectHandlePolicy();

						const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, customTravelPackage);
						RE::BSTSmartPointer<RE::BSScript::Object> packageCustomScriptObject;
						scriptMachine->CreateObject("PF_SQGTravelPackage", packageCustomScriptObject); //TODO defensive pattern against return value;
						scriptMachine->BindObject(packageCustomScriptObject, packageHandle, false);
						policy->PersistHandle(packageHandle); //TODO check if useful

						auto* instancedPackages = new RE::BSTArray<RE::TESPackage*>(); //Done in two time to deal with constness
						aliasInstanceData->instancedPackages = instancedPackages;
						instancedPackages->push_back(customTravelPackage);
					}
				}
				aliasInstancesList->lock.UnlockForRead();
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}
};

namespace RE
{
	struct TESPackageEvent
	{
	public:
		// members
		RE::TESObjectREFR* owner;												// 00
		RE::FormID packageFormId;												// 08
		bool isEndEvent;														// 0C (maybe flag for begin end etc...)
	};
	static_assert(sizeof(TESPackageEvent) == 0x10);
}

class PackageEventSink : public RE::BSTEventSink<RE::TESPackageEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESPackageEvent* a_event, RE::BSTEventSource<RE::TESPackageEvent>* a_eventSource) override
	{
		if(customTravelPackage && a_event->packageFormId == customTravelPackage->formID && a_event->isEndEvent)
		{
			auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* policy = scriptMachine->GetObjectHandlePolicy();

			const auto customTravelPackageHandle = policy->GetHandleForObject(RE::FormType::Package, customTravelPackage);
			RE::BSTSmartPointer<RE::BSScript::Object> customTravelPackageScriptObject;
			scriptMachine->FindBoundObject(customTravelPackageHandle, "PF_SQGTravelPackage", customTravelPackageScriptObject);
			const auto* methodInfo = customTravelPackageScriptObject->type->GetMemberFuncIter();
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
			scriptMachine->DispatchMethodCall(customTravelPackageHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
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
	generatedQuest->currentStage = 20;
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
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(new PackageEventSink());
			}
		})
	)
	{
	    return false;
	}
	
	return true;
}
