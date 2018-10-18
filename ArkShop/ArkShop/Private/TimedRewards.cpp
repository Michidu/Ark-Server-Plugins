#include "TimedRewards.h"

#include <Permissions.h>
#include <Points.h>

namespace ArkShop
{
	TimedRewards::TimedRewards()
	{
		points_interval_ = config["General"]["TimedPointsReward"]["Interval"];

		ArkApi::GetCommands().AddOnTimerCallback("RewardTimer", std::bind(&TimedRewards::RewardTimer, this));
	}

	TimedRewards& TimedRewards::Get()
	{
		static TimedRewards instance;
		return instance;
	}

	void TimedRewards::AddPlayer(uint64 steam_id)
	{
		const auto iter = std::find_if(
			online_players_.begin(), online_players_.end(),
			[steam_id](const std::shared_ptr<OnlinePlayersData>& data) -> bool { return data->steam_id == steam_id; });

		if (iter != online_players_.end())
		{
			return;
		}

		const int interval = points_interval_;

		const auto now = std::chrono::system_clock::now();
		const auto next_time = now + std::chrono::minutes(interval);

		auto groups_map = config["General"]["TimedPointsReward"]["Groups"];

		int points_amount = groups_map["Default"].value("Amount", 0);

		for (auto group_iter = groups_map.begin(); group_iter != groups_map.end(); ++group_iter)
		{
			const FString group_name(group_iter.key().c_str());
			if (group_name == L"Default")
			{
				continue;
			}

			if (Permissions::IsPlayerInGroup(steam_id, group_name))
			{
				points_amount = group_iter.value().value("Amount", 0);
				break;
			}
		}

		if (points_amount == 0)
		{
			return;
		}

		online_players_.push_back(std::make_shared<OnlinePlayersData>(steam_id, points_amount, next_time));
	}

	void TimedRewards::RemovePlayer(uint64 steam_id)
	{
		const auto iter = std::find_if(
			online_players_.begin(), online_players_.end(),
			[steam_id](const std::shared_ptr<OnlinePlayersData>& data) -> bool { return data->steam_id == steam_id; });

		if (iter != online_players_.end())
		{
			online_players_.erase(std::remove(online_players_.begin(), online_players_.end(), *iter),
			                      online_players_.end());
		}
	}

	void TimedRewards::RewardTimer()
	{
		const auto now = std::chrono::system_clock::now();

		for (const auto& data : online_players_)
		{
			const auto next_time = data->next_reward_time;
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(next_time - now);

			if (diff.count() <= 0)
			{
				data->next_reward_time = now + std::chrono::minutes(points_interval_);

				Points::AddPoints(data->points_amount, data->steam_id);
			}
		}
	}
} // namespace ArkShop
