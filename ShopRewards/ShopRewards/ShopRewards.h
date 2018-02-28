#pragma once

#include <API/ARK/Ark.h>
#include <API/UE/Math/ColorList.h>

#include "json.hpp"
#include "hdr/sqlite_modern_cpp.h"

extern nlohmann::json config;

FString GetText(const std::string& str);
sqlite::database& GetDB();