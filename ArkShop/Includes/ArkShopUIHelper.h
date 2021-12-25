#pragma once

#include <API/ARK/Ark.h>

#ifdef ARKSHOPUI_EXPORTS
#define ARK_API __declspec(dllexport)
#else
#define ARK_API
#endif

namespace ArkShopUI
{
	inline ARK_API bool RequestUI(AShooterPlayerController* player_controller)
	{
		using T = bool (*) (AShooterPlayerController*);

		HMODULE hmodule = GetModuleHandleA("ArkShopUI.dll");
		if (!hmodule)
			return false;

		T addr = (T)GetProcAddress(hmodule, "RequestUI");

		return addr(player_controller);
	}

	inline ARK_API bool Reload()
	{
		using T = bool (*) ();

		HMODULE hmodule = GetModuleHandleA("ArkShopUI.dll");
		if (!hmodule)
			return false;

		T addr = (T)GetProcAddress(hmodule, "Reload");

		return addr();
	}

	inline ARK_API bool UpdatePoints(uint64 steam_id, int points)
	{
		using T = bool (*) (uint64, int);

		HMODULE hmodule = GetModuleHandleA("ArkShopUI.dll");
		if (!hmodule)
			return false;

		T addr = (T)GetProcAddress(hmodule, "UpdatePoints");

		return addr(steam_id, points);
	}

	inline ARK_API bool PlayerKits(uint64 steam_id, FString kitdata)
	{
		using T = bool (*) (uint64, FString);

		HMODULE hmodule = GetModuleHandleA("ArkShopUI.dll");
		if (!hmodule)
			return false;

		T addr = (T)GetProcAddress(hmodule, "PlayerKits");

		return addr(steam_id, kitdata);
	}
}