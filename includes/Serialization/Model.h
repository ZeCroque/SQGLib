#pragma once

class FormRecord;

extern uint32_t lastFormId;  // last mod
extern uint32_t firstFormId;  // last mod

extern std::map<RE::FormID, FormRecord> formData;

extern RE::TESQuest* deserializedQuest;

// Iterate over every registered forms
void EachFormData(std::function<bool(FormRecord&)> const& iteration);

void UpdateId();