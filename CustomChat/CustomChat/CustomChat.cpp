#include <windows.h>
#include <fstream>
#include "CustomChat.h"
#include "json.hpp"
#include "API/Base.h"
#include "Tools.h"

#pragma comment(lib, "ArkApi.lib")

namespace CustomChat
{
	nlohmann::json json;

	void Init()
	{
		LoadConfig();
		BindCommands();
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
		auto itemsMap = json["ChatCommands"];
		for (auto iter = itemsMap.begin(); iter != itemsMap.end(); ++iter)
		{
			auto item = iter.value();

			std::string cmd = item["Command"];

			wchar_t* wCmd = Tools::ConvertToWideStr(cmd);

			Ark::AddChatCommand(wCmd, [item](AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
			                    {
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
		}
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
