#include <fstream>

#include <API/ARK/Ark.h>
#include <Logger/Logger.h>
#include <Permissions.h>

#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "Permissions.lib")

nlohmann::json config;

TArray<FString> all_engrams;

FString GetText(const std::string& str)
{
	return FString(config["Messages"].value(str, "No message").c_str());
}

// Helper function for dumping all learnt engrams
void DumpEngrams(APlayerController* player_controller, FString*, bool)
{
	AShooterPlayerState* player_state = static_cast<AShooterPlayerState*>(player_controller->PlayerStateField()());

	std::ofstream f("Engrams.txt");

	auto& engrams = player_state->EngramItemBlueprintsField()();
	for (const auto& item : engrams)
	{
		FString asset_name;
		item.uClass->GetFullName(&asset_name, nullptr);

		f << asset_name.ToString() << "\n";
	}

	f.close();
}

bool CanUseCommand(uint64 steam_id)
{
	const std::string permissions = config.value("Permissions", "");
	if (permissions.empty())
		return true;

	const FString fpermissions(permissions.c_str());

	TArray<FString> groups;
	fpermissions.ParseIntoArray(groups, L",", true);

	for (const auto& group : groups)
	{
		if (Permissions::IsPlayerInGroup(steam_id, group))
			return true;
	}

	return false;
}

void GiveEngrams(AShooterPlayerController* player_controller, FString*, EChatSendMode::Type)
{
	if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);
	if (!CanUseCommand(steam_id))
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, *GetText("Sender"), *GetText("NoPermissions"));
		return;
	}

	APrimalCharacter* primal_character = static_cast<APrimalCharacter*>(player_controller->CharacterField()());
	UPrimalCharacterStatusComponent* char_component = primal_character->MyCharacterStatusComponentField()();

	const int required_lvl = config.value("RequiredLevel", 0);
	const int level = char_component->BaseCharacterLevelField()() + char_component->ExtraCharacterLevelField()();
	if (level >= required_lvl)
	{
		UShooterCheatManager* cheat_manager = static_cast<UShooterCheatManager*>(player_controller->CheatManagerField()());

		for (FString& engram : all_engrams)
		{
			cheat_manager->UnlockEngram(&engram);
		}

		AShooterPlayerState* player_state = static_cast<AShooterPlayerState*>(player_controller->PlayerStateField()());

		const int points_amount = config.value("AmountOfPoints", 0);
		player_state->FreeEngramPointsField() = points_amount;
	}
	else
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, *GetText("Sender"), *GetText("LowLevel"), required_lvl);
	}
}

bool ReadEngrams()
{
	std::ifstream file(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AllEngrams/Engrams.txt");
	if (!file.good())
		return false;

	std::string str;
	while (getline(file, str))
	{
		all_engrams.Add(FString(str.c_str()));
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

void Load()
{
	Log::Get().Init("AllEngrams");

	if (!ReadEngrams())
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

	ArkApi::GetCommands().AddChatCommand("/GiveEngrams", &GiveEngrams);
	ArkApi::GetCommands().AddConsoleCommand("DumpEngrams", &DumpEngrams);
}

void Unload()
{
	ArkApi::GetCommands().RemoveChatCommand("/GiveEngrams");
	ArkApi::GetCommands().RemoveConsoleCommand("DumpEngrams");
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
