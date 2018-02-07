#include <chrono>
#include <fstream>

#include <API/ARK/Ark.h>
#include <Logger/Logger.h>
#include <API/UE/Math/ColorList.h>
#include <Tools.h>

#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(AShooterGameMode_StartNewShooterPlayer, void, AShooterGameMode *, APlayerController *, bool, bool,
	FPrimalPlayerCharacterConfigStruct *, UPrimalPlayerData *);

nlohmann::json config;
std::chrono::time_point<std::chrono::system_clock> next_adv_time;

void WelcomeMsg(AShooterPlayerController* player)
{
	std::wstring msg = ArkApi::Tools::Utf8Decode(config["AdvertMessages"]["WelcomeMsg"]);
	auto config_color = config["AdvertMessages"]["WelcomeMsgColor"];
	const FLinearColor color{config_color[0], config_color[1], config_color[2], config_color[3]};

	ArkApi::GetApiUtils().SendServerMessage(player, color, msg.c_str());
}

void Hook_AShooterGameMode_StartNewShooterPlayer(AShooterGameMode* _this, APlayerController* new_player,
                                                 bool force_create_new_player_data, bool is_from_login,
                                                 FPrimalPlayerCharacterConfigStruct* char_config,
                                                 UPrimalPlayerData* ark_player_data)
{
	AShooterGameMode_StartNewShooterPlayer_original(_this, new_player, force_create_new_player_data, is_from_login,
	                                                char_config,
	                                                ark_player_data);

	if (is_from_login)
	{
		AShooterPlayerController* player = static_cast<AShooterPlayerController*>(new_player);

		Timer(5000, true, &WelcomeMsg, player);
	}
}

void AdvertTimer()
{
	const auto now = std::chrono::system_clock::now();

	auto diff = std::chrono::duration_cast<std::chrono::seconds>(next_adv_time - now);

	if (diff.count() <= 0)
	{
		auto messages_map = config["AdvertMessages"]["Messages"];

		const int size = static_cast<int>(messages_map.size()) - 1;
		const int rnd = GetRandomNumber(0, size);

		auto message_entry = messages_map[rnd];

		std::wstring message = ArkApi::Tools::Utf8Decode(message_entry["Message"]);

		const std::string type = message_entry["Type"];
		if (type == "ServerChat")
		{
			auto config_color = message_entry["Color"];
			const FLinearColor color{config_color[0], config_color[1], config_color[2], config_color[3]};

			ArkApi::GetApiUtils().SendServerMessageToAll(color, message.c_str());
		}
		else if (type == "ClientChat")
		{
			ArkApi::GetApiUtils().SendChatMessageToAll(L"Server", message.c_str());
		}
		else if (type == "Notification")
		{
			const float display_scale = message_entry["DisplayScale"];
			const float display_time = message_entry["DisplayTime"];

			auto config_color = message_entry["Color"];
			const FLinearColor color{config_color[0], config_color[1], config_color[2], config_color[3]};

			ArkApi::GetApiUtils().SendNotificationToAll(color, display_scale, display_time,
			                                            nullptr, message.c_str());
		}

		const int interval = config["AdvertMessages"]["Interval"];
		next_adv_time = now + std::chrono::seconds(interval);
	}
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/ArkAdvert/config.json";
	std::ifstream file{config_path};
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
	Log::Get().Init("ArkAdvert");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	srand(time(nullptr));

	const bool welcome_msg_enabled = config["AdvertMessages"]["WelcomeMsgEnabled"];
	if (welcome_msg_enabled)
		ArkApi::GetHooks().SetHook("AShooterGameMode.StartNewShooterPlayer", &Hook_AShooterGameMode_StartNewShooterPlayer,
		                           &AShooterGameMode_StartNewShooterPlayer_original);

	const int interval = config["AdvertMessages"]["Interval"];
	next_adv_time = std::chrono::system_clock::now() + std::chrono::seconds(interval);

	ArkApi::GetCommands().AddOnTimerCallback("AdvertTimer", &AdvertTimer);
	ArkApi::GetCommands().AddConsoleCommand("Advert.Reload", &ReloadConfig);
}

void Unload()
{
	const bool welcome_msg_enabled = config["AdvertMessages"]["WelcomeMsgEnabled"];
	if (welcome_msg_enabled)
		ArkApi::GetHooks().DisableHook("AShooterGameMode.StartNewShooterPlayer",
		                               &Hook_AShooterGameMode_StartNewShooterPlayer);

	ArkApi::GetCommands().RemoveOnTimerCallback("AdvertTimer");
	ArkApi::GetCommands().RemoveConsoleCommand("Advert.Reload");
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
