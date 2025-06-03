#include "Package.h"

#include "SQG/API/QuestUtils.h"

namespace SQG::Engine::Package
{
	// # Common
	// =======================
	RE::TESPackage* acquirePackage = nullptr;
	RE::TESPackage* activatePackage = nullptr;
	RE::TESPackage* travelPackage = nullptr;
	RE::TESPackage* forceGreetPackage = nullptr;

	std::map<RE::FormID, std::string> packagesFragmentName;

	// # Sinks
	// =======================

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
	class PackageEventSink final : public RE::BSTEventSink<RE::TESPackageEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESPackageEvent* inEvent, RE::BSTEventSource<RE::TESPackageEvent>*) override
		{
			if(inEvent->packageEventType == RE::TESPackageEvent::PackageEventType::kEnd)
			{
				if(const auto packageFragmentName = packagesFragmentName.find(inEvent->packageFormId); packageFragmentName != packagesFragmentName.end())
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


	void InitAliasPackages(const RE::FormID inQuestId)
	{
		if(const auto questData = questsData.find(inQuestId); questData != questsData.end())
		{
			for (const auto& [refId, aliasPackagesData] : questData->second.aliasesPackagesData)
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
									packagesFragmentName[package->formID] = fragmentScriptName;
								}
							}
						}
					}
					aliasInstancesList->lock.UnlockForWrite();
				}
			}
		}
	}

	void RegisterSinks()
	{
		questInitEventSink = std::make_unique<QuestInitEventSink>();
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(questInitEventSink.get());

		packageEventSink = std::make_unique<PackageEventSink>();
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(packageEventSink.get());
	}

	// # Data
	// =======================
	void LoadData()
	{
		activatePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0019B2D});
		acquirePackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0019713});
		travelPackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x0016FAA});
		forceGreetPackage = RE::TESForm::LookupByID<RE::TESPackage>(RE::FormID{0x003C1C4});
	}
}
