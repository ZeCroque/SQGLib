#include "Serialization/Model.h"

#include "Serialization/FormRecord.h"

uint32_t lastFormId = 0;  // last mod
uint32_t firstFormId = 0;  // last mod

std::map<RE::FormID, FormRecord> formData;

RE::TESQuest* deserializedQuest = nullptr;

// Iterate over every registered forms
void EachFormData(std::function<bool(FormRecord&)> const& iteration) {
    for (auto& formRecord : formData | std::views::values) {
        if (!iteration(formRecord)) {
            return;
        }
    }
}

void UpdateId() {
    EachFormData([&](FormRecord& item) {
        if (item.formId > lastFormId) {
	        lastFormId = item.formId;
        }
        return true;
    });
    ++lastFormId;
}