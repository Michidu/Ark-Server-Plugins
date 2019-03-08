#include <DBHelper.h>

#include "AtlasShop.h"

namespace AtlasShop::DBHelper
{
	bool IsPlayerExists(uint64 steam_id)
	{
		return database->IsPlayerExists(steam_id);
	}
} // namespace DBHelper // namespace AtlasShop
