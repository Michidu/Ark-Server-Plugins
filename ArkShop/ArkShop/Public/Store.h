#pragma once

#include "Base.h"

namespace ArkShop::Store
{
	void Init();

	/**
	 * \brief Buys an item from shop for specific player
	 * \param player_controller Player
	 * \param item_id Shop's item id
	 * \param amount Amount of items to buy (only for items)
	 * \return True if success, false otherwise
	 */
	SHOP_API bool Buy(AShooterPlayerController* player_controller, const FString& item_id, int amount);
} // namespace Store // namespace ArkShop
