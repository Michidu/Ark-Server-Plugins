#include <DBHelper.h>

#include "ArkShop.h"

namespace ArkShop::DBHelper
{
	bool IsPlayerExists(uint64 steam_id)
	{
		return database->IsPlayerExists(steam_id);
	}
} // namespace DBHelper // namespace ArkShop
