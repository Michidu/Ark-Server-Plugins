#include "../Public/DBHelper.h"

#include "Main.h"

namespace Permissions::DB
{
	bool IsPlayerExists(uint64 steam_id)
	{
		return GetDB()->IsPlayerExists(steam_id);
	}

	bool IsGroupExists(const FString& group)
	{
		return GetDB()->IsGroupExists(group);
	}
}
