#ifdef PLUGIN_ARK
#include <API/ARK/Ark.h>
#else
#include <API/Atlas/Atlas.h>
#endif

#include <fstream>
#include "json.hpp"

#include "Helper.h"

#ifdef PLUGIN_ARK
#pragma comment(lib, "ArkApi.lib")
#else
#pragma comment(lib, "AtlasApi.lib")
#endif

DECLARE_HOOK(ReportCrash, int, void*);

nlohmann::json config;

void RestartServer()
{
	const DWORD id = GetCurrentProcessId();

	const std::wstring current_dir = ArkApi::Tools::Utf8Decode(ArkApi::Tools::GetCurrentDir());

	Helper::LaunchApp(current_dir + LR"(\ShooterGameServer.exe)", GetCommandLineW());

	Log::GetLog()->warn("Restarted server");

	system(fmt::format("taskkill /PID {}", id).c_str());
}

int Hook_ReportCrash(void* e)
{
	Log::GetLog()->warn("Server crash detected");

	const bool save_word = config["SaveWorld"];
	if (save_word)
	{
		Log::GetLog()->warn("Saving world..");

#ifdef PLUGIN_ARK
		ArkApi::GetApiUtils().GetShooterGameMode()->SaveWorld();
#else
		ArkApi::GetApiUtils().GetShooterGameMode()->SaveWorld(ESaveWorldType::Normal, true);
#endif

		Log::GetLog()->warn("Saved world!");
	}

	const bool should_restart = config.value("ShouldRestart", true);
	if (should_restart)
	{
		Helper::Timer(10, true, &RestartServer);
	}

	return ReportCrash_original(e);
}

std::string GetConfigPath()
{
#ifdef PLUGIN_ARK
	return ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/PluginCrashManager/config.json";
#else
	return ArkApi::Tools::GetCurrentDir() + "/AtlasApi/Plugins/PluginCrashManager/config.json";
#endif
}

void ReadConfig()
{
	const std::string config_path = GetConfigPath();
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
	Log::Get().Init("PluginCrashManager");

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
