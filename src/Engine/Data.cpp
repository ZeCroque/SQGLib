#include "Data.h"

#include "DPF/API.h"
#include "DPF/FileSystem.h"
#include "papyrus/PapyrusCompilationContext.h"
#include "SQG/API/ConditionUtils.h"
#include "SQG/API/PackageUtils.h"
#include "SQG/API/QuestUtils.h"

namespace SQG
{
	// Dialogs data
	// =======================
	DialogTopicData::AnswerData DeserializeAnswer(DPF::FileReader* inSerializer, DialogTopicData* inParent)
	{
		const auto targetStage = inSerializer->Read<int>();
		const auto answer = inSerializer->ReadString();
		const auto promptOverride = inSerializer->ReadString();
		const auto fragmentId = inSerializer->Read<int>();

		std::list<RE::TESConditionItem*> conditions;
		const auto conditionsCount = inSerializer->Read<size_t>();
		for(size_t j = 0; j < conditionsCount; ++j)
		{
			conditions.push_back(DeserializeCondition(inSerializer));
		}

		return DialogTopicData::AnswerData{ inParent, answer, promptOverride, conditions, targetStage , fragmentId, false };
	}

	void DeserializeDialogTopic(DPF::FileReader* inSerializer, DialogTopicData& outData)
	{
		outData.owningQuest = reinterpret_cast<RE::TESQuest*>(inSerializer->ReadFormRef());
		outData.prompt = inSerializer->ReadString();

		const auto answerCount = inSerializer->Read<size_t>();
		for(size_t i = 0; i < answerCount; ++i)
		{
			outData.answers.emplace_back(DeserializeAnswer(inSerializer, &outData));
		}

		const auto childCount = inSerializer->Read<size_t>();
		outData.childEntries.reserve(childCount);
		for(size_t i = 0; i < childCount; ++i)
		{
			DeserializeDialogTopic(inSerializer, *outData.childEntries.emplace_back(std::make_unique<DialogTopicData>()));
		}
	}

	void DeserializeDialog(DPF::FileReader* inSerializer)
	{
		auto* dataManager = SQG::DataManager::GetSingleton();

		const auto speakerFormId = inSerializer->ReadFormId();
		SQG::DeserializeDialogTopic(inSerializer, dataManager->dialogsData[speakerFormId].topLevelTopic);
		dataManager->dialogsData[speakerFormId].forceGreetAnswer = SQG::DeserializeAnswer(inSerializer, nullptr);
		const auto answersCount = inSerializer->Read<size_t>();
		for (size_t j = 0; j < answersCount; ++j)
		{
			dataManager->dialogsData[speakerFormId].helloAnswers.emplace_back(SQG::DeserializeAnswer(inSerializer, nullptr));
		}
	}

	void SerializeAnswer(DPF::FileWriter* inSerializer, const DialogTopicData::AnswerData& inAnswerData)
	{
		inSerializer->Write<int>(inAnswerData.targetStage);
		inSerializer->WriteString(inAnswerData.answer.c_str());
		inSerializer->WriteString(inAnswerData.promptOverride.c_str());
		inSerializer->Write<int>(inAnswerData.fragmentId);
		inSerializer->Write<size_t>(inAnswerData.conditions.size());
		for(const auto* condition : inAnswerData.conditions)
		{
			SerializeCondition(inSerializer, condition);
		}
	}

	void SerializeDialogTopic(DPF::FileWriter* inSerializer, const DialogTopicData& inData)
	{
		inSerializer->WriteFormRef(inData.owningQuest);
		inSerializer->WriteString(inData.prompt.c_str());
		inSerializer->Write<size_t>(inData.answers.size());
		for(auto& answer : inData.answers)
		{
			SerializeAnswer(inSerializer, answer);
		}

		inSerializer->Write<size_t>(inData.childEntries.size());
		for (auto& child : inData.childEntries)
		{
			SerializeDialogTopic(inSerializer, *child.get());
		}
	}

