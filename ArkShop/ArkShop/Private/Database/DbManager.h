#pragma once
#include "../hdr/sqlite_modern_cpp.h"
#include "../ArkShop.h"

namespace ArkShop::DbManager
{
	sqlite::database& GetDb();
	void CreateDatabase();

	bool TryAddNewPlayer(uint64 steam_id);
}
