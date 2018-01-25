#pragma once

#include <API/ARK/Ark.h>

#ifdef ARK_EXPORTS
#define ARK_API __declspec(dllexport)
#else
#define ARK_API __declspec(dllimport)
#endif

namespace Permissions
{
	ARK_API FString GetPlayerGroups(uint64 steam_id);
	ARK_API FString GetGroupPermissions(const FString& group);

	ARK_API bool IsPlayerInGroup(uint64 steam_id, const FString& group);

	ARK_API bool AddPlayerToGroup(uint64 steam_id, const FString& group);
	ARK_API bool RemovePlayerFromGroup(uint64 steam_id, const FString& group);

	ARK_API bool AddGroup(const FString& group);
	ARK_API bool RemoveGroup(const FString& group);

	ARK_API bool IsGroupHasPermission(const FString& group, const FString& permission);
	ARK_API bool IsPlayerHasPermission(uint64 steam_id, const FString& permission);

	ARK_API bool GroupGrantPermission(const FString& group, const FString& permission);
	ARK_API bool GroupRevokePermission(const FString& group, const FString& permission);
}
