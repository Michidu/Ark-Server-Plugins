#include <API/ARK/Ark.h>

#include <fstream>
#include "json.hpp"

#include "Helper.h"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(ReportCrash, int, void*);

nlohmann::json config;

void RestartServer()
{
	const std::wstring current_dir = ArkApi::Tools::Utf8Decode(ArkApi::Tools::GetCurrentDir());

	Helper::LaunchApp(current_dir + LR"(\ShooterGameServer.exe)", GetCommandLineW());

	Log::GetLog()->warn("Restarted server");

	TerminateProcess(GetCurrentProcess(), 3);
}

int Hook_ReportCrash(void* e)
{
	Log::GetLog()->warn("Server crash detected");

	const bool save_word = config["SaveWorld"];
	if (save_word)
	{
		Log::GetLog()->warn("Saving world..");
		ArkApi::GetApiUtils().GetShooterGameMode()->SaveWorld();
		Log::GetLog()->warn("Saved world!");
	}

	Helper::Timer(5, true, &RestartServer);

	return ReportCrash_original(e);
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/PluginServerManager/config.json";
	std::ifstream file{config_path};
	if (!file.is_open())
	{
		throw std::runtime_error("Can't open config.json");
	}

	file >> config;

	file.close();
}

void Load()
{
	Log::Get().Init("PluginServerManager");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	ArkApi::GetHooks().SetHook("Global.ReportCrash", &Hook_ReportCrash, &ReportCrash_original);
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
