#pragma once

#pragma warning(push)
#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include <ranges>

#include <xbyak/xbyak.h>
#undef PAGE_EXECUTE_READWRITE
#undef GetModuleFileName
#undef GetEnvironmentVariable
#undef MessageBox
#undef VerQueryValue
#undef GetFileVersionInfo
#undef GetFileVersionInfoSize
#undef GetModuleHandle
#undef max
#undef min
#undef GetObject

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif
#pragma warning(pop)

using namespace std::literals;

namespace util
{
	using SKSE::stl::report_and_fail;
}

#define DLLEXPORT __declspec(dllexport)

#include "Plugin.h"
