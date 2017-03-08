#pragma once

#include "ArkShop.h"
#include <chrono>

namespace TimedRewards
{
	struct OnlinePlayersData
	{
		OnlinePlayersData(__int64 steamId, const std::chrono::time_point<std::chrono::system_clock>& nextRewardTime)
			: SteamId(steamId),
			  NextRewardTime(nextRewardTime)
		{
		}

		__int64 SteamId;
		std::chrono::time_point<std::chrono::system_clock> NextRewardTime;
	};

	void Init();
	int GetInterval();

	extern std::vector<OnlinePlayersData*> OnlinePlayers;
}
