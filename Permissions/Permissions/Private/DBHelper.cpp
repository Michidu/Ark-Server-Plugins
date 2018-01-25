#include "../Public/DBHelper.h"

#include "Main.h"

namespace Permissions::DB
{
	bool IsPlayerExists(uint64 steam_id)
	{
		auto& db = GetDB();

		int count = 0;

		try
		{
			db << "SELECT count(1) FROM Players WHERE SteamId = ?;" << steam_id >> count;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return count != 0;
	}

	bool IsGroupExists(const FString& group)
	{
		auto& db = GetDB();

		int count = 0;

		try
		{
			db << "SELECT count(1) FROM Groups WHERE GroupName = ?;" << group.ToString() >> count;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return count != 0;
	}
}
