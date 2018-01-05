#include <API/ARK/Ark.h>
#include <Logger/Logger.h>

#include <fstream>

#pragma comment(lib, "ArkApi.lib")

TArray<FString> all_engrams;

// Helper function for dumping all learnt engrams
void DumpEngrams(APlayerController* player_controller, FString* message, bool)
{
	AShooterPlayerState* player_state = static_cast<AShooterPlayerState*>(player_controller->PlayerStateField()());

	std::ofstream f("Engrams.txt");

	auto& engrams = player_state->EngramItemBlueprintsField()();
	for (const auto& item : engrams)
	{
		FString asset_name;
		item.uClass->GetFullName(&asset_name, nullptr);

		f << asset_name.ToString() << "\n";
	}

	f.close();
}

void GiveEngrams(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type mode)
{
	UShooterCheatManager* cheat_manager = static_cast<UShooterCheatManager*>(player_controller->CheatManagerField()());

	for (FString& engram : all_engrams)
	{
		cheat_manager->UnlockEngram(&engram);
	}
}

bool ReadEngrams()
{
	std::ifstream file(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AllEngrams/Engrams.txt");
	if (!file.good())
		return false;

	std::string str;
	while (getline(file, str))
	{
		all_engrams.Add(FString(str.c_str()));
	}

	file.close();

	return true;
}

void Load()
{
	Log::Get().Init("AllEngrams");

	if (!ReadEngrams())
	{
		Log::GetLog()->error("Failed to read engrams");
		return;
	}

	ArkApi::GetCommands().AddChatCommand("/GiveEngrams", &GiveEngrams);
	ArkApi::GetCommands().AddConsoleCommand("DumpEngrams", &DumpEngrams);
}

void Unload()
{
	ArkApi::GetCommands().RemoveChatCommand("/GiveEngrams");
	ArkApi::GetCommands().RemoveConsoleCommand("DumpEngrams");
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