	void SerializeDialog(DPF::FileWriter* inSerializer, const RE::FormID inSpeakerId, const DialogData& inData)
	{
		inSerializer->WriteFormId(inSpeakerId);
		SQG::SerializeDialogTopic(inSerializer, inData.topLevelTopic);
		SQG::SerializeAnswer(inSerializer, inData.forceGreetAnswer);
		inSerializer->Write<size_t>(inData.helloAnswers.size());
		for (auto& answer : inData.helloAnswers)
		{
			SQG::SerializeAnswer(inSerializer, answer);
		}
	}

	// Quest Data
	// =======================
	QuestData::Objective::~Objective()
	{
		for(uint32_t i = 0; i < objective.numTargets; ++i)
		{
			delete objective.targets[i];
		}
		delete[] objective.targets;
	}

	void DeserializeQuestData(DPF::FileReader* inSerializer)
	{
		auto* dataManager = SQG::DataManager::GetSingleton();

		const auto formId = inSerializer->Read<RE::FormID>();
		auto* quest = DPF::GetForm<RE::TESQuest>(formId);
		dataManager->questsData[formId].quest = quest;

		const auto stagesCount = inSerializer->Read<size_t>();
		for (size_t j = 0; j < stagesCount; ++j)
		{
			const auto data = inSerializer->Read<RE::QUEST_STAGE_DATA>();
			const auto type = data.flags.all(RE::QUEST_STAGE_DATA::Flag::kStartUpStage) ? SQG::QuestStageType::Startup : (data.flags.all(RE::QUEST_STAGE_DATA::Flag::kShutDownStage) ? SQG::QuestStageType::Shutdown : SQG::QuestStageType::Default);

			std::string logEntry;
			if (inSerializer->Read<bool>())
			{
				logEntry = inSerializer->ReadString();
			}

			int fragmentIndex = -1;
			if (inSerializer->Read<bool>())
			{
				fragmentIndex = inSerializer->Read<int>();
			}

			SQG::AddQuestStage(quest, data.index, type, logEntry, fragmentIndex);
		}

		const auto objectiveCount = inSerializer->Read<size_t>();
		for (size_t j = 0; j < objectiveCount; ++j)
		{
			const auto index = inSerializer->Read<uint16_t>();

			const auto text = inSerializer->ReadString();

			const auto numTargets = inSerializer->Read<uint32_t>();
			std::list<uint8_t> questTargets;
			for (uint32_t k = 0; k < numTargets; ++k)
			{
				questTargets.emplace_back(inSerializer->Read<uint8_t>());
			}

			SQG::AddObjective(quest, index, text, questTargets);
		}

		const auto aliasesPackagesDataCount = inSerializer->Read<size_t>();
		for (size_t j = 0; j < aliasesPackagesDataCount; ++j)
		{
			auto aliasId = inSerializer->ReadFormId();
			const auto aliasesPackagesDataListCount = inSerializer->Read<size_t>();
			for (size_t k = 0; k < aliasesPackagesDataListCount; ++k)
			{
				auto* package = reinterpret_cast<RE::TESPackage*>(inSerializer->ReadFormRef());
				auto* packageTemplate = reinterpret_cast<RE::TESPackage*>(inSerializer->ReadFormRef());
				SQG::FillPackageWithTemplate(package, packageTemplate, quest);

				const auto fragmentName = inSerializer->ReadString();

				SQG::DeserializePackageData(inSerializer, package);

				dataManager->questsData[formId].aliasesPackagesData[aliasId].push_back({ package, fragmentName });
			}
		}
	}

