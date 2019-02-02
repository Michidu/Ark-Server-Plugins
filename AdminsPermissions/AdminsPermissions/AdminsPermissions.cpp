#ifdef PERMISSIONS_ARK
#include <ArkPermissions.h>
#else
#include <AtlasPermissions.h>
#endif

#include <fstream>

#include "json.hpp"

#ifdef PERMISSIONS_ARK
#pragma comment(lib, "ArkApi.lib")
#else
#pragma comment(lib, "AtlasApi.lib")
#endif

#pragma comment(lib, "Permissions.lib")

nlohmann::json config;

std::string GetConfigPath()
{
#ifdef PERMISSIONS_ARK
	return ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AdminsPermissions/config.json";
#else
	return ArkApi::Tools::GetCurrentDir() + "/AtlasApi/Plugins/AdminsPermissions/config.json";
#endif
}

FString GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Messages"].value(str, "No message")).c_str());
}

void ConsoleCommand(APlayerController* player, FString command)
{
	const bool is_admin = player->bIsAdmin()(), is_cheat = player->bCheatPlayer()();
	player->bIsAdmin() = true;
	player->bCheatPlayer() = true;

	FString result;
	player->ConsoleCommand(&result, &command, true);

	player->bIsAdmin() = is_admin;
	player->bCheatPlayer() = is_cheat;
}

void OnChatMessage(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
{
	TArray<FString> parsed;
	message->ParseIntoArray(parsed, L" ", true);

	if (!parsed.IsValidIndex(1))
		return;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

	if (Permissions::IsPlayerHasPermission(steam_id, "Cheat." + parsed[1]))
	{
		if (!message->RemoveFromStart("/cheat "))
			return;

		ConsoleCommand(player_controller, *message);

		FString log = FString::Format(*GetText("LogMsg"), *ArkApi::IApiUtils::GetCharacterName(player_controller),
		                              **message);

		const bool print_to_chat = config["PrintToChat"];
		if (print_to_chat)
			ArkApi::GetApiUtils().SendServerMessageToAll(FColorList::Yellow, *log);

		ArkApi::GetApiUtils().GetShooterGameMode()->PrintToGameplayLog(&log);
	}
	else
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, *GetText("Sender"), *GetText("NoPermissions"));
	}
}

void ReadConfig()
{
	const std::string config_path = GetConfigPath();
	std::ifstream file{config_path};
	if (!file.is_open())
		throw std::runtime_error("Can't open config.json");

	file >> config;

	file.close();
}

void Load()
{
	Log::Get().Init("AdminsPermissions");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	ArkApi::GetCommands().AddChatCommand("/cheat", &OnChatMessage);
}

void Unload()
{
	ArkApi::GetCommands().RemoveChatCommand("/cheat");
}

BOOL APIENTRY DllMain(HMODULE, DWORD ul_reason_for_call, LPVOID)
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
