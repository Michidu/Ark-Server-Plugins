#pragma once

#include "ArkShop.h"
#include "API/Base.h"

namespace Points
{
	void Init();
	bool AddPoints(int amount, __int64 steamId, bool isTimedReward = false);
	bool SpendPoints(int amount, __int64 steamId);
	int GetPoints(__int64 steamId);
	std::tuple<int, std::unique_ptr<int>> GetPointsAndTimedRewardAmountOverride(__int64 steamId);
	bool SetPoints(__int64 steamId, int newAmount);
	bool SetTimedRewardOverride(__int64 steamId, std::unique_ptr<int> newValue);
}