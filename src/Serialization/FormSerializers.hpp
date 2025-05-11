#pragma once

#include "Serialization/FormRecord.h"

REGISTER_SERIALIZERS(Name, Quest, Spell, Ammo, SoulGem, Enchantment, Value, Weight, MagicItem, Enchantable, Armor, Weapon, SpellTome, LeveledItem, ProduceForm)

template<class T> void NameSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form) {
    auto name = form->As<RE::TESFullName>()->fullName.c_str();
    serializer->WriteString(name);
}
template<class T> void NameSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form) {
    auto name = serializer->ReadString();
    auto named = form->As<RE::TESFullName>();
    if (named) {
        named->fullName = name;
    }
}
template<class T> bool NameSerializer<T>::Condition(RE::TESForm* form) { return form->As<RE::TESFullName>(); }


template<class T> void QuestSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form) {
    auto sourceQuest = form->As<RE::TESQuest>();
    if (sourceQuest) 
    {
         serializer->WriteString(sourceQuest->GetFormEditorID());
    }
}
template<class T> void QuestSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form) {
    auto target = form->As<RE::TESQuest>();
    if (target) 
    {
        target->SetFormEditorID(serializer->ReadString().c_str());
    }
}
template<class T> bool QuestSerializer<T>::Condition(RE::TESForm* form) { return form->As<RE::TESQuest>(); }


template<class T> void SpellSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto spell = form->As<RE::SpellItem>();
    if (spell) {
        serializer->Write<char>(((spell->data.flags & RE::SpellItem::SpellFlag::kCostOverride) == RE::SpellItem::SpellFlag::kCostOverride)?1:0);
        serializer->Write<int32_t>(spell->data.costOverride);
        serializer->Write<float>(spell->data.chargeTime);
        serializer->Write<float>(spell->data.castDuration);
        serializer->Write<float>(spell->data.range);
        serializer->WriteFormRef(spell->data.castingPerk);
    }
}
template<class T> void SpellSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto spell = form->As<RE::SpellItem>();
    if (spell) {

        char costOverride = serializer->Read<char>();

        if (costOverride == 1) {
            spell->data.flags |= RE::SpellItem::SpellFlag::kCostOverride;
        } else {
            spell->data.flags &= static_cast<RE::SpellItem::SpellFlag>(
                ~static_cast<std::uint32_t>(RE::SpellItem::SpellFlag::kCostOverride));
        }

        spell->data.costOverride = serializer->Read<int32_t>();
        spell->data.chargeTime = serializer->Read<float>();
        spell->data.castDuration = serializer->Read<float>();
        spell->data.range = serializer->Read<float>();
        spell->data.castingPerk = serializer->ReadFormRef<RE::BGSPerk>();
    }
}
template<class T> bool SpellSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::SpellItem>(); }


template <typename T> void AmmoSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto ammo = form->As<RE::TESAmmo>();
    if (ammo) {
        serializer->Write<float>(ammo->data.damage);
        serializer->WriteFormRef(ammo->data.projectile);
    }
}
template <typename T> void AmmoSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto ammo = form->As<RE::TESAmmo>();
    if (ammo) {
        ammo->data.damage = serializer->Read<float>();
        auto projectile = serializer->ReadFormRef<RE::BGSProjectile>();
        if (projectile) {
            ammo->data.projectile = projectile;
        }
    }
}
template <typename T> bool AmmoSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::TESAmmo>(); }


template <typename T> void SoulGemSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto soulGem = form->As<RE::TESSoulGem>();
    if (soulGem) {
        serializer->Write<uint8_t>(static_cast<uint8_t>(soulGem->currentSoul.get()));
        serializer->Write<uint8_t>(static_cast<uint8_t>(soulGem->soulCapacity.get()));
        serializer->WriteFormRef(soulGem->linkedSoulGem);
    }
}
template <typename T> void SoulGemSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto soulGem = form->As<RE::TESSoulGem>();
    if (soulGem) {
        soulGem->currentSoul = static_cast<RE::SOUL_LEVEL>(serializer->Read<uint8_t>());
        soulGem->soulCapacity = static_cast<RE::SOUL_LEVEL>(serializer->Read<uint8_t>());
        auto target = serializer->ReadFormRef<RE::TESSoulGem>();
        if (target) {
            soulGem->linkedSoulGem = target;
        }
    }
}
template <typename T> bool SoulGemSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::TESSoulGem>(); }


