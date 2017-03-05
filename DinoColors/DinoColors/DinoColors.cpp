#include <iostream>
#include <fstream>
#include <random>
#include "API/Base.h"
#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(APrimalDinoCharacter_BeginPlay, void, APrimalDinoCharacter*);

nlohmann::json json;

void LoadConfig();
void RandomizeDinoColor(APrimalDinoCharacter* dino);
int GetRandomColor();
int GetRandomNumber(int min, int max);

void Init()
{
	LoadConfig();

	Ark::SetHook("APrimalDinoCharacter", "BeginPlay", &Hook_APrimalDinoCharacter_BeginPlay, reinterpret_cast<LPVOID*>(&APrimalDinoCharacter_BeginPlay_original));
}

void LoadConfig()
{
	std::ifstream file("BeyondApi/Plugins/DinoColors/colors.json");
	if (!file.is_open())
	{
		std::cout << "Could not open file colors.json" << std::endl;
		throw;
	}

	file >> json;
	file.close();
}

void _cdecl Hook_APrimalDinoCharacter_BeginPlay(APrimalDinoCharacter* _this)
{
	APrimalDinoCharacter_BeginPlay_original(_this);

	if (_this->GetTargetingTeamField() < 50000) // Don't change color of tamed dinos
	{
		RandomizeDinoColor(_this);
	}
}

void RandomizeDinoColor(APrimalDinoCharacter* dino)
{
	for (int i = 0; i <= 5; ++i)
	{
		int color = GetRandomColor();

		dino->ForceUpdateColorSets_Implementation(i, color);
	}
}

int GetRandomColor()
{
	auto allColors = json["Colors"];

	size_t size = allColors.size() - 1;
	int rnd = GetRandomNumber(0, static_cast<int>(size));

	return allColors[rnd];
}

int GetRandomNumber(int min, int max)
{
	std::default_random_engine generator(std::random_device{}());
	std::uniform_int_distribution<int> distribution(min, max);

	int rnd = distribution(generator);

	return rnd;
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
