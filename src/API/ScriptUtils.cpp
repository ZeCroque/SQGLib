#include "SQG/API/ScriptUtils.h"

#include "Engine/Script.h"

namespace SQG
{
	void AddScript(const std::string& inStringName, const std::string_view& inData)
	{
		Engine::Script::AddScript(inStringName, inData);
	}
}
