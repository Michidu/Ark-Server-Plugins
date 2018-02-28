#include "ShopRewards.h"

#include <fstream>

#include "DinoRewards.h"
#include "PlayerRewards.h"
#include "Stats.h"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "ArkShop.lib")

DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);

nlohmann::json config;

FString GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Messages"].value(str, "No message")).c_str());
}

sqlite::database& GetDB()
{
	static sqlite::database db(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/ShopRewards/ShopRewards.db");
	return db;
}

void Hook_AShooterGameMode_Logout(AShooterGameMode* _this, AController* exiting)
{
	if (exiting)
	{
		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(exiting);
		if (Stats::IsPlayerExists(steam_id))
		{
			std::string str_name;

			auto& db = GetDB();

			try
			{
				db << "SELECT Name FROM Players WHERE SteamId = ?;" << steam_id >> str_name;
			}
			catch (const sqlite::sqlite_exception& exception)
			{
				Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());

				AShooterGameMode_Logout_original(_this, exiting);
				return;
			}

			const FString name = ArkApi::Tools::Utf8Decode(str_name).c_str();

			AShooterPlayerController* player = static_cast<AShooterPlayerController*>(exiting);
			if (player->GetPlayerCharacter())
			{
				const FString new_name = player->GetPlayerCharacter()->PlayerNameField()();
				if (new_name != name)
				{
					try
					{
						db << "UPDATE Players SET Name = ? WHERE SteamId = ?;" << ArkApi::Tools::Utf8Encode(*new_name) << steam_id;
					}
					catch (const sqlite::sqlite_exception& exception)
					{
						Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
					}
				}
			}
		}
	}

	AShooterGameMode_Logout_original(_this, exiting);
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/ShopRewards/config.json";
	std::ifstream file{config_path};
	if (!file.is_open())
		throw std::runtime_error("Can't open config.json");

	file >> config;

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
	Log::Get().Init("ShopRewards");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	auto& db = GetDB();

	db << "create table if not exists Players ("
		"Id integer primary key autoincrement not null,"
		"SteamId integer default 0,"
		"Name text default '',"
		"PlayerKills integer default 0,"
		"PlayerDeaths integer default 0,"
		"WildDinoKills integer default 0,"
		"TamedDinoKills integer default 0"
		");";

	DinoRewards::Init();
	PlayerRewards::Init();
	Stats::Init();

	ArkApi::GetCommands().AddConsoleCommand("ShopRewards.Reload", &ReloadConfig);

	ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout,
	                           &AShooterGameMode_Logout_original);
}

void Unload()
{
	DinoRewards::Unload();
	PlayerRewards::Unload();

	ArkApi::GetCommands().RemoveConsoleCommand("ShopRewards.Reload");

	ArkApi::GetHooks().DisableHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		Unload();
		break;
	}
	return TRUE;
}
