#pragma once

#include <API/ARK/Ark.h>

//#include "json.hpp"
#include "..\Public\json.hpp"

#define SAFEZONES_TEAM 28368

namespace SafeZones
{
	inline nlohmann::ordered_json config;

	FString GetText(const std::string& str);
	void ReadConfig();
}
