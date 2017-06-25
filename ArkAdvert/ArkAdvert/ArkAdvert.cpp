#include <windows.h>
#include <chrono>
#include <fstream>
#include "ArkAdvert.h"
#include "json.hpp"
#include "Tools.h"

#pragma comment(lib, "ArkApi.lib")

namespace ArkAdvert
{
	DECLARE_HOOK(AShooterGameMode_StartNewShooterPlayer, void, AShooterGameMode *, APlayerController *, bool , bool , FPrimalPlayerCharacterConfigStruct *, UPrimalPlayerData *);

	void ReloadAdvertConfig(APlayerController* playerController, FString* cmd, bool shouldLog);

	nlohmann::json json;

	std::chrono::time_point<std::chrono::system_clock> NextAdvTime;

	void Init()
	{
		LoadConfig();

		bool welcomeMsgEnabled = json["AdvertMessages"]["WelcomeMsgEnabled"];
		if (welcomeMsgEnabled)
			Ark::SetHook("AShooterGameMode", "StartNewShooterPlayer", &Hook_AShooterGameMode_StartNewShooterPlayer, reinterpret_cast<LPVOID*>(&AShooterGameMode_StartNewShooterPlayer_original));

		int interval = json["AdvertMessages"]["Interval"];
		NextAdvTime = std::chrono::system_clock::now() + std::chrono::seconds(interval);

		Ark::AddOnTimerCallback(&AdvertTimer);

		Ark::AddConsoleCommand(L"ReloadAdvertConfig", &ReloadAdvertConfig);
	}

	void LoadConfig()
	{
		std::ifstream file(Tools::GetCurrentDir() + "/BeyondApi/Plugins/ArkAdvert/config.json");
		if (!file.is_open())
		{
			std::cout << "Could not open file config.json" << std::endl;
			throw;
		}

		file >> json;
		file.close();
	}

	void _cdecl Hook_AShooterGameMode_StartNewShooterPlayer(AShooterGameMode* _this, APlayerController* NewPlayer, bool bForceCreateNewPlayerData, bool bIsFromLogin, FPrimalPlayerCharacterConfigStruct* charConfig, UPrimalPlayerData* ArkPlayerData)
	{
		AShooterGameMode_StartNewShooterPlayer_original(_this, NewPlayer, bForceCreateNewPlayerData, bIsFromLogin, charConfig, ArkPlayerData);

		if (bIsFromLogin)
		{
			AShooterPlayerController* player = static_cast<AShooterPlayerController*>(NewPlayer);

			Tools::Timer(5000, true, &WelcomeMsg, player);
		}
	}

	void WelcomeMsg(AShooterPlayerController* player)
	{
		std::string msg = json["AdvertMessages"]["WelcomeMsg"];
		auto configColor = json["AdvertMessages"]["WelcomeMsgColor"];
		FLinearColor color = {configColor[0], configColor[1], configColor[2], configColor[3]};

		Tools::SendColoredMessage(player, TEXT("%hs"), color, msg.c_str());
	}

	void AdvertTimer()
	{
		auto now = std::chrono::system_clock::now();

		auto diff = std::chrono::duration_cast<std::chrono::seconds>(NextAdvTime - now);

		if (diff.count() <= 0)
		{
			auto messagesMap = json["AdvertMessages"]["Messages"];

			int size = static_cast<int>(messagesMap.size()) - 1;
			int rnd = Tools::GetRandomNumber(0, size);

			auto messageEntry = messagesMap[rnd];

			std::string message = messageEntry["Message"];

			std::string type = messageEntry["Type"];
			if (type == "ServerChat")
			{
				auto configColor = messageEntry["Color"];
				FLinearColor color = {configColor[0], configColor[1], configColor[2], configColor[3]};

				Tools::SendColoredMessageToAll(TEXT("%hs"), color, message.c_str());
			}
			else
			{
				Tools::SendChatMessageToAll(TEXT("SERVER"), TEXT("%hs"), message.c_str());
			}

			int interval = json["AdvertMessages"]["Interval"];
			NextAdvTime = now + std::chrono::seconds(interval);
		}
	}

	void ReloadAdvertConfig(APlayerController* playerController, FString* cmd, bool shouldLog)
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
		ArkAdvert::Init();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
