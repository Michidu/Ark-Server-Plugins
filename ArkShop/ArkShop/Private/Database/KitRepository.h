#pragma once

#include "../ArkShop.h"
#include "../hdr/sqlite_modern_cpp.h"

namespace ArkShop::KitRepository
{
	std::string GetPlayerKits(uint64 steam_id);
	bool UpdatePlayerKits(uint64 steam_id, std::string kits_data);
	bool DeleteAllKits();
}