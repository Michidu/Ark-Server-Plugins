#pragma once

#include "Base.h"

namespace ArkShop::Kits
{
	void Init();
	void Unload();

	/**
	* \brief Adds or reduces kits of the specific player
	*/
	SHOP_API bool ChangeKitAmount(const FString& kit_name, int amount, uint64 steam_id);

	/**
	 * \brief Checks if player has permissions to use this kit
	 */
	SHOP_API bool CanUseKit(AShooterPlayerController* player_controller, uint64 steam_id, const FString& kit_name);

	/**
	* \brief Checks if kit exists in server config
	*/
	SHOP_API bool IsKitExists(const FString& kit_name);
} // namespace Kits // namespace ArkShop
