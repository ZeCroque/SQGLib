#pragma once

template <class T> void CopyComponent(RE::TESForm* from, RE::TESForm* to);

//Copies component based on form type
inline void ApplyPattern(RE::TESForm* baseForm, RE::TESForm* newForm);

template<class T> T* CreateForm(T* baseItem, RE::FormID formId = 0);

#include "FormCreator.hpp"