template <typename T> void EnchantmentSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto enchantment = form->As<RE::EnchantmentItem>();
    if (enchantment) {
        serializer->Write<char>(((enchantment->data.flags & RE::EnchantmentItem::EnchantmentFlag::kCostOverride) ==
                                 RE::EnchantmentItem::EnchantmentFlag::kCostOverride)
                                    ? 1
                                    : 0);

        serializer->Write<int32_t>(enchantment->data.costOverride);
        serializer->Write<int32_t>(enchantment->data.chargeOverride);
        serializer->Write<float>(enchantment->data.chargeTime);
    }
}
template <typename T> void EnchantmentSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto enchantment = form->As<RE::EnchantmentItem>();
    if (enchantment) {
        char costOverride = serializer->Read<char>();

        if (costOverride == 1) {
            enchantment->data.flags |= RE::EnchantmentItem::EnchantmentFlag::kCostOverride;
        } else {
            enchantment->data.flags &= static_cast<RE::EnchantmentItem::EnchantmentFlag>(
                ~static_cast<std::uint32_t>(RE::EnchantmentItem::EnchantmentFlag::kCostOverride));
        }

        enchantment->data.costOverride = serializer->Read<int32_t>();
        enchantment->data.chargeOverride = serializer->Read<int32_t>();
        enchantment->data.chargeTime = serializer->Read<float>();
    }
}
template <typename T> bool EnchantmentSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::EnchantmentItem>(); }


template <typename T> void ValueSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
        auto name = form->GetGoldValue();
        serializer->Write<int32_t>(name);
    }
template <typename T> void ValueSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
        auto value = serializer->Read<int32_t>();
        auto named = form->As<RE::TESValueForm>();
        if (named && value) {
            named->value = value;
        }
    }
template <typename T> bool ValueSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::TESValueForm>(); }


template <typename T> void WeightSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto name = form->GetWeight();
    serializer->Write<float>(name);
}
template <typename T> void WeightSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto weight = serializer->Read<float>();
    auto named = form->As<RE::TESWeightForm>();
    if (named && weight) {
        named->weight = weight;
    }
}
template <typename T> bool WeightSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::TESWeightForm>(); }


template <typename T> void MagicItemSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto potion = form->As<RE::MagicItem>();
    if (potion) {
        size_t length = potion->effects.size();
        serializer->Write<uint32_t>(static_cast<uint32_t>(length));
        for (uint32_t i = 0; i < length; i++) {
            auto effect = potion->effects[i];
            serializer->WriteFormRef(effect->baseEffect);
            serializer->Write<float>(effect->effectItem.magnitude);
            serializer->Write<uint32_t>(effect->effectItem.area);
            serializer->Write<uint32_t>(effect->effectItem.duration);
            serializer->Write<float>(effect->cost);
        }

    }
}
template <typename T> void MagicItemSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto potion = form->As<RE::MagicItem>();
    if (potion) {
        potion->effects.clear();
        size_t numEffects = serializer->Read<uint32_t>();

        for (size_t i = 0; i < numEffects; i++) {
            auto effectForm = serializer->ReadFormRef<RE::EffectSetting>();
            auto magnitude = serializer->Read<float>();
            auto area = serializer->Read<uint32_t>();
            auto duration = serializer->Read<uint32_t>();
            auto cost = serializer->Read<float>();

            if (effectForm) {
                bool found = false;
                for (auto effect : potion->effects) {
                    if (effect->baseEffect && effect->baseEffect->GetFormID() == effectForm->GetFormID()) {
                        effect->effectItem.magnitude = magnitude;
                        effect->effectItem.area = area;
                        effect->effectItem.duration = duration;
                        effect->cost = cost;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    auto newEffect = new RE::Effect();
                    newEffect->baseEffect = effectForm;
                    newEffect->effectItem.magnitude = magnitude;
                    newEffect->effectItem.area = area;
                    newEffect->effectItem.duration = duration;
                    newEffect->cost = cost;
                    potion->effects.push_back(newEffect);
                }
            }
        }
    }
}
template <typename T> bool MagicItemSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::MagicItem>(); }


