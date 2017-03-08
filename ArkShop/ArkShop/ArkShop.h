#pragma once

#include "hdr/sqlite_modern_cpp.h"
#include "json.hpp"
#include "API/Base.h"

extern nlohmann::json json;

sqlite::database GetDB();