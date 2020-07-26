#pragma once

#include "Database/IDatabase.h"
#include "Cache/Cache.h"

namespace Permissions
{
	inline std::unique_ptr<IDatabase> database;

	std::string GetDbPath();

	inline bool use_cache = true;
}
