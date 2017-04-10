#pragma once

#include "ArkShop.h"
#include "API/Base.h"

namespace Points
{
	void Init();
	bool AddPoints(int amount, __int64 steamId);
	bool SpendPoints(int amount, __int64 steamId);
	int GetPoints(__int64 steamId);
	bool SetPoints(__int64 steamId, int newAmount);
}