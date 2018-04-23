#include "../Public/DBHelper.h"

#include "Main.h"

namespace Permissions::DB
{
	bool IsPlayerExists(uint64 steam_id)
	{
		return database->IsPlayerExists(steam_id);
	}

	bool IsGroupExists(const FString& group)
	{
		return database->IsGroupExists(group);
	}
}
