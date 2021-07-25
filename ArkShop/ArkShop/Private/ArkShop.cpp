#include "Database/MysqlDB.h"
#include "Database/SqlLiteDB.h"

#include "ArkShop.h"

#include <fstream>

#include <ArkPermissions.h>
#include <DBHelper.h>
#include <Kits.h>
#include <Points.h>
#include <Store.h>

#include "StoreSell.h"
#include "TimedRewards.h"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "Permissions.lib")

DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*,
AShooterCharacter*, bool);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);
DECLARE_HOOK(URCONServer_Init, bool, URCONServer*, FString, int, UShooterCheatManager*);
DECLARE_HOOK(AShooterPlayerController_GridTravelToLocalPos, void, AShooterPlayerController*, unsigned __int16, unsigned
__int16, FVector*);

FString closed_store_reason;
bool store_enabled = true;

//Builds custom data for cryopod
FCustomItemData ArkShop::GetDinoCustomItemData(APrimalDinoCharacter* dino, UPrimalItem* saddle)
{
	FCustomItemData customItemData;

	FARKDinoData dinoData;
	dino->GetDinoData(&dinoData);

	customItemData.CustomDataName = FName("Dino", EFindName::FNAME_Add);
	customItemData.CustomDataNames.Add(FName("MissionTemporary", EFindName::FNAME_Add)); //What it should be...
	customItemData.CustomDataNames.Add(FName("None", EFindName::FNAME_Find)); //What WC has it currently set to...

	customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->CurrentStatusValuesField()()[EPrimalCharacterStatusValue::Health]);
	customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->CurrentStatusValuesField()()[EPrimalCharacterStatusValue::Stamina]);
	customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->CurrentStatusValuesField()()[EPrimalCharacterStatusValue::Torpidity]);
	customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->MaxStatusValuesField()()[EPrimalCharacterStatusValue::Health]);
	customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->MaxStatusValuesField()()[EPrimalCharacterStatusValue::Stamina]);
	customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->MaxStatusValuesField()()[EPrimalCharacterStatusValue::Torpidity]);
	customItemData.CustomDataFloats.Add(dino->bIsFemale()());

	customItemData.CustomDataStrings.Add(dinoData.DinoNameInMap);
	customItemData.CustomDataStrings.Add(dinoData.DinoName);
	customItemData.CustomDataClasses.Add(dinoData.DinoClass);

	FCustomItemByteArray dinoBytes, saddlebytes;
	dinoBytes.Bytes = dinoData.DinoData;
	customItemData.CustomDataBytes.ByteArrays.Add(dinoBytes);
	if (saddle)
	{
		saddle->GetItemBytes(&saddlebytes.Bytes);
		customItemData.CustomDataBytes.ByteArrays.Add(saddlebytes);
	}

	return customItemData;
}

//Spawns dino or gives in cryopod
bool ArkShop::GiveDino(AShooterPlayerController* player_controller, int level, bool neutered, std::string blueprint, std::string saddleblueprint)
{
	bool success = false;
	const FString fblueprint(blueprint.c_str());
	APrimalDinoCharacter* dino = ArkApi::GetApiUtils().SpawnDino(player_controller, fblueprint, nullptr, level, true, neutered);
	if (dino && ArkShop::config["General"].value("GiveDinosInCryopods", false))
	{
		FString cryo = FString(ArkShop::config["General"].value("CryoItemPath", "Blueprint'/Game/Extinction/CoreBlueprints/Weapons/PrimalItem_WeaponEmptyCryopod.PrimalItem_WeaponEmptyCryopod'"));
		UClass* Class = UVictoryCore::BPLoadClass(&cryo);
		UPrimalItem* item = UPrimalItem::AddNewItem(Class, nullptr, false, false, 0, false, 0, false, 0, false, nullptr, 0);
		if (item)
		{
			if (ArkShop::config["General"].value("CryoLimitedTime", false))
				item->AddItemDurability((item->ItemDurabilityField() - 3600) * -1);

			UPrimalItem* saddle = nullptr;
			if (saddleblueprint.size() > 0)
			{
				FString fblueprint(saddleblueprint.c_str());
				UClass* Class = UVictoryCore::BPLoadClass(&fblueprint);
				saddle = UPrimalItem::AddNewItem(Class, nullptr, false, false, 0, false, 0, false, 0, false, nullptr, 0);
			}

			FCustomItemData customItemData = GetDinoCustomItemData(dino, saddle);
			item->SetCustomItemData(&customItemData);
			item->UpdatedItem(true);

			if (player_controller->GetPlayerInventoryComponent())
			{
				UPrimalItem* item2 = player_controller->GetPlayerInventoryComponent()->AddItemObject(item);

				if (item2)
					success = true;
			}
		}

		dino->Destroy(true, false);
	}
	else if (dino)
		success = true;

	return success;
}

bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* new_player,
	UPrimalPlayerData* player_data, AShooterCharacter* player_character,
	bool is_from_login)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(new_player);

	if (!ArkShop::DBHelper::IsPlayerExists(steam_id))
	{
		const bool is_added = ArkShop::database->TryAddNewPlayer(steam_id);
		if (!is_added)
		{
			return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character,
				is_from_login);
		}
	}

	const bool rewards_enabled = ArkShop::config["General"]["TimedPointsReward"]["Enabled"];
	if (rewards_enabled)
	{
		const int interval = ArkShop::config["General"]["TimedPointsReward"]["Interval"];

		ArkShop::TimedRewards::Get().AddTask(
			FString::Format("Points_{}", steam_id), steam_id, [steam_id]()
			{
				auto groups_map = ArkShop::config["General"]["TimedPointsReward"]["Groups"];

				const bool stack_rewards = ArkShop::config["General"]["TimedPointsReward"].value("StackRewards", false);

				int high_points_amount = 0;
				for (auto group_iter = groups_map.begin(); group_iter != groups_map.end(); ++group_iter)
				{
					const FString group_name(group_iter.key().c_str());
					if (Permissions::IsPlayerInGroup(steam_id, group_name))
					{
						int points_amount = group_iter.value().value("Amount", 0);

						if (!stack_rewards)
						{
							if (points_amount > high_points_amount)
							{
								high_points_amount = points_amount;
							}
						}
						else
						{
							high_points_amount += points_amount;
						}
					}
				}

				if (high_points_amount <= 0)
				{
					return;
				}

				ArkShop::Points::AddPoints(high_points_amount, steam_id);
			},
			interval);
	}
	return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
}

void Hook_AShooterGameMode_Logout(AShooterGameMode* _this, AController* exiting)
{
	// Remove player from the online list

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(exiting);

	ArkShop::TimedRewards::Get().RemovePlayer(steam_id);

	AShooterGameMode_Logout_original(_this, exiting);
}

FString ArkShop::GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Messages"].value(str, "No message")));
}

bool ArkShop::IsStoreEnabled(AShooterPlayerController* player_controller)
{
	if (!store_enabled)
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *closed_store_reason);
		return false;
	}
	return true;
}

void ArkShop::ToogleStore(bool enabled, const FString& reason)
{
	store_enabled = enabled;
	closed_store_reason = reason;
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/ArkShop/config.json";
	std::ifstream file{ config_path };
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

	try
	{
		const auto& mysql_conf = ArkShop::config["Mysql"];

		const bool use_mysql = mysql_conf["UseMysql"];
		if (use_mysql)
		{
			ArkShop::database = std::make_unique<MySql>(
				mysql_conf.value("MysqlHost", ""),
				mysql_conf.value("MysqlUser", ""),
				mysql_conf.value("MysqlPass", ""),
				mysql_conf.value("MysqlDB", ""),
				mysql_conf.value("MysqlPlayersTable", "ArkShopPlayers"),
				mysql_conf.value("MysqlPort", 3306));
		}
		else
		{
			const std::string db_path = ArkShop::config["General"]["DbPathOverride"];
			ArkShop::database = std::make_unique<SqlLite>(db_path);
		}

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
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}
}

void Unload()
{
	const FString help = ArkShop::GetText("HelpCmd");
	if (help != ArkApi::Tools::Utf8Decode("No message").c_str())
	{
		ArkApi::GetCommands().RemoveChatCommand(help);
	}

	ArkApi::GetCommands().RemoveOnTimerCallback("RewardTimer");
	ArkApi::GetHooks().DisableHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout);

	ArkApi::GetCommands().RemoveConsoleCommand("ArkShop.Reload");
	ArkApi::GetCommands().RemoveRconCommand("ArkShop.Reload");

	ArkShop::Points::Unload();
	ArkShop::Store::Unload();
	ArkShop::Kits::Unload();

	ArkShop::StoreSell::Unload();
}

extern "C" __declspec(dllexport) void Plugin_Unload()
{
	// Stop threads here
	ArkApi::GetCommands().RemoveOnTimerCallback("RewardTimer");
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