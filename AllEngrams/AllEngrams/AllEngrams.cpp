#include <fstream>

#include <API/ARK/Ark.h>
#include <API/UE/Math/ColorList.h>
#include <ArkPermissions.h>

#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "Permissions.lib")

DECLARE_HOOK(AShooterGameMode_InitGame, void, AShooterGameMode*, FString*, FString*, FString*);
DECLARE_HOOK(UPrimalCharacterStatusComponent_ServerApplyLevelUp, void, UPrimalCharacterStatusComponent*,
	EPrimalCharacterStatusValue::Type, AShooterPlayerController*);

nlohmann::json config;

struct EngramEntry
{
	FString name;
	TSubclassOf<UPrimalItem> engram;
	int level{};
	int cost{};
};

TArray<EngramEntry> original_engrams;
TArray<EngramEntry> unlock_engrams;

TArray<FString> include_engrams;
TArray<FString> exclude_engrams;

FString GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Messages"].value(str, "No message")).c_str());
}

void ReadEngrams()
{
	const bool auto_detect = config.value("AutoDetectEngrams", true);

	// Make a copy of original engrams list
	if (original_engrams.Num() == 0)
	{
		TArray<UPrimalEngramEntry*> all_engrams_entries = static_cast<UPrimalGlobals*>(Globals::GEngine()()->
				GameSingletonField())->PrimalGameDataOverrideField()->
				                       EngramBlueprintEntriesField();

		for (UPrimalEngramEntry* engram_entry : all_engrams_entries)
		{
			FString name;
			engram_entry->NameField().ToString(&name);

			name.RemoveFromEnd("_1");

			original_engrams.Add({name, engram_entry->BluePrintEntryField(), engram_entry->GetRequiredLevel(), engram_entry->RequiredEngramPointsField()});
		}
	}

	unlock_engrams.Empty();

	if (!auto_detect && include_engrams.Num() > 0)
	{
		for (const auto& engram_entry : original_engrams)
		{
			const FString name = engram_entry.name;

			if (include_engrams.Find(name) != INDEX_NONE)
			{
				unlock_engrams.Add({name, engram_entry.engram, engram_entry.level, engram_entry.cost});
			}
		}
	}
	else if (auto_detect)
	{
		for (const auto& engram_entry : original_engrams)
		{
			const FString name = engram_entry.name;

			if (exclude_engrams.Find(name) == INDEX_NONE)
			{
				unlock_engrams.Add({name, engram_entry.engram, engram_entry.level});
			}
		}
	}
}

bool UnlockEngrams(AShooterPlayerController* player_controller)
{
	if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
	{
		return false;
	}

	auto* primal_character = player_controller->GetPlayerCharacter();
	UPrimalCharacterStatusComponent* char_component = primal_character->GetCharacterStatusComponent();

	const bool auto_lvl = config.value("AutoDetectLevel", false);
	const int required_lvl = config.value("RequiredLevel", 1);
	const int level = char_component->GetCharacterLevel();

	if (auto_lvl || level >= required_lvl)
	{
		auto* player_state = player_controller->GetShooterPlayerState();
		for (const auto& engram_entry : unlock_engrams)
		{
			if (auto_lvl && level < engram_entry.level)
			{
				continue;
			}

			if (!player_state->HasEngram(engram_entry.engram))
			{
				if (config.value("RefundUsedEngramPoints", false))
				{
					player_state->FreeEngramPointsField() += engram_entry.cost;
					player_state->MulticastProperty(FName("FreeEngramPoints", EFindName::FNAME_Add));
				}
				player_state->ServerUnlockEngram(engram_entry.engram, false, true);
			}
		}
		return true;
	}

	return false;
}

void Hook_AShooterGameMode_InitGame(AShooterGameMode* a_shooter_game_mode, FString* map_name, FString* options,
                                    FString* error_message)
{
	AShooterGameMode_InitGame_original(a_shooter_game_mode, map_name, options, error_message);

	ReadEngrams();
}

void Hook_UPrimalCharacterStatusComponent_ServerApplyLevelUp(UPrimalCharacterStatusComponent* _this,
                                                             EPrimalCharacterStatusValue::Type LevelUpValueType,
                                                             AShooterPlayerController* ByPC)
{
	UPrimalCharacterStatusComponent_ServerApplyLevelUp_original(_this, LevelUpValueType, ByPC);

	if (ByPC != nullptr
		&& _this
		&& _this->GetOwner()
		&& _this->GetOwner()->IsA(AShooterCharacter::GetPrivateStaticClass()))
	{
		const bool use_permission = config.value("UseAutoPermission", false);

		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(ByPC);
		if (use_permission && !Permissions::IsPlayerHasPermission(steam_id, "AllEngrams.AutoGiveEngrams"))
		{
			return;
		}

		UnlockEngrams(ByPC);
	}
}

