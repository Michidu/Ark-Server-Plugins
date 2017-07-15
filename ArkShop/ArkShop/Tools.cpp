#include <windows.h>
#include <fstream>
#include <chrono>
#include <random>
#include "Tools.h"

namespace Tools
{
	__int64 GetSteamId(AShooterPlayerController* playerController)
	{
		__int64 steamId = 0;

		APlayerState* playerState = playerController->GetPlayerStateField();
		if (playerState)
		{
			steamId = playerState->GetUniqueIdField()->UniqueNetId->GetUniqueNetIdField();
		}

		return steamId;
	}

	AShooterPlayerController* FindPlayerFromName(const std::string& steamName)
	{
		AShooterPlayerController* result = nullptr;

		auto playerControllers = Ark::GetWorld()->GetPlayerControllerListField();
		for (uint32_t i = 0; i < playerControllers.Num(); i++)
		{
			auto playerController = playerControllers[i];

			std::string currentName = playerController->GetPlayerStateField()->GetPlayerNameField().ToString();

			if (currentName == steamName)
			{
				AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(playerController.Get());

				result = aShooterPC;
				break;
			}
		}

		return result;
	}

	AShooterPlayerController* FindPlayerFromSteamId(unsigned __int64 steamId)
	{
		AShooterPlayerController* result = nullptr;

		auto playerControllers = Ark::GetWorld()->GetPlayerControllerListField();
		for (uint32_t i = 0; i < playerControllers.Num(); i++)
		{
			auto playerController = playerControllers[i];

			APlayerState* playerState = playerController->GetPlayerStateField();
			__int64 currentSteamId = playerState->GetUniqueIdField()->UniqueNetId->GetUniqueNetIdField();

			if (currentSteamId == steamId)
			{
				AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(playerController.Get());

				result = aShooterPC;
				break;
			}
		}

		return result;
	}

	int GetRandomNumber(int min, int max)
	{
		std::default_random_engine generator(std::random_device{}());
		std::uniform_int_distribution<int> distribution(min, max);

		int rnd = distribution(generator);

		return rnd;
	}

	void Log(const std::string& text)
	{
		static std::ofstream file(GetCurrentDir() + "/BeyondApi/Plugins/ArkShop/logs.txt", std::ios_base::app);

		auto time = std::chrono::system_clock::now();
		std::time_t tTime = std::chrono::system_clock::to_time_t(time);

		char buffer[256];
		ctime_s(buffer, sizeof(buffer), &tTime);

		buffer[strlen(buffer) - 1] = '\0';

		std::string timeStr = buffer;

		std::string finalText = timeStr + ": " + text + "\n";

		file << finalText;

		file.flush();
	}

	std::string GetCurrentDir()
	{
		char buffer[MAX_PATH];
		GetModuleFileNameA(nullptr, buffer, MAX_PATH);
		std::string::size_type pos = std::string(buffer).find_last_of("\\/");

		return std::string(buffer).substr(0, pos);
	}

	std::wstring ConvertToWideStr(const std::string& text)
	{
		const size_t size = text.size();

		std::wstring wstr;
		if (size > 0) 
		{
			wstr.resize(size);

			size_t convertedChars;
			mbstowcs_s(&convertedChars, &wstr[0], size + 1, text.c_str(), _TRUNCATE);
		}

		return wstr;
	}
}
