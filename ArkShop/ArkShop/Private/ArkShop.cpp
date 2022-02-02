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
#include <ArkShopUIHelper.h>

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "Permissions.lib")

DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*, AShooterCharacter*, bool);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);
DECLARE_HOOK(URCONServer_Init, bool, URCONServer*, FString, int, UShooterCheatManager*);
DECLARE_HOOK(AShooterPlayerController_GridTravelToLocalPos, void, AShooterPlayerController*, unsigned __int16, unsigned __int16, FVector*);

FString closed_store_reason;
bool store_enabled = true;

FString ArkShop::SetMapName()
{
	if (!ArkShop::MapName.IsEmpty())
		return ArkShop::MapName;

	LPWSTR* argv;
	int argc;
	int i;
	FString param(L"-serverkey=");
	FString LocalMapName;

	ArkApi::GetApiUtils().GetShooterGameMode()->GetMapName(&LocalMapName);

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (NULL != argv)
	{
		for (i = 0; i < argc; i++)
		{
			FString arg(argv[i]);
			if (arg.Contains(param))
			{
				if (arg.RemoveFromStart(param))
				{
					LocalMapName = arg;
					break;
				}
			}
		}

		LocalFree(argv);
	}

	Log::GetLog()->info("MapName: {}", LocalMapName.ToString());
	ArkShop::MapName = LocalMapName;

	return ArkShop::MapName;
}

void ArkShop::PostToDiscord(const std::wstring log)
{
	if (!ArkShop::discord_enabled || ArkShop::discord_webhook_url.IsEmpty())
		return;

	FString msg = L"{{\"content\":\"```stylus\\n{}```\",\"username\":\"{}\",\"avatar_url\":null}}";
	FString output = FString::Format(*msg, log, ArkShop::discord_sender_name);
	static_cast<AShooterGameState*>(ArkApi::GetApiUtils().GetWorld()->GameStateField())->HTTPPostRequest(ArkShop::discord_webhook_url, output);
}

float ArkShop::getStatValue(float StatModifier, float InitialValueConstant, float RandomizerRangeMultiplier, float StateModifierScale, bool bDisplayAsPercent)
{
	float ItemStatValue;

	if (bDisplayAsPercent)
		InitialValueConstant += 100;

	if (InitialValueConstant > StatModifier)
		StatModifier = InitialValueConstant;

	ItemStatValue = (StatModifier - InitialValueConstant) / (InitialValueConstant * RandomizerRangeMultiplier * StateModifierScale);

	return ItemStatValue;
}

void ArkShop::ApplyItemStats(TArray<UPrimalItem*> items, int armor, int durability, int damage)
{
	if (armor > 0 || durability > 0 || damage > 0)
	{
		for (UPrimalItem* item : items)
		{
			bool updated = false;

			static int statInfoStructSize = GetStructSize<FItemStatInfo>();

			if (armor > 0)
			{
				FItemStatInfo* itemstat = static_cast<FItemStatInfo*>(FMemory::Malloc(0x0024));
				RtlSecureZeroMemory(itemstat, 0x0024);
				item->GetItemStatInfo(itemstat, EPrimalItemStat::Armor);

				if (itemstat->bUsed()())
				{
					float newStat = 0.f;
					bool percent = itemstat->bDisplayAsPercent()();

					newStat = getStatValue(armor, itemstat->InitialValueConstantField(), itemstat->RandomizerRangeMultiplierField(), itemstat->StateModifierScaleField(), percent);

					if (newStat >= 65536.f)
						newStat = 65535;

					item->ItemStatValuesField()()[EPrimalItemStat::Armor] = newStat;
					updated = true;
				}

				FMemory::Free(itemstat);
			}

			if (durability > 0)
			{
				FItemStatInfo* itemstat = static_cast<FItemStatInfo*>(FMemory::Malloc(0x0024));
				RtlSecureZeroMemory(itemstat, 0x0024);
				item->GetItemStatInfo(itemstat, EPrimalItemStat::MaxDurability);

				if (itemstat->bUsed()())
				{
					float newStat = 0.f;
					bool percent = itemstat->bDisplayAsPercent()();

					newStat = getStatValue(durability, itemstat->InitialValueConstantField(), itemstat->RandomizerRangeMultiplierField(), itemstat->StateModifierScaleField(), percent) + 1;

					if (newStat >= 65536.f)
						newStat = 65535;

					item->ItemStatValuesField()()[EPrimalItemStat::MaxDurability] = newStat;
					item->ItemDurabilityField() = item->GetItemStatModifier(EPrimalItemStat::MaxDurability);
					updated = true;
				}

				FMemory::Free(itemstat);
			}

			if (damage > 0)
			{
				FItemStatInfo* itemstat = static_cast<FItemStatInfo*>(FMemory::Malloc(0x0024));
				RtlSecureZeroMemory(itemstat, 0x0024);
				item->GetItemStatInfo(itemstat, EPrimalItemStat::WeaponDamagePercent);

				if (itemstat->bUsed()())
				{
					float newStat = 0.f;
					bool percent = itemstat->bDisplayAsPercent()();

					newStat = getStatValue(damage, itemstat->InitialValueConstantField(), itemstat->RandomizerRangeMultiplierField(), itemstat->StateModifierScaleField(), percent);

					if (newStat >= 65536.f)
						newStat = 65535;

					item->SetItemStatValues(EPrimalItemStat::WeaponDamagePercent, newStat);
					updated = true;
				}

				FMemory::Free(itemstat);
			}

			if (updated)
				item->UpdatedItem(false);
		}
	}
}

