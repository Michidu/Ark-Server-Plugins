#include <API/ARK/Ark.h>
#include <API/UE/Math/ColorList.h>
#include <Logger/Logger.h>
#include <Permissions.h>

#include <fstream>

#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "Permissions.lib")

nlohmann::json config;

FString GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Messages"].value(str, "No message")).c_str());
}

void OnChatMessage(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
{
	TArray<FString> parsed;
	message->ParseIntoArray(parsed, L" ", true);

	if (!parsed.IsValidIndex(1))
		return;

	const uint64 steam_id = ArkApi::GetApiUtils().GetSteamIdFromController(player_controller);

	if (Permissions::IsPlayerHasPermission(steam_id, "Cheat." + parsed[1]))
	{
		if (!message->RemoveFromStart("/cheat "))
			return;

		FString result;
		player_controller->ConsoleCommand(&result, message, true);

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
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AdminsPermissions/config.json";
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
