#include "KitRepository.h"
#include "DbManager.h"

namespace ArkShop::KitRepository
{
	std::string GetPlayerKits(const uint64 steam_id)
	{
		auto& db = DbManager::GetDb();

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

	bool UpdatePlayerKits(const uint64 steam_id, const std::string kits_data)
	{
		auto& db = DbManager::GetDb();

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

	bool DeleteAllKits()
	{
		auto& db = DbManager::GetDb();

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
}
