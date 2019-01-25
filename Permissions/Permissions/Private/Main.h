#pragma once

#include "Database/IDatabase.h"

namespace Permissions
{
	inline std::unique_ptr<IDatabase> database;

	std::string GetDbPath();
}
