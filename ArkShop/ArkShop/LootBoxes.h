#pragma once

#include "Base.h"

namespace ArkShop::LootBoxes
{
	void Init();
	void Unload();

	
	SHOP_API bool CanUseCajas(uint64 steam_id, FString lootboxname);
	SHOP_API bool ChangeCajasAmount(const FString& kit_name, int amount, uint64 steam_id);

	/**
	* \brief Checks if kit exists in server config
	*/
} // namespace Kits // namespace ArkShop
