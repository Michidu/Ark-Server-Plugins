#pragma once

#include <chrono>
#include "ITimedRewards.h"

#include <utility>

#include "ArkShop.h"

namespace ArkShop
{
	class TimedRewards : public ITimedRewards
	{
	public:
		static TimedRewards& Get();

		TimedRewards(const TimedRewards&) = delete;
		TimedRewards(TimedRewards&&) = delete;
		TimedRewards& operator=(const TimedRewards&) = delete;
		TimedRewards& operator=(TimedRewards&&) = delete;

		void AddTask(const FString& id, uint64 steam_id, const std::function<void()>& reward_callback,
		             int interval) override;
		void RemovePlayer(uint64 steam_id);

	private:
		struct RewardData
		{
			FString id;
			std::function<void()> reward_callback;
			std::chrono::time_point<std::chrono::system_clock> next_reward_time;
			int interval;
		};

		struct OnlinePlayersData
		{
			OnlinePlayersData(uint64 steam_id, const FString& id, std::function<void()> reward_callback,
			                  const std::chrono::time_point<std::chrono::system_clock>& next_reward_time, int interval)
				: steam_id(steam_id)
			{
				reward_callbacks.Add({id, std::move(reward_callback), next_reward_time, interval});
			}

			uint64 steam_id;
			TArray<RewardData> reward_callbacks;
		};

		TimedRewards();
		~TimedRewards() = default;

		void RewardTimer();

		std::vector<std::shared_ptr<OnlinePlayersData>> online_players_;
	};
} // namespace ArkShop
