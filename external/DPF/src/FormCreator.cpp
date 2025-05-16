#include "FormCreator.h"

#include "FormRecord.h"
#include "Model.h"

namespace DPF
{
	template <class T> static void CopyComponent(RE::TESForm* from, RE::TESForm* to)
	{
	    auto fromT = from->As<T>();
	    auto toT = to->As<T>();
	    if (fromT && toT) {
	        toT->CopyComponent(fromT);
	    }
	}

	static void CopyFormArmorModel(RE::TESForm* source, RE::TESForm* target) {

	    if (const auto sourceModelBipedForm = source->As<RE::TESObjectARMO>(), targeteModelBipedForm = target->As<RE::TESObjectARMO>(); sourceModelBipedForm && targeteModelBipedForm) {
	        targeteModelBipedForm->armorAddons = sourceModelBipedForm->armorAddons;
	    }
	}

	static void CopyFormObjectWeaponModel(RE::TESForm* source, RE::TESForm* target) {
	    if (const auto sourceModelWeapon = source->As<RE::TESObjectWEAP>(), targeteModelWeapon = target->As<RE::TESObjectWEAP>(); sourceModelWeapon && targeteModelWeapon) {
	        targeteModelWeapon->firstPersonModelObject = sourceModelWeapon->firstPersonModelObject;
	        targeteModelWeapon->attackSound = sourceModelWeapon->attackSound;
	        targeteModelWeapon->attackSound2D = sourceModelWeapon->attackSound2D;
	        targeteModelWeapon->attackSound = sourceModelWeapon->attackSound;
	        targeteModelWeapon->attackFailSound = sourceModelWeapon->attackFailSound;
	        targeteModelWeapon->idleSound = sourceModelWeapon->idleSound;
	        targeteModelWeapon->equipSound = sourceModelWeapon->equipSound;
	        targeteModelWeapon->unequipSound = sourceModelWeapon->unequipSound;
	        targeteModelWeapon->soundLevel = sourceModelWeapon->soundLevel;
	    }
	}


	static void CopyMagicEffect(RE::TESForm* source, RE::TESForm* target) {
	    if (const auto sourceEffect = source->As<RE::EffectSetting>(), targetEffect = target->As<RE::EffectSetting>(); sourceEffect && targetEffect) {
	        targetEffect->effectSounds = sourceEffect->effectSounds;
	        targetEffect->data.castingArt = sourceEffect->data.castingArt;
	        targetEffect->data.light = sourceEffect->data.light;
	        targetEffect->data.hitEffectArt = sourceEffect->data.hitEffectArt;
	        targetEffect->data.effectShader = sourceEffect->data.effectShader;
	        targetEffect->data.hitVisuals = sourceEffect->data.hitVisuals;
	        targetEffect->data.enchantShader = sourceEffect->data.enchantShader;
	        targetEffect->data.enchantEffectArt = sourceEffect->data.enchantEffectArt;
	        targetEffect->data.enchantVisuals = sourceEffect->data.enchantVisuals;
	        targetEffect->data.projectileBase = sourceEffect->data.projectileBase;
	        targetEffect->data.explosion = sourceEffect->data.explosion;
	        targetEffect->data.impactDataSet = sourceEffect->data.impactDataSet;
	        targetEffect->data.imageSpaceMod = sourceEffect->data.imageSpaceMod;
	    }
	}


	static void CopyBookAppearence(RE::TESForm* source, RE::TESForm* target) {
	    if (const auto sourceBook = source->As<RE::TESObjectBOOK>(), targetBook = target->As<RE::TESObjectBOOK>(); sourceBook && targetBook) {
	        targetBook->inventoryModel = sourceBook->inventoryModel;
	    }
	}

	static void CopyAppearence(RE::TESForm* source, RE::TESForm* target)
	{
	    CopyFormArmorModel(source, target);
	    CopyFormObjectWeaponModel(source, target);
	    CopyMagicEffect(source, target);
	    CopyBookAppearence(source, target);
	    CopyComponent<RE::BGSPickupPutdownSounds>(source, target);
	    CopyComponent<RE::BGSMenuDisplayObject>(source, target);
	    CopyComponent<RE::TESModel>(source, target);
	    CopyComponent<RE::TESBipedModelForm>(source, target);
	}

