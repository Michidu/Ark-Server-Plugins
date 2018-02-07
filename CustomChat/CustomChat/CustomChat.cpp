#include <fstream>

#include <API/ARK/Ark.h>
#include <API/UE/Math/ColorList.h>
#include <Logger/Logger.h>

#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

nlohmann::json config;

void BindCommands()
{
	int i = 0;

	auto items_map = config["ChatCommands"];
	for (const auto& item : items_map)
	{
		std::string cmd = item["Command"];

		ArkApi::GetCommands().
			AddChatCommand(cmd.c_str(),
			               [i](AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type mode)
			               {
				               auto item = config["ChatCommands"][i];

				               std::wstring reply = ArkApi::Tools::Utf8Decode(item["Reply"]);
				               std::string type = item["Type"];

				               if (type == "ServerChat")
				               {
					               auto config_color = item["Color"];
					               FLinearColor color{config_color[0], config_color[1], config_color[2], config_color[3]};

					               ArkApi::GetApiUtils().SendServerMessage(player_controller, color, reply.c_str());
				               }
				               else if (type == "ClientChat")
				               {
					               ArkApi::GetApiUtils().SendChatMessage(player_controller, L"Server", reply.c_str());
				               }
				               else if (type == "Notification")
				               {
					               float display_scale = item["DisplayScale"];
					               float display_time = item["DisplayTime"];

					               auto config_color = item["Color"];
					               FLinearColor color{config_color[0], config_color[1], config_color[2], config_color[3]};

					               ArkApi::GetApiUtils().SendNotification(player_controller, color, display_scale, display_time,
					                                                      nullptr, reply.c_str());
				               }
			               });

		++i;
	}
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/CustomChat/config.json";
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
	Log::Get().Init("CustomChat");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	BindCommands();

	ArkApi::GetCommands().AddConsoleCommand("CustomChat.Reload", &ReloadConfig);
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
