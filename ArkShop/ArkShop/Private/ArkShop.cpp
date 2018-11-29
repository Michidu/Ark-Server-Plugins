#include "ArkShop.h"

#include <fstream>

#include <DBHelper.h>
#include <Kits.h>
#include <Points.h>
#include <Store.h>

#include "StoreSell.h"

#include "TimedRewards.h"
#include "Database/DbManager.h"


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
		const bool is_added = ArkShop::DbManager::TryAddNewPlayer(steam_id);
		if (!is_added)
		{
			return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character,
				is_from_login);
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
FString ClosedStoreReason;
bool StoreEnabled = true;
bool ArkShop::IsStoreEnabled(AShooterPlayerController* player_controller)
{
	if (!StoreEnabled)
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *ClosedStoreReason);
		return false;
	}
	return true;
}

void ArkShop::ToogleStore(bool Enabled, const FString& Reason)
{
	StoreEnabled = !StoreEnabled;
	ClosedStoreReason = Reason;
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
	{
		throw std::runtime_error("Can't open config.json");
	}

	file >> ArkShop::config;

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
	const FString help = ArkShop::GetText("HelpMessage");
	if (help != ArkApi::Tools::Utf8Decode("No message").c_str())
	{
		const float display_time = ArkShop::config["General"].value("ShopDisplayTime", 15.0f);
		const float text_size = ArkShop::config["General"].value("ShopTextSize", 1.3f);
		ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::Green, text_size, display_time, nullptr,
		                                       *help);
	}
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

	ArkShop::DbManager::CreateDatabase();

	ArkShop::Points::Init();
	ArkShop::Store::Init();
	ArkShop::Kits::Init();
	ArkShop::StoreSell::Init();

	const FString help = ArkShop::GetText("HelpCmd");
	if (help != ArkApi::Tools::Utf8Decode("No message").c_str())
	{
		ArkApi::GetCommands().AddChatCommand(help, &ShowHelp);
	}

	ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation",
	                           &Hook_AShooterGameMode_HandleNewPlayer,
	                           &AShooterGameMode_HandleNewPlayer_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout,
	                           &AShooterGameMode_Logout_original);

	ArkApi::GetCommands().AddConsoleCommand("ArkShop.Reload", &ReloadConfig);
	ArkApi::GetCommands().AddRconCommand("ArkShop.Reload", &ReloadConfigRcon);
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
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
