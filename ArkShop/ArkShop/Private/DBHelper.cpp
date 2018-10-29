#include <DBHelper.h>

#include "ArkShop.h"
#include "Database/DbManager.h"


namespace ArkShop::DBHelper
{
	bool IsPlayerExists(const uint64 steam_id)
	{
		auto& db = DbManager::GetDb();

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
} // namespace DBHelper // namespace ArkShop
