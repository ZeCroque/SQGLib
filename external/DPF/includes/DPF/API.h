#pragma once

namespace DPF
{
	void Init(RE::FormID inFormId, const std::string& inPluginName);

	void SaveCache(const SKSE::MessagingInterface::Message* inMessage);

	void LoadCache(const SKSE::MessagingInterface::Message* inMessage);

	void DeleteCache(const SKSE::MessagingInterface::Message* inMessage);

	RE::TESForm* CreateForm(RE::TESForm* inModelForm);

	template <typename T> T* CreateForm(T* inModelForm)
	{
		return CreateForm(static_cast<RE::TESForm*>(inModelForm))->As<T>();
	}
}