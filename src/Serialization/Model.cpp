#include "Serialization/Model.h"

#include "Serialization/FormRecord.h"

// Iterate over every registered forms
void EachFormData(std::function<bool(FormRecord*)> const& iteration) {
    for (const auto& formRecord : formData | std::views::values) {
        if (!iteration(formRecord)) {
            return;
        }
    }
}

void UpdateId() {
    EachFormData([&](FormRecord* item) {
        if (item->formId > lastFormId) {
	        lastFormId = item->formId;
        }
        return true;
    });
    ++lastFormId;
}