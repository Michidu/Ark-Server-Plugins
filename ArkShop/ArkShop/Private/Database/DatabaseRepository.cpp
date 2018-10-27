#include <Tools.h>

#include "../ArkShop.h"
#include "DatabaseRepository.h"

namespace ArkShop::DatabaseRepository
{
	sqlite::database& DatabaseRepository::GetDB()
	{
		static sqlite::database db(config["General"].value("DbPathOverride", "").empty()
			? ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/ArkShop/ArkShop.db"
			: config["General"]["DbPathOverride"]);
		return db;
	}

	void DatabaseRepository::CreateDatabase()
	{
		auto& db = GetDB();

		db << "create table if not exists Players ("
			"Id integer primary key autoincrement not null,"
			"SteamId integer default 0,"
			"Kits text default '{}',"
			"Points integer default 0"
			");";
	}


	bool DatabaseRepository::TryAddNewPlayer(const uint64 steam_id)
	{
		auto& db = GetDB();

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

	std::string DatabaseRepository::GetPlayerKits(uint64 steam_id)
	{
		auto& db = GetDB();

		std::string kits_config = "{}";

		try
		{
			db << "SELECT Kits FROM Players WHERE SteamId = ?;" << steam_id >> kits_config;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return kits_config;
	}

	bool DatabaseRepository::UpdatePlayerKits(uint64 steam_id, const std::string kits_data)
	{
		auto& db = GetDB();

		try
		{
			db << "UPDATE Players SET Kits = ? WHERE SteamId = ?;" << kits_data << steam_id;
			return true;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool DatabaseRepository::DeleteAllKits()
	{
		auto& db = GetDB();
		
		try
		{
			db << "UPDATE Players SET Kits = \"{}\";";
			return true;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	int DatabaseRepository::GetPoints(const uint64 steam_id)
	{
		auto& db = GetDB();
		int points = 0;

		try
		{
			db << "SELECT Points FROM Players WHERE SteamId = ?;" << steam_id >> points;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return points;
	}

	bool DatabaseRepository::SetPoints(const uint64 steam_id, const int amount)
	{
		auto& db = GetDB();

		try
		{
			db << "UPDATE Players SET Points = ? WHERE SteamId = ?;" << amount << steam_id;
			return true;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool DatabaseRepository::DeleteAllPoint()
	{
		auto& db = GetDB();

		try
		{
			db << "UPDATE Players SET Points = 0;";
			return true;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
		
	}

}