	//Copies component based on form type
	void ApplyPattern(FormRecord& instance)
	{
	    if (const auto newForm = instance.actualForm, baseForm = instance.baseForm; newForm) {
	        if (const auto weaponBaseForm = baseForm->As<RE::TESObjectWEAP>(), weaponNewForm = newForm->As<RE::TESObjectWEAP>(); weaponNewForm && weaponBaseForm) {
	            weaponNewForm->firstPersonModelObject = weaponBaseForm->firstPersonModelObject;

	            weaponNewForm->weaponData = weaponBaseForm->weaponData;
	            weaponNewForm->criticalData = weaponBaseForm->criticalData;

	            weaponNewForm->attackSound = weaponBaseForm->attackSound;
	            weaponNewForm->attackSound2D = weaponBaseForm->attackSound2D;
	            weaponNewForm->attackSound = weaponBaseForm->attackSound;
	            weaponNewForm->attackFailSound = weaponBaseForm->attackFailSound;
	            weaponNewForm->idleSound = weaponBaseForm->idleSound;
	            weaponNewForm->equipSound = weaponBaseForm->equipSound;
	            weaponNewForm->unequipSound = weaponBaseForm->unequipSound;
	            weaponNewForm->soundLevel = weaponBaseForm->soundLevel;

	            weaponNewForm->impactDataSet = weaponBaseForm->impactDataSet;
	            weaponNewForm->templateWeapon = weaponBaseForm->templateWeapon;
	            weaponNewForm->embeddedNode = weaponBaseForm->embeddedNode;

	        } else if (const auto bookBaseForm = baseForm->As<RE::TESObjectBOOK>(), bookNewForm = newForm->As<RE::TESObjectBOOK>(); bookBaseForm && bookNewForm) {
	            bookNewForm->data.flags = bookBaseForm->data.flags;
	            bookNewForm->data.teaches.spell = bookBaseForm->data.teaches.spell;
	            bookNewForm->data.teaches.actorValueToAdvance = bookBaseForm->data.teaches.actorValueToAdvance;
	            bookNewForm->data.type = bookBaseForm->data.type;
	            bookNewForm->inventoryModel = bookBaseForm->inventoryModel;
	            bookNewForm->itemCardDescription = bookBaseForm->itemCardDescription;
	        } else if (const auto ammoBaseForm = baseForm->As<RE::TESAmmo>(), ammoNewForm = newForm->As<RE::TESAmmo>();ammoBaseForm && ammoNewForm) {
	            ammoNewForm->data.damage = ammoBaseForm->data.damage;
	            ammoNewForm->data.flags = ammoBaseForm->data.flags;
	            ammoNewForm->data.projectile = ammoBaseForm->data.projectile;
	        }
	        else {
	            newForm->Copy(baseForm);
	        }

	        CopyComponent<RE::TESDescription>(baseForm, newForm);
	        CopyComponent<RE::BGSKeywordForm>(baseForm, newForm);
	        CopyComponent<RE::BGSPickupPutdownSounds>(baseForm, newForm);
	        CopyComponent<RE::TESModelTextureSwap>(baseForm, newForm);
	        CopyComponent<RE::TESModel>(baseForm, newForm);
	        CopyComponent<RE::BGSMessageIcon>(baseForm, newForm);
	        CopyComponent<RE::TESIcon>(baseForm, newForm);
	        CopyComponent<RE::TESFullName>(baseForm, newForm);
	        CopyComponent<RE::TESValueForm>(baseForm, newForm);
	        CopyComponent<RE::TESWeightForm>(baseForm, newForm);
	        CopyComponent<RE::BGSDestructibleObjectForm>(baseForm, newForm);
	        CopyComponent<RE::TESEnchantableForm>(baseForm, newForm);
	        CopyComponent<RE::BGSBlockBashData>(baseForm, newForm);
	        CopyComponent<RE::BGSEquipType>(baseForm, newForm);
	        CopyComponent<RE::TESAttackDamageForm>(baseForm, newForm);
	        CopyComponent<RE::TESBipedModelForm>(baseForm, newForm);
	    
		    if (instance.modelForm) {
		        CopyAppearence(instance.modelForm, newForm);
		    }
	    }
	}

	RE::TESForm* AddForm(RE::TESForm* baseItem, RE::FormID formId)
	{
		RE::TESForm* result = nullptr;
	    if (!baseItem)
	    {
		    return nullptr;
	    }
	    if(std::ranges::any_of(formData | std::views::values, [&](FormRecord& item) {
	        if (item.deleted) {
	            auto factory = RE::IFormFactory::GetFormFactoryByType(baseItem->GetFormType());
	            result = factory->Create();
	            result->SetFormID(item.formId, false);
	            item.Undelete(result, baseItem->GetFormType());
	            item.baseForm = baseItem;
	            ApplyPattern(item);
	            return true;
	        }
	        return false;
	    }))
		{
	        return result;
	    }

	    auto factory = RE::IFormFactory::GetFormFactoryByType(baseItem->GetFormType());
		result = factory->Create();

	    RE::FormID newFormId;
	    if(formId > 0)
	    {
			newFormId = formId;   
	    }
	    else
	    {
	        newFormId = lastFormId;
		    ++lastFormId;
	    }
		result->SetFormID(newFormId, false);
	    auto slot = FormRecord::CreateNew(result, baseItem->GetFormType(), newFormId);
	    slot.baseForm = baseItem;
	    ApplyPattern(slot);
	    formData[newFormId] = slot;
	    return result;
	}
}