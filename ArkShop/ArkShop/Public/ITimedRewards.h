#pragma once

#include "Base.h"

#include <functional>

namespace ArkShop
{
	class SHOP_API ITimedRewards
	{
	public:
		virtual ~ITimedRewards() = default;

		virtual void AddTask(const FString& id, uint64 steam_id, const std::function<void()>& reward_callback, int interval) = 0;
	};

	SHOP_API ITimedRewards& APIENTRY GetTimedRewards();
} // namespace ArkApi
