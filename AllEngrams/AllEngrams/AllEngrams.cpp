#include <fstream>

#include <API/ARK/Ark.h>
#include <API/UE/Math/ColorList.h>
#include <Permissions.h>

#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "Permissions.lib")

DECLARE_HOOK(AShooterGameMode_InitGame, void, AShooterGameMode*, FString*, FString*, FString*);

nlohmann::json config;

TArray<FEngramEntryAutoUnlock> original_engrams;

TArray<FString> include_engrams;
TArray<FString> exclude_engrams;

FString GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Messages"].value(str, "No message")).c_str());
}

void ReadEngrams()
{
	const int required_lvl = config.value("RequiredLevel", 1);
	const bool auto_detect = config.value("AutoDetectEngrams", true);

	auto& auto_unlocks = ArkApi::GetApiUtils().GetShooterGameMode()->EngramEntryAutoUnlocksField()();

	// Make a copy of original engrams list
	if (original_engrams.Num() == 0)
		original_engrams = auto_unlocks;
	else
		auto_unlocks = original_engrams;

	if (!auto_detect && include_engrams.Num() > 0)
	{
		for (const auto& include : include_engrams)
		{
			auto_unlocks.Add({include, required_lvl});
		}
	}

	if (auto_detect && exclude_engrams.Num() > 0)
	{
		for (const auto& exclude : exclude_engrams)
		{
			auto_unlocks.RemoveAll([&exclude](const auto& unlock)
			{
				return unlock.EngramClassName == exclude;
			});
		}
	}

	for (auto& item : auto_unlocks)
	{
		item.LevelToAutoUnlock = required_lvl;
	}
}

void Hook_AShooterGameMode_InitGame(AShooterGameMode* a_shooter_game_mode, FString* map_name, FString* options,
                                    FString* error_message)
{
	const bool auto_unlock = config.value("AutoUnlockEngrams", true);

	a_shooter_game_mode->bAutoUnlockAllEngramsField() = auto_unlock;

	AShooterGameMode_InitGame_original(a_shooter_game_mode, map_name, options, error_message);

	if (auto_unlock)
		ReadEngrams();
}

// Helper function for dumping all engrams
void DumpEngrams(APlayerController*, FString*, bool)
{
	std::ofstream f(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AllEngrams/EngramsDump.txt");

	const auto& auto_unlocks = ArkApi::GetApiUtils().GetShooterGameMode()->EngramEntryAutoUnlocksField()();
	for (const auto& item : auto_unlocks)
	{
		f << item.EngramClassName.ToString() << "\n";
	}

	f.close();
}

void GiveEngrams(AShooterPlayerController* player_controller, FString*, EChatSendMode::Type)
{
	if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);
	if (!Permissions::IsPlayerHasPermission(steam_id, "AllEngrams.GiveEngrams"))
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, *GetText("Sender"), *GetText("NoPermissions"));
		return;
	}

	APrimalCharacter* primal_character = static_cast<APrimalCharacter*>(player_controller->CharacterField()());
	UPrimalCharacterStatusComponent* char_component = primal_character->MyCharacterStatusComponentField()();

	const int required_lvl = config.value("RequiredLevel", 1);
	const int level = char_component->BaseCharacterLevelField()() + char_component->ExtraCharacterLevelField()();
	if (level >= required_lvl)
	{
		AShooterPlayerState* player_state = static_cast<AShooterPlayerState*>(player_controller->PlayerStateField()());

		TArray<UPrimalEngramEntry*> all_engrams_entries = Globals::GEngine()()
			->GameSingletonField()()->PrimalGameDataOverrideField()()->
			  EngramBlueprintEntriesField()();

		for (UPrimalEngramEntry* engram_entry : all_engrams_entries)
		{
			player_state->ServerUnlockEngram(engram_entry->BluePrintEntryField()(), false, true);
		}

		ArkApi::GetApiUtils().SendChatMessage(player_controller, *GetText("Sender"), *GetText("Unlocked"));
	}
	else
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, *GetText("Sender"), *GetText("LowLevel"), required_lvl);
	}
}

bool ReadIncludeEngrams()
{
	std::ifstream file(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AllEngrams/Engrams.txt");
	if (!file.good())
		return false;

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
		return false;

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
		throw std::runtime_error("Can't open config.json");

	file >> config;

	file.close();
}

void ReloadCmd(APlayerController* player_controller, FString*, bool)
{
	AShooterPlayerController* shooter_player = static_cast<AShooterPlayerController*>(player_controller);

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

	if (!config.value("AutoUnlockEngrams", true))
		ArkApi::GetCommands().AddChatCommand("/GiveEngrams", &GiveEngrams);

	ArkApi::GetCommands().AddConsoleCommand("DumpEngrams", &DumpEngrams);
	ArkApi::GetCommands().AddConsoleCommand("AllEngrams.Reload", &ReloadCmd);
}

void Unload()
{
	ArkApi::GetHooks().DisableHook("AShooterGameMode.InitGame", &Hook_AShooterGameMode_InitGame);

	ArkApi::GetCommands().RemoveChatCommand("/GiveEngrams");
	ArkApi::GetCommands().RemoveConsoleCommand("DumpEngrams");
	ArkApi::GetCommands().RemoveConsoleCommand("AllEngrams.Reload");
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
