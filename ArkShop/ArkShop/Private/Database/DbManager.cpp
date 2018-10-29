#include "DbManager.h"

#include <Tools.h>


namespace ArkShop::DbManager
{
	sqlite::database& GetDb()
	{
		static sqlite::database db(config["General"].value("DbPathOverride", "").empty()
			? ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/ArkShop/ArkShop.db"
			: config["General"]["DbPathOverride"]);
		return db;
	}

	void CreateDatabase()
	{
		auto& db = GetDb();

		db << "create table if not exists Players ("
			"Id integer primary key autoincrement not null,"
			"SteamId integer default 0,"
			"Kits text default '{}',"
			"Points integer default 0"
			");";
	}

	bool TryAddNewPlayer(const uint64 steam_id)
	{
		auto& db = GetDb();

		try
		{
			db << "INSERT INTO Players (SteamId) VALUES (?);"
				<< steam_id;
			return true;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}
}
