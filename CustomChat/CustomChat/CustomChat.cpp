#include <windows.h>
#include <fstream>
#include "CustomChat.h"
#include "json.hpp"
#include "API/Base.h"
#include "Tools.h"

#pragma comment(lib, "ArkApi.lib")

namespace CustomChat
{
	void ReloadChatConfig(APlayerController* playerController, FString* cmd, bool shouldLog);

	nlohmann::json json;

	void Init()
	{
		LoadConfig();
		BindCommands();

		Ark::AddConsoleCommand(L"ReloadChatConfig", &ReloadChatConfig);
	}

	void LoadConfig()
	{
		std::ifstream file(Tools::GetCurrentDir() + "/BeyondApi/Plugins/CustomChat/config.json");
		if (!file.is_open())
		{
			std::cout << "Could not open file config.json" << std::endl;
			throw;
		}

		file >> json;
		file.close();
	}

	void BindCommands()
	{
		int i = 0;

		auto itemsMap = json["ChatCommands"];
		for (auto iter = itemsMap.begin(); iter != itemsMap.end(); ++iter)
		{
			auto item = iter.value();

			std::string cmd = item["Command"];

			wchar_t* wCmd = Tools::ConvertToWideStr(cmd);

			Ark::AddChatCommand(wCmd, [i](AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
			                    {
				                    auto item = json["ChatCommands"][i];

				                    std::string reply = item["Reply"];
				                    std::string type = item["Type"];

				                    wchar_t* msg = Tools::ConvertToWideStr(reply);

				                    if (type == "ServerChat")
				                    {
					                    auto configColor = item["Color"];
					                    FLinearColor color = {configColor[0], configColor[1], configColor[2], configColor[3]};

					                    Tools::SendColoredMessage(playerController, msg, color);
				                    }
				                    else if (type == "ClientChat")
				                    {
					                    Tools::SendChatMessage(playerController, TEXT("SERVER"), msg);
				                    }
				                    else if (type == "Notification")
				                    {
					                    float displayScale = item["DisplayScale"];
					                    float displayTime = item["DisplayTime"];

					                    auto configColor = item["Color"];
					                    FLinearColor color = {configColor[0], configColor[1], configColor[2], configColor[3]};

					                    Tools::SendNotification(playerController, msg, color, displayScale, displayTime);
				                    }

				                    delete[] msg;
			                    });
			++i;
		}
	}

	void ReloadChatConfig(APlayerController* playerController, FString* cmd, bool shouldLog)
	{
		LoadConfig();

		AShooterPlayerController* aShooterController = static_cast<AShooterPlayerController*>(playerController);

		Tools::SendDirectMessage(aShooterController, TEXT("Reloaded config"));
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CustomChat::Init();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
