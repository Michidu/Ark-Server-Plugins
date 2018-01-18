#pragma once

#include <Logger/Logger.h>
#include <API/UE/Containers/FString.h>

#include "hdr/sqlite_modern_cpp.h"
#include "json.hpp"

namespace ArkShop
{
	extern nlohmann::json config;

	sqlite::database& GetDB();
	FString GetText(const std::string& str);
}
