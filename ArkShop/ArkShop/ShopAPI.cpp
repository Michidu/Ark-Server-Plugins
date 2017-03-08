#include "ShopAPI.h"
#include "Points.h"

namespace ShopAPI
{
	bool AddPoints(int amount, __int64 steamId)
	{
		return Points::AddPoints(amount, steamId);
	}

	bool SpendPoints(int amount, __int64 steamId)
	{
		return Points::SpendPoints(amount, steamId);
	}

	int GetPoints(__int64 steamId)
	{
		return Points::GetPoints(steamId);
	}
}
