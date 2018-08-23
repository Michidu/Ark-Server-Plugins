#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <SQLiteCpp/Database.h>

#include <API/ARK/Ark.h>
#include <API/UE/Math/ColorList.h>

#include "json.hpp"

namespace ArkHome
{
	extern nlohmann::json config;

	SQLite::Database& GetDB();
	FString GetText(const std::string& str);
}
