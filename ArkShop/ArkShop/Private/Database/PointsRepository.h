#pragma once

#include "../ArkShop.h"

namespace ArkShop::PointsRepository
{
	int GetPoints(uint64 steam_id);
	bool SetPoints(uint64 steam_id, int amount);
	bool DeleteAllPoint();
}