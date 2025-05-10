#pragma once

static uint32_t lastFormId = 0;  // last mod
static uint32_t firstFormId = 0;  // last mod

class FormRecord;

static std::map<RE::FormID, FormRecord*> formData;

static RE::TESQuest* deserializedQuest = nullptr;

// Iterate over every registered forms
void EachFormData(std::function<bool(FormRecord*)> const& iteration);

void UpdateId();