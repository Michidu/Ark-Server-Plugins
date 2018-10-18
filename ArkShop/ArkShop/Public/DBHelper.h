#pragma once

#include "Base.h"

namespace ArkShop::DBHelper
{
	/**
	 * \brief Checks if player exists in shop database
	 * \param steam_id Players steam id
	 * \return True if exists, false otherwise
	 */
	SHOP_API bool IsPlayerExists(uint64 steam_id);
} // namespace DBHelper // namespace ArkShop
