#pragma once

#include "hdr/sqlite_modern_cpp.h"

TArray<uint64> AllowedNoCollisionBuilders;

inline sqlite::database& GetDB()
{
	static sqlite::database db(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/NoCollision/NoCollision.db");
	return db;
}

inline void InitSqlLite()
{
	try
	{
		auto& db = GetDB();
		db << "create table if not exists NoCollision ("
			"SteamId integer primary key not null"
			");";
		db << "SELECT SteamId FROM NoCollision;"
			>> [&](const uint64& SteamID)
		{
			AllowedNoCollisionBuilders.Add(SteamID);
		};
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
}

inline void DBAddNoCollision(const uint64& SteamID)
{
	try
	{
		int count = 0;

		auto& db = GetDB();
		db << "SELECT count(1) FROM NoCollision WHERE SteamId = ?;" << SteamID >> count;
		if (count == 0)
		{
			db << "INSERT INTO NoCollision (SteamId) VALUES (?);" << SteamID;
		}
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
}

inline void DBRemoveNoCollision(const uint64& SteamID)
{
	try
	{
		int count = 0;

		auto& db = GetDB();
		db << "SELECT count(1) FROM NoCollision WHERE SteamId = ?;" << SteamID >> count;
		if (count == 0)
		{
			db << "DELETE FROM NoCollision WHERE SteamId = ?;" << SteamID;
		}
	}
	catch (const sqlite::sqlite_exception& exception)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
	}
}
