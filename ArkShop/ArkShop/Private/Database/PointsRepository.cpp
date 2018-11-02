#include "PointsRepository.h"
#include "DbManager.h"

namespace ArkShop::PointsRepository
{
	int GetPoints(const uint64 steam_id)
	{
		auto& db = DbManager::GetDb();
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

	bool SetPoints(const uint64 steam_id, const int amount)
	{
		auto& db = DbManager::GetDb();

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

	bool DeleteAllPoint()
	{
		auto& db = DbManager::GetDb();

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
