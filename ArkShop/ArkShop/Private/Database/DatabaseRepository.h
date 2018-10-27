#pragma once
#include "../hdr/sqlite_modern_cpp.h"

namespace ArkShop::DatabaseRepository
{
	sqlite::database& GetDB();

	void CreateDatabase();
	bool TryAddNewPlayer(uint64 steam_id);

	std::string GetPlayerKits(uint64 steam_id);
	bool UpdatePlayerKits(uint64 steam_id, std::string kits_data);
	bool DeleteAllKits();

	int GetPoints(uint64 steam_id);
	bool SetPoints(uint64 steam_id, int amount);
	bool DeleteAllPoint();
}
