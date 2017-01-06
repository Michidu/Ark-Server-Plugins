// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "StarterKitPrivatePCH.h"
#include "AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include "API/Base.h"
#include "json.hpp"

nlohmann::json json;
nlohmann::json jsonPlayers;

void LoadConfig();
void LoadPlayersConfig();
void SavePlayersConfig();

void GiveKit(AShooterPlayerController* _AShooterPlayerController, FString* message, int mode)
{
	std::string playerIdStr = std::to_string(_AShooterPlayerController->GetLinkedPlayerID());

	int totalKitsAmount = json["AmountOfKits"];

	int kitsLeft = jsonPlayers["players"].value(playerIdStr, totalKitsAmount);
	if (kitsLeft > 0)
	{
		// Iterate through all items in config and give them to player
		auto itemsMap = json["Items"];
		for (auto iter = itemsMap.begin(); iter != itemsMap.end(); ++iter)
		{
			_AShooterPlayerController->GiveItemNum(std::stoi(iter.key()), iter.value(), 0, false);
		}

		// Reduce kits amount and save it
		jsonPlayers["players"][playerIdStr] = --kitsLeft;

		SavePlayersConfig();
	}

	// ConsoleCommand is 'better' in APlayerController
	APlayerController* playerController = static_cast<APlayerController*>(_AShooterPlayerController);

	// Send chat message
	FString steamName = playerController->GetPlayerState()->GetPlayerName();

	FString res;
	FString cmd = "ServerChatToPlayer \"" + steamName + "\" You have " + FString::FromInt(kitsLeft) + " starter kits left";
	playerController->ConsoleCommand(&res, cmd, false);
}

void SavePlayersConfig()
{
	std::ofstream file("BeyondApi/Plugins/StarterKit/playersConfig.json");
	file << jsonPlayers.dump(4);
	file.close();
}

void Init()
{
	LoadConfig();
	LoadPlayersConfig();

	Ark::AddChatCommand("/kit", &GiveKit);
}

void LoadConfig()
{
	std::ifstream file("BeyondApi/Plugins/StarterKit/config.json");
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
	std::ifstream file("BeyondApi/Plugins/StarterKit/playersConfig.json");
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

#define LOCTEXT_NAMESPACE "FStarterKitModule"

void FStarterKitModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FStarterKitModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FStarterKitModule, StarterKit)
