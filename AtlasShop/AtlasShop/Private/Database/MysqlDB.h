#pragma once

#include <mysql++11.h>

#include "IDatabase.h"

#pragma comment(lib, "mysqlclient.lib")

class MySql : public IDatabase
{
public:
	explicit MySql(std::string server, std::string username, std::string password, std::string db_name)
	{
		try
		{
			daotk::mysql::connect_options options;
			options.server = move(server);
			options.username = move(username);
			options.password = move(password);
			options.dbname = move(db_name);
			options.autoreconnect = true;
			options.timeout = 30;

			bool result = db_.open(options);
			if (!result)
			{
				Log::GetLog()->critical("Failed to open connection!");
			}

			result = db_.query("CREATE TABLE IF NOT EXISTS Players ("
				"Id INT NOT NULL AUTO_INCREMENT,"
				"SteamId BIGINT(11) NOT NULL DEFAULT 0,"
				"Kits VARCHAR(256) NOT NULL DEFAULT '{}',"
				"Points INT DEFAULT 0,"
				"TotalSpent INT DEFAULT 0,"
				"PRIMARY KEY(Id),"
				"UNIQUE INDEX SteamId_UNIQUE (SteamId ASC)); ");
			if (!result)
			{
				Log::GetLog()->critical("({} {}) Failed to create table!", __FILE__, __FUNCTION__);
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	bool TryAddNewPlayer(uint64 steam_id) override
	{
		try
		{
			return db_.query("INSERT INTO Players (SteamId) VALUES (%I64u);", steam_id);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool IsPlayerExists(uint64 steam_id) override
	{
		try
		{
			auto result = db_.query("SELECT count(1) FROM Players WHERE SteamId = %I64u;", steam_id).get_value<
				int>();
			return result > 0;
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	std::string GetPlayerKits(uint64 steam_id) override
	{
		std::string kits_config = "{}";

		try
		{
			kits_config = db_.query("SELECT Kits FROM Players WHERE SteamId = %I64u;", steam_id).get_value<std::
				string>();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return kits_config;
	}

	bool UpdatePlayerKits(uint64 steam_id, const std::string& kits_data) override
	{
		try
		{
			return db_.query("UPDATE Players SET Kits = '%s' WHERE SteamId = %I64u;", kits_data.c_str(),
			                 steam_id);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool DeleteAllKits() override
	{
		try
		{
			return db_.query("UPDATE Players SET Kits = \"{}\";");
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	int GetPoints(uint64 steam_id) override
	{
		int points = 0;

		try
		{
			points = db_.query("SELECT Points FROM Players WHERE SteamId = %I64u;", steam_id).get_value<int>();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return points;
	}

	bool SetPoints(uint64 steam_id, int amount) override
	{
		try
		{
			return db_.query("UPDATE Players SET Points = %d WHERE SteamId = %I64u;", amount, steam_id);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool AddPoints(uint64 steam_id, int amount) override
	{
		if (amount < 0)
			return false;

		try
		{
			return db_.query("UPDATE Players SET Points = Points + %d WHERE SteamId = %I64u;", amount,
			                 steam_id);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool SpendPoints(uint64 steam_id, int amount) override
	{
		if (amount < 0)
			return false;

		try
		{
			return db_.query("UPDATE Players SET Points = Points - %d WHERE SteamId = %I64u;", amount,
			                 steam_id);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool AddTotalSpent(uint64 steam_id, int amount) override
	{
		if (amount < 0)
			return false;

		try
		{
			return db_.query("UPDATE Players SET TotalSpent = %d WHERE SteamId = %I64u;", amount, steam_id);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	int GetTotalSpent(uint64 steam_id) override
	{
		int points = 0;

		try
		{
			points = db_.query("SELECT TotalSpent FROM Players WHERE SteamId = %I64u;", steam_id).get_value<int
			>();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return points;
	}

	bool DeleteAllPoints() override
	{
		try
		{
			return db_.query("UPDATE Players SET Points = 0;");
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

private:
	daotk::mysql::connection db_;
};