template <typename T> void EnchantableSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto enchanted = form->As<RE::TESEnchantableForm>();
    if (enchanted && enchanted->formEnchanting) {
        serializer->Write<char>(1);
        serializer->WriteFormRef(enchanted->formEnchanting);
        serializer->Write<uint16_t>(enchanted->amountofEnchantment);

    } else {
        serializer->Write<char>(0);
    }
}
template <typename T> void EnchantableSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto enchanted = form->As<RE::TESEnchantableForm>();
    int32_t hasEffects = serializer->Read<char>();
    if (hasEffects == 1) {
        auto enchantment = serializer->ReadFormRef<RE::EnchantmentItem>();
        auto ammount = serializer->Read<uint16_t>();
        if (enchantment) {
            enchanted->formEnchanting = enchantment;
            enchanted->amountofEnchantment = ammount;
        }
    }
}
template <typename T> bool EnchantableSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::TESEnchantableForm>(); }


template <typename T> void ArmorSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto* amor = form->As<RE::TESObjectARMO>();
    serializer->Write<uint32_t>(amor->armorRating);
}
template <typename T> void ArmorSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto armorRating = serializer->Read<uint32_t>();
    auto* target = form->As<RE::TESObjectARMO>();
    if (target) {
        target->armorRating = armorRating;
    }
}
template <typename T> bool ArmorSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::TESObjectARMO>(); }


template <typename T> void WeaponSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto sourceModelWeapon = form->As<RE::TESObjectWEAP>();
    if (sourceModelWeapon) {
        serializer->WriteFormRef(sourceModelWeapon->criticalData.effect);
        serializer->Write<uint16_t>(sourceModelWeapon->attackDamage);
        serializer->Write<uint16_t>(sourceModelWeapon->criticalData.damage);
        serializer->Write<float>(sourceModelWeapon->weaponData.speed);
        serializer->Write<float>(sourceModelWeapon->weaponData.reach);
        serializer->Write<float>(sourceModelWeapon->weaponData.minRange);
        serializer->Write<float>(sourceModelWeapon->weaponData.maxRange);
        serializer->Write<float>(sourceModelWeapon->weaponData.staggerValue);
    }
}
template <typename T> void WeaponSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto target = form->As<RE::TESObjectWEAP>();
    if (target) {
        auto effect = serializer->ReadFormRef<RE::SpellItem>();
        if (effect) {
            target->criticalData.effect = effect;
        }
        target->attackDamage = serializer->Read<uint16_t>();
        target->criticalData.damage = serializer->Read<uint16_t>();;
        target->weaponData.speed = serializer->Read<float>();
        target->weaponData.reach = serializer->Read<float>();
        target->weaponData.minRange = serializer->Read<float>();
        target->weaponData.maxRange = serializer->Read<float>();
        target->weaponData.staggerValue = serializer->Read<float>();
    }
}
template <typename T> bool WeaponSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::TESObjectWEAP>(); }


template <typename T> void SpellTomeSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto book = form->As<RE::TESObjectBOOK>();
    serializer->WriteFormRef(book->data.teaches.spell);
}
template <typename T> void SpellTomeSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto spell = serializer->ReadFormRef<RE::SpellItem>();
    auto book = form->As<RE::TESObjectBOOK>();

    if (spell && book) {
        book->data.flags = RE::OBJ_BOOK::Flag::kTeachesSpell;
        book->data.teaches.spell = spell;
    }
}
template <typename T> bool SpellTomeSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::TESObjectBOOK>(); }


