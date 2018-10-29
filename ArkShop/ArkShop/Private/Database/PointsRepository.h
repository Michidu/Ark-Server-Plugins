#pragma once

#include "../ArkShop.h"
#include "../hdr/sqlite_modern_cpp.h"

namespace ArkShop::PointsRepository
{
	int GetPoints(uint64 steam_id);
	bool SetPoints(uint64 steam_id, int amount);
	bool DeleteAllPoint();
}