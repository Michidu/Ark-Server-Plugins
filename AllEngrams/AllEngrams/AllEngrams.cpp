#include <windows.h>
#include <fstream>
#include <iostream>
#include "AllEngrams.h"
#include "Tools.h"

#pragma comment(lib, "ArkApi.lib")

namespace
{
	// Helper function for dumping all learnt engrams (Not used)
	void DumpEngrams(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
	{
		AShooterPlayerState* playerState = static_cast<AShooterPlayerState*>(playerController->GetPlayerStateField());
		auto engrams = playerState->GetEngramItemBlueprintsField();

		std::ofstream f("Engrams.txt");

		for (uint32_t i = 0; i < engrams.Num(); ++i)
		{
			auto item = engrams[i];

			FString assetName;
			item.uClass->GetFullName(&assetName, nullptr);

			f << assetName.ToString() << "\n";
		}

		f.close();
	}

	void GiveEngrams(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
	{
		UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(playerController->GetCheatManagerField());

		std::wifstream file(Tools::GetCurrentDir() + "/BeyondApi/Plugins/AllEngrams/Engrams.txt");
		if (!file.good())
		{
			std::cerr << "Can't open Engrams.txt\n";
			return;
		}

		std::wstring str;
		while (getline(file, str))
		{
			FString fStr = &str[0];
			cheatManager->UnlockEngram(&fStr);
		}

		file.close();
	}

	void Init()
	{
		Ark::AddChatCommand(L"/GiveEngrams", &GiveEngrams);
	}
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
