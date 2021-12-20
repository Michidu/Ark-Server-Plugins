#include "SafeZones.h"

#include <windows.h>
#include <fstream>

#include "SafeZoneManager.h"
#include "Hooks.h"
#include "Commands.h"
#include "SzTools.h"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "Permissions.lib")

namespace SafeZones
{
	FString GetText(const std::string& str)
	{
		return FString(ArkApi::Tools::Utf8Decode(config.value("Messages", nlohmann::json::object()).value(str, "No message")).c_str());
	}

	void ReadConfig()
	{
		const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/SafeZone/config.json";
		std::ifstream file{config_path};
		if (!file.is_open())
			throw std::runtime_error("Can't open config.json");

		file >> config;

		file.close();
	}

	void Init()
	{
		Log::Get().Init("SafeZone");

		srand(static_cast<unsigned>(time(nullptr)));

		try
		{
			ReadConfig();
		}
		catch (const std::exception& error)
		{
			Log::GetLog()->error(error.what());
			return;
		}

		Hooks::InitHooks();
		Commands::Init();

		if (ArkApi::GetApiUtils().GetStatus() == ArkApi::ServerStatus::Ready)
		{
			SafeZones::Tools::Timer::Get().DelayExec(
				[]()
				{
					SafeZoneManager::Get().ClearAllTriggerSpheres();
					SafeZoneManager::Get().ReadSafeZones();
				},
				1, false);
		}
	}

	void Clean()
	{
		SafeZoneManager::Get().ClearAllTriggerSpheres();

		Hooks::RemoveHooks();
		Commands::Clean();
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		SafeZones::Init();
		break;
	case DLL_PROCESS_DETACH:
		SafeZones::Clean();
		break;
	}
	return TRUE;
}
