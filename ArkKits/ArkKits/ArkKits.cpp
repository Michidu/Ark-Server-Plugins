#include <windows.h>
#include <iostream>
#include <fstream>
#include "API/Base.h"
#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

nlohmann::json json;
nlohmann::json jsonPlayers;

void LoadConfig();
void LoadPlayersConfig();
void SavePlayersConfig();

void GiveKit(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
{
	std::string playerIdStr = std::to_string(playerController->GetLinkedPlayerIDField());

	int totalKitsAmount = json["AmountOfKits"];

	int kitsLeft = jsonPlayers["players"].value(playerIdStr, totalKitsAmount);
	if (kitsLeft > 0)
	{
		// Iterate through all items in config and give them to player
		auto itemsMap = json["Items"];
		for (auto iter = itemsMap.begin(); iter != itemsMap.end(); ++iter)
		{
			playerController->GiveItemNum(std::stoi(iter.key()), iter.value(), 0, false);
		}

		// Reduce kits amount and save it
		jsonPlayers["players"][playerIdStr] = --kitsLeft;

		SavePlayersConfig();
	}

	// Send chat message

	wchar_t buffer[256];
	swprintf_s(buffer, TEXT("You have %d starter kits left"), kitsLeft);

	FString cmd(buffer);

	FLinearColor msgColor = {1,1,1,1};
	playerController->ClientServerChatDirectMessage(&cmd, msgColor, false);
}

void SavePlayersConfig()
{
	std::ofstream file("BeyondApi/Plugins/ArkKits/playersConfig.json");
	file << jsonPlayers.dump(2);
	file.close();
}

void Init()
{
	LoadConfig();
	LoadPlayersConfig();

	Ark::AddChatCommand(L"/kit", &GiveKit);
}

void LoadConfig()
{
	std::ifstream file("BeyondApi/Plugins/ArkKits/config.json");
	if (!file.is_open())
	{
		std::cout << "Could not open file config.json" << std::endl;
		return;
	}

	file >> json;
	file.close();
}

void LoadPlayersConfig()
{
	std::ifstream file("BeyondApi/Plugins/ArkKits/playersConfig.json");
	if (!file.is_open())
	{
		std::cout << "Could not open file playersConfig.json" << std::endl;
		return;
	}

	file >> jsonPlayers;
	file.close();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Init();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
