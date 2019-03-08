#include "Database/MysqlDB.h"
#include "Database/SqlLiteDB.h"

#include "AtlasShop.h"

#include <fstream>

#include <AtlasPermissions.h>
#include <DBHelper.h>
#include <Kits.h>
#include <Points.h>
#include <Store.h>

#include "StoreSell.h"
#include "TimedRewards.h"

#pragma comment(lib, "AtlasApi.lib")
#pragma comment(lib, "Permissions.lib")

DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*,
AShooterCharacter*, bool);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);
DECLARE_HOOK(URCONServer_Init, bool, URCONServer *, FString, int, UShooterCheatManager *);
DECLARE_HOOK(AShooterPlayerController_GridTravelToLocalPos, void, AShooterPlayerController*, unsigned __int16, unsigned
__int16, FVector *);

FString closed_store_reason;
bool store_enabled = true;

bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* new_player,
                                           UPrimalPlayerData* player_data, AShooterCharacter* player_character,
                                           bool is_from_login)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(new_player);

	if (!AtlasShop::DBHelper::IsPlayerExists(steam_id))
	{
		const bool is_added = AtlasShop::database->TryAddNewPlayer(steam_id);
		if (!is_added)
		{
			return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character,
			                                                 is_from_login);
		}
	}

	const bool rewards_enabled = AtlasShop::config["General"]["TimedPointsReward"]["Enabled"];
	if (rewards_enabled)
	{
		const int interval = AtlasShop::config["General"]["TimedPointsReward"]["Interval"];

		AtlasShop::TimedRewards::Get().AddTask(
			FString::Format("Points_{}", steam_id), steam_id, [steam_id]()
			{
				auto groups_map = AtlasShop::config["General"]["TimedPointsReward"]
					["Groups"];

				int points_amount = groups_map["Default"].value("Amount", 0);

				for (auto group_iter = groups_map.begin(); group_iter != groups_map.
				     end(); ++group_iter)
				{
					const FString group_name(group_iter.key().c_str());
					if (group_name == L"Default")
					{
						continue;
					}

					if (Permissions::IsPlayerInGroup(steam_id, group_name))
					{
						points_amount = group_iter.value().value("Amount", 0);
						break;
					}
				}

				if (points_amount == 0)
				{
					return;
				}

				AtlasShop::Points::AddPoints(points_amount, steam_id);
			},
			interval);
	}
	return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
}

void Hook_AShooterGameMode_Logout(AShooterGameMode* _this, AController* exiting)
{
	// Remove player from the online list

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(exiting);

	AtlasShop::TimedRewards::Get().RemovePlayer(steam_id);

	AShooterGameMode_Logout_original(_this, exiting);
}

FString AtlasShop::GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Messages"].value(str, "No message")));
}

bool AtlasShop::IsStoreEnabled(AShooterPlayerController* player_controller)
{
	if (!store_enabled)
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *closed_store_reason);
		return false;
	}
	return true;
}

void AtlasShop::ToogleStore(bool enabled, const FString& reason)
{
	store_enabled = enabled;
	closed_store_reason = reason;
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/AtlasApi/Plugins/AtlasShop/config.json";
	std::ifstream file{config_path};
	if (!file.is_open())
	{
		throw std::runtime_error("Can't open config.json");
	}

	file >> AtlasShop::config;

	file.close();
}

void ReloadConfig(APlayerController* player_controller, FString* /*unused*/, bool /*unused*/)
{
	auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

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

void ReloadConfigRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
{
	FString reply;

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());

		reply = error.what();
		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
		return;
	}

	reply = "Reloaded config";
	rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
}

void ShowHelp(AShooterPlayerController* player_controller, FString* /*unused*/, EChatSendMode::Type /*unused*/)
{
	const FString help = AtlasShop::GetText("HelpMessage");
	if (help != ArkApi::Tools::Utf8Decode("No message").c_str())
	{
		const float display_time = AtlasShop::config["General"].value("ShopDisplayTime", 15.0f);
		const float text_size = AtlasShop::config["General"].value("ShopTextSize", 1.3f);
		ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::Green, text_size, display_time, nullptr,
		                                       *help);
	}
}

void Load()
{
	Log::Get().Init("AtlasShop");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	const auto& mysql_conf = AtlasShop::config["Mysql"];

	const bool use_mysql = mysql_conf["UseMysql"];
	if (use_mysql)
	{
		AtlasShop::database = std::make_unique<MySql>(mysql_conf.value("MysqlHost", ""),
		                                              mysql_conf.value("MysqlUser", ""),
		                                              mysql_conf.value("MysqlPass", ""),
		                                              mysql_conf.value("MysqlDB", ""));
	}
	else
	{
		const std::string db_path = AtlasShop::config["General"]["DbPathOverride"];
		AtlasShop::database = std::make_unique<SqlLite>(db_path);
	}

	try
	{
		AtlasShop::Points::Init();
		AtlasShop::Store::Init();
		AtlasShop::Kits::Init();
		AtlasShop::StoreSell::Init();

		const FString help = AtlasShop::GetText("HelpCmd");
		if (help != ArkApi::Tools::Utf8Decode("No message").c_str())
		{
			ArkApi::GetCommands().AddChatCommand(help, &ShowHelp);
		}

		ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation",
		                           &Hook_AShooterGameMode_HandleNewPlayer,
		                           &AShooterGameMode_HandleNewPlayer_original);
		ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout,
		                           &AShooterGameMode_Logout_original);

		ArkApi::GetCommands().AddConsoleCommand("AtlasShop.Reload", &ReloadConfig);
		ArkApi::GetCommands().AddRconCommand("AtlasShop.Reload", &ReloadConfigRcon);
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}
}

void Unload()
{
	const FString help = AtlasShop::GetText("HelpCmd");
	if (help != ArkApi::Tools::Utf8Decode("No message").c_str())
	{
		ArkApi::GetCommands().RemoveChatCommand(help);
	}

	ArkApi::GetHooks().DisableHook("AShooterGameMode.HandleNewPlayer_Implementation",
	                               &Hook_AShooterGameMode_HandleNewPlayer);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout);

	ArkApi::GetCommands().RemoveConsoleCommand("AtlasShop.Reload");
	ArkApi::GetCommands().RemoveRconCommand("AtlasShop.Reload");

	AtlasShop::Points::Unload();
	AtlasShop::Store::Unload();
	AtlasShop::Kits::Unload();

	AtlasShop::StoreSell::Unload();
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
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