	void SerializeQuestData(DPF::FileWriter* inSerializer, const RE::FormID inQuestId, QuestData& inData)
	{
		auto* dataManager = SQG::DataManager::GetSingleton();

		inSerializer->Write<RE::FormID>(inQuestId);

		inSerializer->Write<size_t>(inData.stages.size());
		for (const auto& stage : inData.stages)
		{
			inSerializer->Write<RE::QUEST_STAGE_DATA>(stage->data);

			const auto hasLogEntry = stage->questStageItem != nullptr;
			inSerializer->Write<bool>(hasLogEntry);
			if (hasLogEntry)
			{
				inSerializer->WriteString(inData.logEntries[stage->data.index].c_str());
			}

			if (auto it = dataManager->questsData[inData.quest->formID].stagesToFragmentIndex.find(stage->data.index); it != dataManager->questsData[inData.quest->formID].stagesToFragmentIndex.end())
			{
				inSerializer->Write<bool>(true);
				inSerializer->Write<int>(it->second);
			}
			else
			{
				inSerializer->Write(false);
			}
		}

		inSerializer->Write<size_t>(inData.objectives.size());
		for (const auto& objective : inData.objectives)
		{
			inSerializer->Write<uint16_t>(objective->objective.index);
			inSerializer->WriteString(objective->objective.displayText.c_str());
			inSerializer->Write<uint32_t>(objective->objective.numTargets);
			for (uint32_t i = 0; i < objective->objective.numTargets; ++i)
			{
				inSerializer->Write<uint8_t>(objective->objective.targets[i]->alias);
			}
		}

		inSerializer->Write<size_t>(inData.aliasesPackagesData.size());
		for (auto& [aliasId, aliasPackageDataList] : inData.aliasesPackagesData)
		{
			inSerializer->WriteFormId(aliasId);
			inSerializer->Write<size_t>(aliasPackageDataList.size());
			for (auto& [package, fragmentScriptName] : aliasPackageDataList)
			{
				inSerializer->WriteFormRef(package);
				inSerializer->WriteFormRef(reinterpret_cast<RE::TESCustomPackageData*>(package->data)->templateParent);
				inSerializer->WriteString(fragmentScriptName.c_str());
				SQG::SerializePackageData(inSerializer, package);
			}
		}
	}

