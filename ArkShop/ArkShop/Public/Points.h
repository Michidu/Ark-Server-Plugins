#pragma once

#include "Base.h"

namespace ArkShop::Points
{
	void Init();
	void Unload();

	/**
	* \brief Add points to the specific player
	* \param amount Amount of points to add
	* \param steam_id Players steam id
	* \return True if success, false otherwise
	*/
	SHOP_API bool AddPoints(int amount, uint64 steam_id);

	/**
	* \brief Subtracts points from the specific player
	* \param amount Amount of points
	* \param steam_id Players steam id
	* \return True if success, false otherwise
	*/
	SHOP_API bool SpendPoints(int amount, uint64 steam_id);

	/**
	* \brief Receives points from the specific player
	* \param steam_id Players steam id
	* \return Amount of points the player has
	*/
	SHOP_API int GetPoints(uint64 steam_id);

	int GetTotalSpent(uint64 steam_id);

	/**
	* \brief Change points amount for the specific player
	* \param steam_id Players steam id
	* \param new_amount New amount of points
	* \return True if success, false otherwise
	*/
	SHOP_API bool SetPoints(uint64 steam_id, int new_amount);
} // namespace Points // namespace ArkShop