// Helper function for dumping all engrams
void DumpEngrams(APlayerController* /*unused*/, FString* /*unused*/, bool /*unused*/)
{
	std::ofstream f(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AllEngrams/EngramsDump.txt");

	for (const auto& engram_entry : original_engrams)
	{
		f << engram_entry.name.ToString() << "\n";
	}

	f.close();
}

void DumpEngrams_RCON(RCONClientConnection* conn, RCONPacket* packet, UWorld* /*unused*/)
{
	FString reply("");
	try
	{
		DumpEngrams(nullptr, nullptr, false);
		reply = "Engrams dumped!";
		conn->SendMessageW(packet->Id, 0, &reply);
	}
	catch (const std::exception& ex)
	{
		reply = ex.what();
		conn->SendMessageW(packet->Id, 0, &reply);
	}
}

void GiveEngrams(AShooterPlayerController* player_controller, FString* /*unused*/, EChatSendMode::Type /*unused*/)
{
	if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
	{
		return;
	}

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);
	if (!Permissions::IsPlayerHasPermission(steam_id, "AllEngrams.GiveEngrams"))
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, *GetText("Sender"), *GetText("NoPermissions"));
		return;
	}

	const int required_lvl = config.value("RequiredLevel", 1);

	if (UnlockEngrams(player_controller))
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, *GetText("Sender"), *GetText("Unlocked"));
	}
	else
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, *GetText("Sender"), *GetText("LowLevel"),
		                                      required_lvl);
	}
}

bool ReadIncludeEngrams()
{
	std::ifstream file(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AllEngrams/Engrams.txt");
	if (!file.good())
	{
		return false;
	}

	include_engrams.Empty();

	std::string str;
	while (getline(file, str))
	{
		include_engrams.Add(FString(str.c_str()));
	}

	file.close();

	return true;
}

bool ReadExcludeEngrams()
{
	std::ifstream file(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AllEngrams/ExcludeEngrams.txt");
	if (!file.good())
	{
		return false;
	}

	exclude_engrams.Empty();

	std::string str;
	while (getline(file, str))
	{
		exclude_engrams.Add(FString(str.c_str()));
	}

	file.close();

	return true;
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AllEngrams/config.json";
	std::ifstream file{config_path};
	if (!file.is_open())
	{
		throw std::runtime_error("Can't open config.json");
	}

	file >> config;

	file.close();
}

void ReloadCmd(APlayerController* player_controller, FString* /*unused*/, bool /*unused*/)
{
	auto* shooter_player = static_cast<AShooterPlayerController*>(player_controller);

	if (!ReadIncludeEngrams() || !ReadExcludeEngrams())
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_player, FColorList::Red, "Failed to read engrams");
		return;
	}

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_player, FColorList::Red, error.what());
		Log::GetLog()->error(error.what());
		return;
	}

	ReadEngrams();

	ArkApi::GetApiUtils().SendServerMessage(shooter_player, FColorList::Green, "Reloaded config");
}

void ReloadCmd_RCON(RCONClientConnection* conn, RCONPacket* packet, UWorld* /*unused*/)
{
	FString reply("");
	if (!ReadIncludeEngrams() || !ReadExcludeEngrams())
	{
		reply = "Failed to read engrams";
		conn->SendMessageW(packet->Id, 0, &reply);
		return;
	}

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		reply = error.what();
		conn->SendMessageW(packet->Id, 0, &reply);
		Log::GetLog()->error(error.what());
		return;
	}

	ReadEngrams();

	reply = "Reloaded Config";
	conn->SendMessageW(packet->Id, 0, &reply);
}

void Load()
{
	Log::Get().Init("AllEngrams");

	if (!ReadIncludeEngrams() || !ReadExcludeEngrams())
	{
		Log::GetLog()->error("Failed to read engrams");
		return;
	}

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	ArkApi::GetHooks().SetHook("AShooterGameMode.InitGame", &Hook_AShooterGameMode_InitGame,
	                           &AShooterGameMode_InitGame_original);

	if (config.value("AutoUnlockEngrams", true))
	{
		ArkApi::GetHooks().SetHook("UPrimalCharacterStatusComponent.ServerApplyLevelUp",
		                           &Hook_UPrimalCharacterStatusComponent_ServerApplyLevelUp,
		                           &UPrimalCharacterStatusComponent_ServerApplyLevelUp_original);
	}
	else
	{
		ArkApi::GetCommands().AddChatCommand("/GiveEngrams", &GiveEngrams);
	}

	ArkApi::GetCommands().AddConsoleCommand("DumpEngrams", &DumpEngrams);
	ArkApi::GetCommands().AddConsoleCommand("AllEngrams.Reload", &ReloadCmd);

	ArkApi::GetCommands().AddRconCommand("AllEngrams.Reload", &ReloadCmd_RCON);
	ArkApi::GetCommands().AddRconCommand("DumpEngrams", &DumpEngrams_RCON);

	if (ArkApi::GetApiUtils().GetStatus() == ArkApi::ServerStatus::Ready)
	{
		ReadEngrams();
	}
}

void Unload()
{
	ArkApi::GetHooks().DisableHook("AShooterGameMode.InitGame", &Hook_AShooterGameMode_InitGame);
	ArkApi::GetHooks().DisableHook("UPrimalCharacterStatusComponent.ServerApplyLevelUp",
	                               &Hook_UPrimalCharacterStatusComponent_ServerApplyLevelUp);

	ArkApi::GetCommands().RemoveChatCommand("/GiveEngrams");
	ArkApi::GetCommands().RemoveConsoleCommand("DumpEngrams");
	ArkApi::GetCommands().RemoveConsoleCommand("AllEngrams.Reload");
	ArkApi::GetCommands().RemoveRconCommand("AllEngrams.Reload");
	ArkApi::GetCommands().RemoveRconCommand("DumpEngrams");
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
