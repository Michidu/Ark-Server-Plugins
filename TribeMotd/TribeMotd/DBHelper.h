#pragma once

#include "TribeMotd.h"

namespace DBHelper
{
	inline bool IsEntryExists(int tribe_id)
	{
		int count = 0;

		try
		{
			auto& db = GetDB();

			db << "SELECT count(1) FROM TribeMotd WHERE TribeId = ?;" << tribe_id >> count;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return count != 0;
	}
	
	inline bool IsPlayerEntryExists(uint64 steam_id)
	{
		int count = 0;

		try
		{
			auto& db = GetDB();

			db << "SELECT count(1) FROM PlayerMotd WHERE SteamId = ?;" << steam_id >> count;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return count != 0;
	}
}
