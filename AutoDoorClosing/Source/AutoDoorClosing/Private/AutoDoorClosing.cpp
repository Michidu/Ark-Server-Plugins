// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AutoDoorClosingPrivatePCH.h"
#include "AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <fstream>
#include "json.hpp"
#include "API/Base.h"

DECLARE_HOOK(GotoDoorState, void, APrimalStructureDoor*, char);

void LoadConfig();

// Timer
float ClosingTimer = 5.0;

void Init()
{
	LoadConfig();

	Ark::SetHook("APrimalStructureDoor", "GotoDoorState", &Hook_GotoDoorState, reinterpret_cast<LPVOID*>(&GotoDoorState_original));
}

void LoadConfig()
{
	nlohmann::json json;

	std::ifstream file("BeyondApi/Plugins/AutoDoorClosing/config.json");
	if (!file.is_open())
	{
		std::cout << "Could not open file config.json" << std::endl;
		return;
	}

	file >> json;
	file.close();

	ClosingTimer = json["ClosingTimer"];
}

void _cdecl Hook_GotoDoorState(APrimalStructureDoor* _APrimalStructureDoor, char DoorState)
{
	GotoDoorState_original(_APrimalStructureDoor, DoorState);

	if (DoorState == static_cast<char>(1) || DoorState == static_cast<char>(2))
	{
		_APrimalStructureDoor->DelayedGotoDoorState(static_cast<char>(0), ClosingTimer);
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

#define LOCTEXT_NAMESPACE "FAutoDoorClosingModule"

void FAutoDoorClosingModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FAutoDoorClosingModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAutoDoorClosingModule, AutoDoorClosing)