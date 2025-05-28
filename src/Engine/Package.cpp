#include "Package.h"

#include "SQG/API/QuestUtils.h"

namespace SQG
{
	namespace PackageEngine
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
		class QuestInitEventSink : public RE::BSTEventSink<RE::TESQuestInitEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestInitEvent* a_event, RE::BSTEventSource<RE::TESQuestInitEvent>* a_eventSource) override
			{
				InitAliasPackages(a_event->formID);
				return RE::BSEventNotifyControl::kContinue;
			}
		};
		std::unique_ptr<QuestInitEventSink> questInitEventSink;

		// ## Package end sink
		// =======================
		class PackageEventSink : public RE::BSTEventSink<RE::TESPackageEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESPackageEvent* a_event, RE::BSTEventSource<RE::TESPackageEvent>* a_eventSource) override
			{
				if(a_event->packageEventType == RE::TESPackageEvent::PackageEventType::kEnd)
				{
					if(auto packageFragmentName = packagesFragmentName.find(a_event->packageFormId); packageFragmentName != packagesFragmentName.end())
					{
						auto* scriptMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
						auto* policy = scriptMachine->GetObjectHandlePolicy();
						
						auto* package = RE::TESForm::LookupByID<RE::TESPackage>(a_event->packageFormId);
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


		void InitAliasPackages(RE::FormID inQuestId)
		{
			if(const auto questData = questsData.find(inQuestId); questData != questsData.end())
			{
				for (const auto& aliasPackagesData : questData->second.aliasesPackagesData)
				{
					auto* alias = RE::TESForm::LookupByID<RE::TESObjectREFR>(aliasPackagesData.first);
					if(const auto* aliasInstancesList = reinterpret_cast<RE::ExtraAliasInstanceArray*>(alias->extraList.GetByType(RE::ExtraDataType::kAliasInstanceArray)))
					{
						aliasInstancesList->lock.LockForWrite();
						for(auto* aliasInstanceData : aliasInstancesList->aliases)
						{
							if(aliasInstanceData->quest == RE::TESForm::LookupByID<RE::TESQuest>(inQuestId))
							{
								auto* instancedPackages = new RE::BSTArray<RE::TESPackage*>(); //Done in two time to deal with constness
								aliasInstanceData->instancedPackages = instancedPackages;
								for(auto& aliasPackageData : aliasPackagesData.second)
								{
									instancedPackages->push_back(aliasPackageData.package);
									if(!aliasPackageData.fragmentScriptName.empty())
									{
										packagesFragmentName[aliasPackageData.package->formID] = aliasPackageData.fragmentScriptName;
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
}
