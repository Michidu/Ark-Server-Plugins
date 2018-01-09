#include <fstream>
#include <chrono>
#include "ArkShop.h"
#include "Tools.h"
#include "DBHelper.h"
#include "TimedRewards.h"
#include "Points.h"
#include "Kits.h"
#include "Store.h"
#include "Market.h"

#pragma comment(lib, "ArkApi.lib")

nlohmann::json json;

namespace
{
	DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*, AShooterCharacter*, bool);
	DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);

	void LoadConfig();
	void AddPlayerToRewards(__int64 steamId);
	void RemovePlayerRewards(__int64 steamId);
	void ReloadShopConfig(APlayerController* playerController, FString* cmd, bool shouldLog);

	void Init()
	{
		auto db = GetDB();

		//setup and migrate database
		int hasTablePlayers = false;
		db << "PRAGMA table_info(Players);" >> [&]() { hasTablePlayers = true; };
		if (!hasTablePlayers)
		{
			try
			{
				db << "begin;";
				db << "create table if not exists Players ("
					"Id integer primary key autoincrement not null,"
					"SteamId integer default 0,"
					"Kits text default '{}',"
					"Points integer default 0,"
					"TimedRewardAmountOverride integer default null"
					");";
				db << "PRAGMA user_version = 1;";
				db << "commit;";
			}
			catch (sqlite::sqlite_exception &e)
			{
				db << "rollback;";
				Tools::Log("Failed to setup database");
				throw;
			}
		}
		else
		{
			int userVersion = 0;
			db << "PRAGMA user_version;" >> userVersion;

			if (userVersion == 0)
			{
				try
				{
					userVersion = 1;
					db << "begin;";
					db << u"alter table Players add column TimedRewardAmountOverride integer default null;";
					db << "PRAGMA user_version = " + std::to_string(userVersion) + ";";
					db << "commit;";
				}
				catch (sqlite::sqlite_exception &e)
				{
					db << "rollback;";
					Tools::Log("Failed to migrate database to version 1");
					throw;
				}
			}
		}

		LoadConfig();

		Points::Init();
		Store::Init();
		Kits::Init();

		bool rewardsEnabled = json["General"]["TimedPointsReward"]["Enabled"];
		if (rewardsEnabled)
		{
			TimedRewards::Init();
		}

		Ark::SetHook("AShooterGameMode", "HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer, reinterpret_cast<LPVOID*>(&AShooterGameMode_HandleNewPlayer_original));
		Ark::SetHook("AShooterGameMode", "Logout", &Hook_AShooterGameMode_Logout, reinterpret_cast<LPVOID*>(&AShooterGameMode_Logout_original));

		Ark::AddConsoleCommand(L"ReloadShopConfig", &ReloadShopConfig);
	}

	void LoadConfig()
	{
		std::ifstream file(Tools::GetCurrentDir() + "/BeyondApi/Plugins/ArkShop/config.json");
		if (!file.is_open())
		{
			std::cout << "Could not open file config.json" << std::endl;
			throw;
		}

		file >> json;
		file.close();
	}

	bool _cdecl Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* NewPlayer, UPrimalPlayerData* PlayerData, AShooterCharacter* PlayerCharacter, bool bIsFromLogin)
	{
		__int64 steamId = Tools::GetSteamId(NewPlayer);

		if (!DBHelper::IsPlayerEntryExists(steamId))
		{
			auto db = GetDB();

			try
			{
				db << "INSERT INTO Players (SteamId) VALUES (?);"
					<< steamId;
			}
			catch (sqlite::sqlite_exception& e)
			{
				std::cout << "HandleNewPlayer() Unexpected DB error " << e.what() << std::endl;
			}
		}

		// Add new player to the online list

		bool rewardsEnabled = json["General"]["TimedPointsReward"]["Enabled"];
		if (rewardsEnabled)
		{
			AddPlayerToRewards(steamId);
		}

		return AShooterGameMode_HandleNewPlayer_original(_this, NewPlayer, PlayerData, PlayerCharacter, bIsFromLogin);
	}

	void AddPlayerToRewards(__int64 steamId)
	{
		std::vector<TimedRewards::OnlinePlayersData*>::iterator res = std::find_if(TimedRewards::OnlinePlayers.begin(), TimedRewards::OnlinePlayers.end(),
		                                                                           [steamId](TimedRewards::OnlinePlayersData* data) -> bool { return data->SteamId == steamId; });
		if (res != TimedRewards::OnlinePlayers.end())
			return;

		int interval = TimedRewards::GetInterval();

		auto now = std::chrono::system_clock::now();
		auto nextTime = now + std::chrono::minutes(interval);

		auto data = new TimedRewards::OnlinePlayersData(steamId, nextTime);
		TimedRewards::OnlinePlayers.push_back(data);
	}

	void _cdecl Hook_AShooterGameMode_Logout(AShooterGameMode* _this, AController* Exiting)
	{
		// Remove player from the online list

		AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(Exiting);

		__int64 steamId = Tools::GetSteamId(aShooterPC);

		bool rewardsEnabled = json["General"]["TimedPointsReward"]["Enabled"];
		if (rewardsEnabled)
		{
			RemovePlayerRewards(steamId);
		}

		AShooterGameMode_Logout_original(_this, Exiting);
	}

	void RemovePlayerRewards(__int64 steamId)
	{
		for (auto data : TimedRewards::OnlinePlayers)
		{
			if (data->SteamId == steamId)
			{
				auto& v = TimedRewards::OnlinePlayers;
				v.erase(remove(v.begin(), v.end(), data), v.end());

				delete data;
				break;
			}
		}
	}

	void ReloadShopConfig(APlayerController* playerController, FString* cmd, bool shouldLog)
	{
		LoadConfig();

		AShooterPlayerController* aShooterController = static_cast<AShooterPlayerController*>(playerController);

		Tools::SendDirectMessage(aShooterController, TEXT("Reloaded config"));
	}
}

sqlite::database GetDB()
{
	static sqlite::database db(Tools::GetCurrentDir() + "/ArkDb.db");
	return db;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Init();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
