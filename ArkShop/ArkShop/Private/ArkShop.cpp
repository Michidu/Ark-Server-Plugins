#include "ArkShop.h"

#include <fstream>

#include <Points.h>
#include <Kits.h>
#include <Store.h>
#include <DBHelper.h>

#include "TimedRewards.h"
#include "StoreSell.h"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "Permissions.lib")

nlohmann::json ArkShop::config;

DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*,
	AShooterCharacter*, bool);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);

bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* new_player,
                                           UPrimalPlayerData* player_data, AShooterCharacter* player_character,
                                           bool is_from_login)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(new_player);

	if (!ArkShop::DBHelper::IsPlayerExists(steam_id))
	{
		auto& db = ArkShop::GetDB();

		try
		{
			db << "INSERT INTO Players (SteamId) VALUES (?);"
				<< steam_id;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
		}
	}

	// Add new player to the online list

	const bool rewards_enabled = ArkShop::config["General"]["TimedPointsReward"]["Enabled"];
	if (rewards_enabled)
	{
		ArkShop::TimedRewards::Get().AddPlayer(steam_id);
	}

	return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
}


void Hook_AShooterGameMode_Logout(AShooterGameMode* _this, AController* exiting)
{
	// Remove player from the online list

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(exiting);

	const bool rewards_enabled = ArkShop::config["General"]["TimedPointsReward"]["Enabled"];
	if (rewards_enabled)
	{
		ArkShop::TimedRewards::Get().RemovePlayer(steam_id);
	}

	AShooterGameMode_Logout_original(_this, exiting);
}

sqlite::database& ArkShop::GetDB()
{
	static sqlite::database db(config["General"].value("DbPathOverride", "").empty()
		                           ? ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/ArkShop/ArkShop.db"
		                           : config["General"]["DbPathOverride"]);
	return db;
}

FString ArkShop::GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Messages"].value(str, "No message")).c_str());
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/ArkShop/config.json";
	std::ifstream file{config_path};
	if (!file.is_open())
		throw std::runtime_error("Can't open config.json");

	file >> ArkShop::config;

	file.close();
}

void ReloadConfig(APlayerController* player_controller, FString*, bool)
{
	AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Failed to reload config");

		Log::GetLog()->error(error.what());
		return;
	}

	ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Reloaded config");
}

void Load()
{
	Log::Get().Init("ArkShop");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	auto& db = ArkShop::GetDB();

	db << "create table if not exists Players ("
		"Id integer primary key autoincrement not null,"
		"SteamId integer default 0,"
		"Kits text default '{}',"
		"Points integer default 0"
		");";

	ArkShop::Points::Init();
	ArkShop::Store::Init();
	ArkShop::Kits::Init();
	ArkShop::StoreSell::Init();

	ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer,
	                           &AShooterGameMode_HandleNewPlayer_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout,
	                           &AShooterGameMode_Logout_original);

	ArkApi::GetCommands().AddConsoleCommand("ArkShop.Reload", &ReloadConfig);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
