#include "Package.h"

#include "SQG/API/Data.h"

namespace SQG::Engine::Package
{
	// # Common
	// =======================

	// This methods will add the custom packages we want to a given quest alias. We have to wait for the game to create the ExtraAliasInstanceArray inside our quest alias to be able to do it so it's called in two situations:
	//	- When the quest has just been generated and goes through its init process
	//  - After a savegame has been successfully loaded
	void InitAliasPackages(const RE::FormID inQuestId)
	{
		const auto* dataManager = DataManager::GetSingleton();
		if(const auto questData = dataManager->questsData.find(inQuestId); questData != dataManager->questsData.end())
		{
			for(const auto& [refId, aliasPackagesData] : questData->second.aliasesPackagesData)
			{
				auto* alias = RE::TESForm::LookupByID<RE::TESObjectREFR>(refId);
				if(const auto* aliasInstancesList = reinterpret_cast<RE::ExtraAliasInstanceArray*>(alias->extraList.GetByType(RE::ExtraDataType::kAliasInstanceArray)))
				{
					aliasInstancesList->lock.LockForWrite();
					for(auto* aliasInstanceData : aliasInstancesList->aliases)
					{
						if(aliasInstanceData->quest == RE::TESForm::LookupByID<RE::TESQuest>(inQuestId))
						{
							auto* instancedPackages = new RE::BSTArray<RE::TESPackage*>(); //Done in two time to deal with constness
							aliasInstanceData->instancedPackages = instancedPackages;
							for(const auto& [package, fragmentScriptName] : aliasPackagesData)
							{
								instancedPackages->push_back(package);
								if(!fragmentScriptName.empty())
								{
									DataManager::GetSingleton()->packagesFragmentName[package->formID] = fragmentScriptName;
								}
							}
						}
					}
					aliasInstancesList->lock.UnlockForWrite();
				}
			}
		}
	}

	// # Sinks
	// =======================
	namespace
	{
		// ## Quest init sink
		// =======================
		class QuestInitEventSink final : public RE::BSTEventSink<RE::TESQuestInitEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestInitEvent* inEvent, RE::BSTEventSource<RE::TESQuestInitEvent>*) override
			{
				Engine::Package::InitAliasPackages(inEvent->formID);
				return RE::BSEventNotifyControl::kContinue;
			}
		};
		std::unique_ptr<QuestInitEventSink> questInitEventSink;

		// ## Package end sink
		// =======================

		// The fragment system can't be manipulated easily in current version of CommonLibSSE (AFAIK) so we're just faking one by reacting to PackageEvents
		class PackageEventSink final : public RE::BSTEventSink<RE::TESPackageEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESPackageEvent* inEvent, RE::BSTEventSource<RE::TESPackageEvent>*) override
			{
				if(inEvent->packageEventType == RE::TESPackageEvent::PackageEventType::kEnd)
				{
					auto* dataManager = DataManager::GetSingleton();
					if(const auto packageFragmentName = dataManager->packagesFragmentName.find(inEvent->packageFormId); packageFragmentName != dataManager->packagesFragmentName.end())
					{
						auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
						auto* policy = scriptMachine->GetObjectHandlePolicy();

						const auto* package = RE::TESForm::LookupByID<RE::TESPackage>(inEvent->packageFormId);
						const auto packageHandle = policy->GetHandleForObject(RE::FormType::Package, package);
						RE::BSTSmartPointer<RE::BSScript::Object> packageScriptObject;
						scriptMachine->FindBoundObject(packageHandle, packageFragmentName->second.c_str(), packageScriptObject);
						const auto* methodInfo = packageScriptObject->type->GetMemberFuncIter();
						RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> stackCallbackFunctor;
						scriptMachine->DispatchMethodCall(packageHandle, methodInfo->func->GetObjectTypeName(), methodInfo->func->GetName(), RE::MakeFunctionArguments(), stackCallbackFunctor);
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
		std::unique_ptr<PackageEventSink> packageEventSink;
	}

	void RegisterSinks()
	{
		questInitEventSink = std::make_unique<QuestInitEventSink>();
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(questInitEventSink.get());

		packageEventSink = std::make_unique<PackageEventSink>();
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(packageEventSink.get());
	}
}
