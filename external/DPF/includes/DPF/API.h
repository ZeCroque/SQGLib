#pragma once

namespace DPF
{
	void Init(RE::FormID inFormId, const std::string& inPluginName);

	void SaveCache(std::string name);

	void LoadCache(std::string name);

	void DeleteCache(std::string name);

	RE::TESForm* CreateForm(RE::TESForm* inModelForm);

	template <typename T> T* CreateForm(T* inModelForm)
	{
		return CreateForm(static_cast<RE::TESForm*>(inModelForm))->As<T>();
	}
}