	// Package Manager
	// =======================
	PackageData::PackageData()
	{
		std::memset(this, 0, sizeof(PackageData));  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess)
	}

	PackageData::PackageData(const RE::PackageLocation::Type inType, const RE::PackageLocation::Data& inData, std::uint32_t inRadius) : PackageData()
	{
		locationData.locType = inType;
		locationData.data = inData;
		locationData.rad = inRadius;
	}

	PackageData::PackageData(const RE::PackageTarget::Type inType, const RE::PackageTarget::Target& inData) : PackageData()
	{
		targetData.targType = inType;
		targetData.target = inData;
	}

	PackageData::PackageData(const RE::BGSNamedPackageData<RE::IPackageData>::Data& inData) : PackageData()
	{
		nativeData = inData;
	}

	PackageData::PackageData(RE::TESTopic* inTopic) : PackageData()
	{
		topicData = inTopic;
	}

	PackageData::~PackageData()  // NOLINT(modernize-use-equals-default)
	{
	}

	PackageData& PackageData::operator=(const PackageData& inOther)
	{
		if(this == &inOther)
		{
			return *this;
		}
		std::memcpy(this, &inOther, sizeof(PackageData));  // NOLINT(bugprone-undefined-memory-manipulation, clang-diagnostic-dynamic-class-memaccess)
		return *this;
	}

	void DeserializePackageData(DPF::FileReader* inSerializer, RE::TESPackage* outPackage)
	{
		const auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(outPackage->data);

		const int dataCount = inSerializer->Read<uint16_t>();
		for(auto i = 0; i < dataCount; ++i)
		{
			const auto dataUid = inSerializer->Read<int8_t>();
			for(uint16_t j = 0; j < customPackageData->data.dataSize; ++j)
			{
				if(dataUid == customPackageData->data.uids[j])
				{
					if(const auto packageDataTypeName = inSerializer->ReadString(); packageDataTypeName == "Location")
					{
						const auto* bgsPackageDataLocation = reinterpret_cast<RE::BGSPackageDataLocation*>(customPackageData->data.data[i] - 1);  // NOLINT(clang-diagnostic-reinterpret-base-class, bugprone-pointer-arithmetic-on-polymorphic-object)
						bgsPackageDataLocation->pointer->rad = inSerializer->Read<uint32_t>();
						bgsPackageDataLocation->pointer->locType = inSerializer->Read<REX::EnumSet<RE::PackageLocation::Type, std::uint8_t>>();
						if (bgsPackageDataLocation->pointer->locType == RE::PackageLocation::Type::kNearReference)
						{
							bgsPackageDataLocation->pointer->data.refHandle = reinterpret_cast<RE::TESObjectREFR*>(inSerializer->ReadFormRef())->GetHandle();
						}
						//TODO other types
					}
					else if(packageDataTypeName == "Topic")
					{
						const auto bgsPackageTopicData = reinterpret_cast<RE::BGSPackageDataTopic*>(customPackageData->data.data[i]);
						bgsPackageTopicData->unk00 = 0;
						bgsPackageTopicData->unk08 = 0;
						bgsPackageTopicData->topic = reinterpret_cast<RE::TESTopic*>(inSerializer->ReadFormRef());
					}
					else if(packageDataTypeName == "SingleRef")
					{
						const auto* bgsPackageDataSingleRef = reinterpret_cast<RE::BGSPackageDataSingleRef*>(customPackageData->data.data[i]);
						bgsPackageDataSingleRef->pointer->targType = RE::PackageTarget::Type::kNearReference;
						bgsPackageDataSingleRef->pointer->target.handle = reinterpret_cast<RE::TESObjectREFR*>(inSerializer->ReadFormRef())->GetHandle();
					}
					else if(packageDataTypeName == "TargetSelector")
					{
						const auto* bgsPackageDataTargetSelector = reinterpret_cast<RE::BGSPackageDataTargetSelector*>(customPackageData->data.data[i]);
						bgsPackageDataTargetSelector->pointer->targType = RE::PackageTarget::Type::kObjectType;
						bgsPackageDataTargetSelector->pointer->target.objType = inSerializer->Read<REX::EnumSet<RE::PACKAGE_OBJECT_TYPE, std::uint32_t>>();
					}
					else if(packageDataTypeName == "Bool")
					{
						const auto bgsPackageDataBool = reinterpret_cast<RE::BGSPackageDataBool*>(customPackageData->data.data[i]);
						bgsPackageDataBool->data.i = inSerializer->Read<uint32_t>();
					}
					else if(packageDataTypeName == "Int")
					{
						const auto bgsPackageDataInt = reinterpret_cast<RE::BGSPackageDataInt*>(customPackageData->data.data[i]);
						bgsPackageDataInt->data.i = inSerializer->Read<uint32_t>();
					}
					else if(packageDataTypeName == "Float")
					{
						const auto bgsPackageDataFloat = reinterpret_cast<RE::BGSPackageDataFloat*>(customPackageData->data.data[i]);
						bgsPackageDataFloat->data.f = inSerializer->Read<float>();
					}
					break;
				}
			}
		}

		RE::TESConditionItem** conditionItemHolder = &outPackage->packConditions.head;
		while(inSerializer->Read<bool>())
		{
			*conditionItemHolder = DeserializeCondition(inSerializer);
			conditionItemHolder = &(*conditionItemHolder)->next;
		}
	}

	void SerializePackageData(DPF::FileWriter* inSerializer, const RE::TESPackage* inPackage)
	{
		const auto* customPackageData = reinterpret_cast<RE::TESCustomPackageData*>(inPackage->data);

		inSerializer->Write<uint16_t>(customPackageData->data.dataSize);
		for(uint16_t i = 0; i < customPackageData->data.dataSize; ++i)
		{
			inSerializer->Write<int8_t>(customPackageData->data.uids[i]);

			const auto packageDataTypeName = customPackageData->data.data[i]->GetTypeName();
			inSerializer->WriteString(packageDataTypeName.c_str());
			if(packageDataTypeName == "Location")
			{
				const auto* bgsPackageDataLocation = reinterpret_cast<RE::BGSPackageDataLocation*>(customPackageData->data.data[i] - 1); // NOLINT(clang-diagnostic-reinterpret-base-class, bugprone-pointer-arithmetic-on-polymorphic-object)
				inSerializer->Write<uint32_t>(bgsPackageDataLocation->pointer->rad);
				inSerializer->Write<REX::EnumSet<RE::PackageLocation::Type, std::uint8_t>>(bgsPackageDataLocation->pointer->locType);
				if (bgsPackageDataLocation->pointer->locType == RE::PackageLocation::Type::kNearReference)
				{
					inSerializer->WriteFormRef(bgsPackageDataLocation->pointer->data.refHandle.get().get());
				}
				//TODO other types
			}
			else if(packageDataTypeName == "Topic")
			{
				const auto bgsPackageTopicData = reinterpret_cast<RE::BGSPackageDataTopic*>(customPackageData->data.data[i]);
				inSerializer->WriteFormRef(bgsPackageTopicData->topic);
			}
			else if(packageDataTypeName == "SingleRef")
			{
				const auto* bgsPackageDataSingleRef = reinterpret_cast<RE::BGSPackageDataSingleRef*>(customPackageData->data.data[i]);
				inSerializer->WriteFormRef(bgsPackageDataSingleRef->pointer->target.handle.get().get());
			}
			else if(packageDataTypeName == "TargetSelector")
			{
				const auto* bgsPackageDataTargetSelector = reinterpret_cast<RE::BGSPackageDataTargetSelector*>(customPackageData->data.data[i]);
				inSerializer->Write<REX::EnumSet<RE::PACKAGE_OBJECT_TYPE, std::uint32_t>>(bgsPackageDataTargetSelector->pointer->target.objType);
			}
			else if(packageDataTypeName == "Bool")
			{
				const auto bgsPackageDataBool = reinterpret_cast<RE::BGSPackageDataBool*>(customPackageData->data.data[i]);
				inSerializer->Write<uint32_t>(bgsPackageDataBool->data.i);
			}
			else if(packageDataTypeName == "Int")
			{
				const auto bgsPackageDataInt = reinterpret_cast<RE::BGSPackageDataInt*>(customPackageData->data.data[i]);
				inSerializer->Write<uint32_t>(bgsPackageDataInt->data.i);
			}
			else if(packageDataTypeName == "Float")
			{
				const auto bgsPackageDataFloat = reinterpret_cast<RE::BGSPackageDataFloat*>(customPackageData->data.data[i]);
				inSerializer->Write<float>(bgsPackageDataFloat->data.f);
			}
		}

		RE::TESConditionItem* const* conditionItemHolder = &inPackage->packConditions.head;
		while(*conditionItemHolder)
		{
			inSerializer->Write<bool>(true);
			SerializeCondition(inSerializer, *conditionItemHolder);
			conditionItemHolder = &(*conditionItemHolder)->next;
		}
		inSerializer->Write<bool>(false);
	}

	// Condition Data
	// =======================
	RE::TESConditionItem* DeserializeCondition(DPF::FileReader* inSerializer)
	{
		const auto function = inSerializer->Read<REX::EnumSet<RE::FUNCTION_DATA::FunctionID, std::uint16_t>>();
		const auto param = inSerializer->ReadFormRef();
		const auto opCode = inSerializer->Read<RE::CONDITION_ITEM_DATA::OpCode>();
		const auto comparisonValue = inSerializer->Read<RE::CONDITION_ITEM_DATA::GlobalOrFloat>();

		return CreateCondition(param, function, opCode, comparisonValue);
	}

	void SerializeCondition(DPF::FileWriter* inSerializer, const RE::TESConditionItem* inCondition)
	{
		inSerializer->Write<REX::EnumSet<RE::FUNCTION_DATA::FunctionID, std::uint16_t>>(inCondition->data.functionData.function);
		inSerializer->WriteFormRef(static_cast<RE::TESForm*>(inCondition->data.functionData.params[0]));
		inSerializer->Write<RE::CONDITION_ITEM_DATA::OpCode>(inCondition->data.flags.opCode);
		inSerializer->Write<RE::CONDITION_ITEM_DATA::GlobalOrFloat>(inCondition->data.comparisonValue);
	}

	// Script Data
	// =======================
	void DeserializeScript(DPF::FileReader* inSerializer)
	{
		auto* dataManager = SQG::DataManager::GetSingleton();

		const auto scriptName = inSerializer->ReadString();
		const auto node = dataManager->compiledScripts[scriptName] = new caprica::papyrus::PapyrusCompilationNode(nullptr, scriptName, "");
		node->createPexWriter();

		while (const auto heapSize = inSerializer->Read<size_t>())
		{
			for (size_t j = 0; j < heapSize; ++j)
			{
				node->getData().make<char>(inSerializer->Read<char>());
			}
		}
	}

	void SerializeScript(DPF::FileWriter* inSerializer, const std::string& inScriptName, const caprica::papyrus::PapyrusCompilationNode* inNode)
	{
		inSerializer->WriteString(inScriptName.c_str());
		inNode->getPexWriter()->applyToBuffers([&](const char* scriptData, const size_t size)
		{
			inSerializer->Write<size_t>(size);
			for (size_t i = 0; i < size; ++i)
			{
				inSerializer->Write<char>(scriptData[i]);
			}
		});
		inSerializer->Write<size_t>(0);
	}

	void Deserialize(DPF::FileReader* inSerializer)
	{
		const auto size = inSerializer->Read<size_t>();
		for (size_t i = 0; i < size; ++i)
		{
			SQG::DeserializeQuestData(inSerializer);
		}

		const auto scriptCount = inSerializer->Read<size_t>();
		for (size_t i = 0; i < scriptCount; ++i)
		{
			SQG::DeserializeScript(inSerializer);
		}

		const auto speakersCount = inSerializer->Read<size_t>();
		for (size_t i = 0; i < speakersCount; ++i)
		{
			SQG::DeserializeDialog(inSerializer);
		}
	}

	void Serialize(DPF::FileWriter* inSerializer)
	{
		auto* dataManager = SQG::DataManager::GetSingleton();

		inSerializer->Write<size_t>(dataManager->questsData.size());
		for (auto& [formId, data] : dataManager->questsData)
		{
			SQG::SerializeQuestData(inSerializer, formId, data);
		}

		inSerializer->Write<size_t>(dataManager->compiledScripts.size());
		for (const auto& [scriptName, node] : dataManager->compiledScripts)
		{
			SQG::SerializeScript(inSerializer, scriptName, node);
		}

		inSerializer->Write<size_t>(dataManager->dialogsData.size());
		for (const auto& [speakerFormId, data] : dataManager->dialogsData)
		{
			SQG::SerializeDialog(inSerializer, speakerFormId, data);
		}
	}

	// Data Manager
	// =======================
	void DataManager::InitData()
	{
		const auto dataHandler = RE::TESDataHandler::GetSingleton();

		forceGreetTopic = reinterpret_cast<RE::TESTopic*>(dataHandler->LookupForm(RE::FormID{ 0x00EAB4 }, "SQGLib.esp"));
		subTopicsInfos[0] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{ 0x00BF96 }, "SQGLib.esp"));
		subTopicsInfos[1] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{ 0x00BF99 }, "SQGLib.esp"));
		subTopicsInfos[2] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{ 0x00BF9C }, "SQGLib.esp"));
		subTopicsInfos[3] = reinterpret_cast<RE::TESTopicInfo*>(dataHandler->LookupForm(RE::FormID{ 0x00BF9F }, "SQGLib.esp"));

		impossibleCondition = new RE::TESConditionItem();
		impossibleCondition->data.dataID = std::numeric_limits<std::uint32_t>::max();
		impossibleCondition->data.functionData.function.reset(static_cast<RE::FUNCTION_DATA::FunctionID>(std::numeric_limits<std::uint16_t>::max()));
		impossibleCondition->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kIsXBox);
		impossibleCondition->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
		impossibleCondition->data.comparisonValue.f = 1.f;
		impossibleCondition->next = nullptr;

		activatePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{ 0x0019B2D });
		acquirePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{ 0x0019713 });
		travelPackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{ 0x0016FAA });
		forceGreetPackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{ 0x003C1C4 });

		referenceQuest = reinterpret_cast<RE::TESQuest*>(dataHandler->LookupForm(RE::FormID{ 0x003371 }, "SQGLib.esp"));
	}

	void DataManager::ClearSerializationData()
	{
		questsData.clear();
		dialogsData.clear();
		compiledScripts.clear();
		packagesFragmentName.clear();
	}
}
