#include "Serialization/Model.h"

#include "Serialization/FormRecord.h"

uint32_t lastFormId = 0;  // last mod
uint32_t firstFormId = 0;  // last mod

std::map<RE::FormID, FormRecord> formData;

void UpdateId() {
	std::ranges::for_each((formData | std::views::values), [&](const FormRecord& item) {
		if (item.formId > lastFormId) {

			lastFormId = item.formId;
		}
	});
	++lastFormId; 
}
