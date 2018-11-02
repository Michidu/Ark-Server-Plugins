#pragma once

#include <API/UE/Containers/FString.h>

#include "json.hpp"

namespace ArkShop
{
	extern nlohmann::json config;

	FString GetText(const std::string& str);
} // namespace ArkShop