template <typename T> void LeveledItemSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto leveledList = form->As<RE::TESLeveledList>(); 
    if (leveledList) {
        serializer->Write<int8_t>(leveledList->chanceNone);
        serializer->Write<uint8_t>(leveledList->numEntries);
        //serializer->Write<uint8_t>(static_cast<uint8_t>(leveledList->llFlags));
        serializer->WriteFormRef(leveledList->chanceGlobal);
        auto size = static_cast<uint32_t>(leveledList->entries.size());
        serializer->Write<uint32_t>(size);
        for (auto item : leveledList->entries) {
            serializer->Write<uint16_t>(item.count);
            serializer->Write<uint16_t>(item.level);
            serializer->WriteFormRef(item.form);
        }
    }
}
template <typename T> void LeveledItemSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto leveledList = form->As<RE::TESLeveledList>();
    if (leveledList) {
        leveledList->chanceNone = serializer->Read<int8_t>();
        leveledList->numEntries = serializer->Read<uint8_t>();
        //leveledList->llFlags = static_cast<RE::TESLeveledList::Flag>(serializer->Read<uint8_t>());
        auto global = serializer->ReadFormRef<RE::TESGlobal>();
        if (global){
            leveledList->chanceGlobal = global;
        }
        auto size = serializer->Read<uint32_t>();
        leveledList->entries.resize(static_cast<size_t>(size));
        for (uint32_t i = 0; i < size; ++i) {
            leveledList->entries[i] = RE::LEVELED_OBJECT();
            auto count = serializer->Read<uint16_t>();
            auto level = serializer->Read<uint16_t>();
            auto newForm = serializer->ReadFormRef();
            if (form) {
                leveledList->entries[i].count = count;
                leveledList->entries[i].level = level;
                leveledList->entries[i].form = newForm;
            }
        }
    }

}
template <typename T> bool LeveledItemSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::TESLeveledList>(); }


template<class T> void ProduceFormSerializer<T>::Serialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto produceForm = form->As<RE::TESProduceForm>(); 
    if (produceForm) {
        serializer->WriteFormRef(produceForm->harvestSound);
        serializer->WriteFormRef(produceForm->produceItem);
    }
}
template<class T> void ProduceFormSerializer<T>::Deserialize(Serializer<T>* serializer, RE::TESForm* form)  {
    auto produceForm = form->As<RE::TESProduceForm>();
    if (produceForm) {
        auto sound = serializer->ReadFormRef<RE::BGSSoundDescriptorForm>();
        auto item = serializer->ReadFormRef<RE::TESBoundObject>();
        if (sound) {
            produceForm->harvestSound = sound;
        }
        if (item) {
            produceForm->produceItem = item;
        }
    }
}
template<class T> bool ProduceFormSerializer<T>::Condition(RE::TESForm* form)  { return form->As<RE::TESProduceForm>(); }



template <typename T> void StoreEachFormData(Serializer<T>* serializer, FormRecord& elem) {

    if (!serializer) {
        return;
    }
    auto& filters = GetFilters<T>();

    serializer->Write<uint32_t>(static_cast<uint32_t>(filters.size()));

    if(std::none_of(filters.begin(), filters.end(), [&](std::unique_ptr<FormSerializer<T>>& filter)
    {
        if (elem.actualForm && filter->Condition(elem.actualForm)) {
            serializer->StartWritingSection();
            serializer->Write<char>(1);
            filter->Serialize(serializer, elem.actualForm);
            serializer->finishWritingSection();
            return true;
        }
	    return false;
    }))
    {
        serializer->StartWritingSection();
        serializer->Write<char>(0);
        serializer->finishWritingSection();
    }
}

template <typename T> void RestoreEachFormData(Serializer<T>* serializer, FormRecord& elem) {

     if (!serializer) {
         return;
     }

    auto& filters = GetFilters<T>();

     auto size = serializer->Read<uint32_t>();
     uint32_t i = 0;

     for (auto& filter : filters) {
         if (i >= size) {
             break;
         }
         serializer->startReadingSection();
         auto kind = serializer->Read<char>();
         if (kind == 1 && elem.actualForm) {
            filter->Deserialize(serializer, elem.actualForm);
         }
         serializer->finishReadingSection();
         ++i;
     }
 }