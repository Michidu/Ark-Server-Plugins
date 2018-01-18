#include "Main.h"

#include <fstream>

#include <API/ARK/Ark.h>
#include <API/UE/Math/ColorList.h>

#include "../Public/Permissions.h"

#pragma comment(lib, "ArkApi.lib")

nlohmann::json config;

void AddPlayerToGroup(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		uint64 steam_id;

		FString group = *parsed[2];

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = Permissions::AddPlayerToGroup(steam_id, group.ToString());
		if (result)
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully added player");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't add player");
	}
}

void RemovePlayerFromGroup(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		uint64 steam_id;

		FString group = *parsed[2];

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = Permissions::RemovePlayerFromGroup(steam_id, group.ToString());
		if (result)
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully removed player");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't remove player");
	}
}

void RemoveGroup(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		FString group = *parsed[1];

		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = Permissions::RemoveGroup(group.ToString());
		if (result)
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully removed group");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't remove group");
	}
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/Permissions/config.json";
	std::ifstream file{ config_path };
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
	Log::Get().Init("Permission");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	ArkApi::GetCommands().AddConsoleCommand("Permissions.Reload", &ReloadConfig);

	ArkApi::GetCommands().AddConsoleCommand("Permissions.Add", &AddPlayerToGroup);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.Remove", &RemovePlayerFromGroup);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.RemoveGroup", &RemoveGroup);
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
