#pragma once

#include <API/ARK/Ark.h>

#ifdef ARK_EXPORTS
#define ARK_API __declspec(dllexport)
#else
#define ARK_API __declspec(dllimport)
#endif

namespace Permissions
{
	ARK_API TArray<FString> GetPlayerGroups(uint64 steam_id);
	ARK_API TArray<FString> GetGroupPermissions(const FString& group);
	ARK_API TArray<uint64> GetGroupMembers(const FString& group);

	ARK_API bool IsPlayerInGroup(uint64 steam_id, const FString& group);

	ARK_API std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group);
	ARK_API std::optional<std::string> RemovePlayerFromGroup(uint64 steam_id, const FString& group);

	ARK_API std::optional<std::string> AddGroup(const FString& group);
	ARK_API std::optional<std::string> RemoveGroup(const FString& group);

	ARK_API bool IsGroupHasPermission(const FString& group, const FString& permission);
	ARK_API bool IsPlayerHasPermission(uint64 steam_id, const FString& permission);

	ARK_API std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission);
	ARK_API std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission);
}
