#pragma once

#include "Permissions.h"

namespace Permissions::DB
{
	/**
	 * \brief Checks if player exists in database
	 */
	ARK_API bool IsPlayerExists(uint64 steam_id);

	/**
	* \brief Checks if group exists in database
	*/
	ARK_API bool IsGroupExists(const FString& group);
}
