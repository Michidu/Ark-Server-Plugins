#pragma once

#include "ShopRewards.h"

namespace Stats
{
	void Init();

	bool IsPlayerExists(uint64 steam_id);

	bool AddPlayerKill(uint64 steam_id);
	bool AddPlayerDeath(uint64 steam_id);
	bool AddWildDinoKill(uint64 steam_id);
	bool AddTamedDinoKill(uint64 steam_id);
}
