#pragma once

#include <mysql++11.h>

#include "IDatabase.h"

#pragma comment(lib, "mysqlclient.lib")

class MySql : public IDatabase
{
public:
	explicit MySql(std::string server, std::string username, std::string password, std::string db_name, std::string table_players)
		: table_players_(move(table_players))
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

			result = db_.query(fmt::format("CREATE TABLE IF NOT EXISTS {} ("
				"Id INT NOT NULL AUTO_INCREMENT,"
				"SteamId BIGINT(11) NOT NULL DEFAULT 0,"
				"Kits VARCHAR(768) NOT NULL DEFAULT '{{}}',"
				"Points INT DEFAULT 0,"
				"TotalSpent INT DEFAULT 0,"
				"PRIMARY KEY(Id),"
				"UNIQUE INDEX SteamId_UNIQUE (SteamId ASC));", table_players_));

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
			return db_.query(fmt::format("INSERT INTO {} (SteamId) VALUES ({});", table_players_, steam_id));
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
			const auto result = db_.query(fmt::format("SELECT count(1) FROM {} WHERE SteamId = {};", table_players_, steam_id)).get_value<int>();

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
			kits_config = db_.query(fmt::format("SELECT Kits FROM {} WHERE SteamId = {};", table_players_, steam_id)).get_value<std::string>();
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
			return db_.query(fmt::format("UPDATE {} SET Kits = '{}' WHERE SteamId = {};", table_players_, kits_data.c_str(), steam_id));
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
			return db_.query(fmt::format("UPDATE {} SET Kits = '{{}}';", table_players_));
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
			points = db_.query(fmt::format("SELECT Points FROM {} WHERE SteamId = {};", table_players_, steam_id)).get_value<int>();
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
			return db_.query(fmt::format("UPDATE {} SET Points = {} WHERE SteamId = {};", table_players_, amount, steam_id));
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
			return db_.query(fmt::format("UPDATE {} SET Points = Points + {} WHERE SteamId = {};", table_players_, amount, steam_id));
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
			return db_.query(fmt::format("UPDATE {} SET Points = Points - {} WHERE SteamId = {};", table_players_, amount, steam_id));
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
			return db_.query(fmt::format("UPDATE {} SET TotalSpent = {} WHERE SteamId = {}", table_players_, amount, steam_id));
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
			points = db_.query(fmt::format("SELECT TotalSpent FROM {} WHERE SteamId = {};", table_players_, steam_id)).get_value<int>();
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
			return db_.query(fmt::format("UPDATE {} SET Points = 0;"));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

private:
	daotk::mysql::connection db_;
	std::string table_players_;
};