FString ArkShop::GetBlueprintShort(UObjectBase* object)
{
	if (object != nullptr && object->ClassField() != nullptr)
	{
		FString path_name;
		object->ClassField()->GetDefaultObject(true)->GetFullName(&path_name, nullptr);

		if (int find_index = 0; path_name.FindChar(' ', find_index))
		{
			path_name = path_name.Mid(find_index + 1,
				path_name.Len() - (find_index + (path_name.EndsWith(
					"_C", ESearchCase::
					CaseSensitive)
					? 3
					: 1)));
			path_name = path_name.Replace(L"Default__", L"", ESearchCase::CaseSensitive);
			return path_name;
		}
	}

	return FString("");
}

//Builds custom data for cryopod
FCustomItemData ArkShop::GetDinoCustomItemData(APrimalDinoCharacter* dino, UPrimalItem* saddle, bool Modded)
{
	FCustomItemData customItemData;

	if (!Modded)
	{
		FARKDinoData dinoData;
		dino->GetDinoData(&dinoData);

		customItemData.CustomDataName = FName("Dino", EFindName::FNAME_Add);
		customItemData.CustomDataNames.Add(FName("MissionTemporary", EFindName::FNAME_Add));
		customItemData.CustomDataNames.Add(FName("None", EFindName::FNAME_Find));

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

		FCustomItemByteArray dinoBytes;
		dinoBytes.Bytes = dinoData.DinoData;
		customItemData.CustomDataBytes.ByteArrays.Add(dinoBytes);

		if (saddle)
		{
			FCustomItemByteArray saddlebytes;
			saddle->GetItemBytes(&saddlebytes.Bytes);
			customItemData.CustomDataBytes.ByteArrays.Add(saddlebytes);
		}
	}
	else
	{
		FARKDinoData dinoData;
		dino->GetDinoData(&dinoData);

		customItemData.CustomDataName = FName("Dino", EFindName::FNAME_Add);

		customItemData.CustomDataFloats.Add(1800.0f);
		customItemData.CustomDataFloats.Add(1.0f);

		customItemData.CustomDataStrings.Add(dinoData.DinoNameInMap); //0
		customItemData.CustomDataStrings.Add(dinoData.DinoName); //1
		customItemData.CustomDataStrings.Add(L"0"); //2
		customItemData.CustomDataStrings.Add(FString(std::to_string(dino->bNeutered()()))); //3
		customItemData.CustomDataStrings.Add(FString(std::to_string(dino->bIsFemale()()))); //4
		customItemData.CustomDataStrings.Add(L""); //5
		customItemData.CustomDataStrings.Add(L""); //6
		customItemData.CustomDataStrings.Add(L"SDOTU"); //7
		customItemData.CustomDataStrings.Add(FString(std::to_string(dino->bUsesGender()()))); //8
		customItemData.CustomDataStrings.Add(L"0"); //9
		customItemData.CustomDataStrings.Add(GetBlueprintShort(dino)); //10
		customItemData.CustomDataStrings.Add(L""); //11
		customItemData.CustomDataStrings.Add(FString(std::to_string(dino->bPreventMating()()))); //12
		customItemData.CustomDataStrings.Add(FString(std::to_string(dino->bUseBabyGestation()()))); //13
		customItemData.CustomDataStrings.Add(FString(std::to_string(dino->bDebugBaby()()))); //14
		customItemData.CustomDataStrings.Add("0"); //15
		customItemData.CustomDataStrings.Add(dino->DescriptiveNameField()); //16

		customItemData.CustomDataDoubles.Doubles.Add(ArkApi::GetApiUtils().GetWorld()->TimeSecondsField() * -1);
		customItemData.CustomDataDoubles.Doubles.Add(0);

		FCustomItemByteArray dinoBytes;
		dinoBytes.Bytes = dinoData.DinoData;
		customItemData.CustomDataBytes.ByteArrays.Add(dinoBytes); //0

		if (saddle)
		{
			FCustomItemByteArray saddlebytes, emptyBytes;
			saddle->GetItemBytes(&saddlebytes.Bytes);
			customItemData.CustomDataBytes.ByteArrays.Add(emptyBytes); //1
			customItemData.CustomDataBytes.ByteArrays.Add(saddlebytes); //2
		}
	}

	return customItemData;
}

