#pragma once

#include <chrono>

#include "ArkShop.h"
#include "Base.h"

namespace ArkShop
{
	/**
	 * \brief Added players will be receiving points every n minutes
	 */
	class TimedRewards
	{
	public:
		static TimedRewards& Get();

		TimedRewards(const TimedRewards&) = delete;
		TimedRewards(TimedRewards&&) = delete;
		TimedRewards& operator=(const TimedRewards&) = delete;
		TimedRewards& operator=(TimedRewards&&) = delete;

		void AddPlayer(uint64 steam_id);
		void RemovePlayer(uint64 steam_id);

	private:
		struct OnlinePlayersData
		{
			OnlinePlayersData(uint64 steam_id, int points_amount,
			                  const std::chrono::time_point<std::chrono::system_clock>& next_reward_time)
				: steam_id(steam_id),
				  points_amount(points_amount),
				  next_reward_time(next_reward_time)
			{
			}

			uint64 steam_id;
			int points_amount;
			std::chrono::time_point<std::chrono::system_clock> next_reward_time;
		};

		TimedRewards();
		~TimedRewards() = default;

		void RewardTimer();

		int points_interval_;
		std::vector<std::shared_ptr<OnlinePlayersData>> online_players_;
	};
} // namespace ArkShop
