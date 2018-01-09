#include "TimedRewards.h"
#include "Points.h"
#include <chrono>

namespace TimedRewards
{
	std::vector<OnlinePlayersData*> OnlinePlayers;

	namespace
	{
		// Config variables
		int pointsInterval = 0;
		int pointsAmount = 0;

		void RewardTimer()
		{
			auto now = std::chrono::system_clock::now();

			for (OnlinePlayersData* data : OnlinePlayers)
			{
				auto nextTime = data->NextRewardTime;
				auto diff = std::chrono::duration_cast<std::chrono::seconds>(nextTime - now);

				if (diff.count() <= 0)
				{
					Points::AddPoints(pointsAmount, data->SteamId, true);

					data->NextRewardTime = now + std::chrono::minutes(pointsInterval);
				}
			}
		}
	}

	void Init()
	{
		pointsInterval = json["General"]["TimedPointsReward"]["Interval"];
		pointsAmount = json["General"]["TimedPointsReward"]["Amount"];

		Ark::AddOnTimerCallback(&RewardTimer);
	}

	int GetInterval()
	{
		return pointsInterval;
	}
}