//Spawns dino or gives in cryopod
bool ArkShop::GiveDino(AShooterPlayerController* player_controller, int level, bool neutered, std::string gender, std::string blueprint, std::string saddleblueprint)
{
	bool success = false;
	const FString fblueprint(blueprint.c_str());
	APrimalDinoCharacter* dino = ArkApi::GetApiUtils().SpawnDino(player_controller, fblueprint, nullptr, level, true, neutered);
	if (dino)
	{
		if (dino->bUsesGender()())
		{
			if (strcmp(gender.c_str(), "male") == 0)
				dino->bIsFemale() = false;
			else if (strcmp(gender.c_str(), "female") == 0)
				dino->bIsFemale() = true;
		}

		if (ArkShop::config["General"].value("GiveDinosInCryopods", false))
		{
			bool Modded = config["General"].value("UseSoulTraps", false);

			FString cryo = FString(ArkShop::config["General"].value("CryoItemPath", "Blueprint'/Game/Extinction/CoreBlueprints/Weapons/PrimalItem_WeaponEmptyCryopod.PrimalItem_WeaponEmptyCryopod'"));
			if (Modded)
				cryo = FString("Blueprint'/Game/Mods/DinoStorage2/SoulTrap_DS.SoulTrap_DS'");
			if (cryo.IsEmpty())
				cryo = FString("Blueprint'/Game/Extinction/CoreBlueprints/Weapons/PrimalItem_WeaponEmptyCryopod.PrimalItem_WeaponEmptyCryopod'");

			UClass* Class = UVictoryCore::BPLoadClass(&cryo);
			UPrimalItem* item = UPrimalItem::AddNewItem(Class, nullptr, false, false, 0, false, 0, false, 0, false, nullptr, 0, false, false);
			if (item)
			{
				if (ArkShop::config["General"].value("CryoLimitedTime", false) && !Modded)
					item->AddItemDurability((item->ItemDurabilityField() - 3600) * -1);

				if (Modded)
					item->ItemDurabilityField() = 0.001;

			UPrimalItem* saddle = nullptr;
			if (saddleblueprint.size() > 0)
			{
				FString fblueprint(saddleblueprint.c_str());
				UClass* Class = UVictoryCore::BPLoadClass(&fblueprint);
				saddle = UPrimalItem::AddNewItem(Class, nullptr, false, false, 0, false, 0, false, 0, false, nullptr, 0, false, false);
			}

				FCustomItemData customItemData = GetDinoCustomItemData(dino, saddle, Modded);
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
		else
			success = true;
	}

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

UClass* ArkShop::GetRemappedClass(FString& objectBp, RemapType remapType)
{
	UClass* remap = nullptr;
	TArray<FClassRemapping> remaps{};
	UPrimalGameData* PGD = UPrimalGameData::BPGetGameData();

	switch (remapType)
	{
	case Engram:
		remaps = PGD->Remap_EngramsField();
		break;
	case Item:
		remaps = PGD->Remap_ItemsField();
		break;
	case NPC:
		remaps = PGD->Remap_EngramsField();
		break;
	}

	if (remaps.Num() > 0)
	{
		TSubclassOf<UObject> remappedClass;
		PGD->GetRemappedClass(&remappedClass, &remaps, UVictoryCore::BPLoadClass(&objectBp));
		return remappedClass.uClass;
	}
	else
	{
		return UVictoryCore::BPLoadClass(&objectBp);
	}
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

		if (ArkApi::Tools::IsPluginLoaded("ArkShopUI"))
			ArkShopUI::Reload();
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

		if (ArkApi::Tools::IsPluginLoaded("ArkShopUI"))
			ArkShopUI::Reload();
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

		//Discord Functions
		const auto& discord_config = ArkShop::config["General"].value("Discord", nlohmann::json::object());

		ArkShop::discord_enabled = discord_config.value("Enabled", false);
		ArkShop::discord_sender_name = discord_config.value("SenderName", "");
		ArkShop::discord_webhook_url = discord_config.value("URL", "").c_str();

		const FString help = ArkShop::GetText("HelpCmd");
		if (help != ArkApi::Tools::Utf8Decode("No message").c_str())
		{
			ArkApi::GetCommands().AddChatCommand(help, &ShowHelp);
		}

		ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer, &AShooterGameMode_HandleNewPlayer_original);
		ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout, &AShooterGameMode_Logout_original);

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