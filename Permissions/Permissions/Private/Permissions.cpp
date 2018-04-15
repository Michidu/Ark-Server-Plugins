#include "../Public/Permissions.h"

#include "../Public/DBHelper.h"
#include "Main.h"

namespace Permissions
{
	TArray<FString> GetPlayerGroups(uint64 steam_id)
	{		
		return GetDB()->GetPlayerGroups(steam_id);
	}

	TArray<FString> GetGroupPermissions(const FString& group)
	{
		if (group.IsEmpty()) return {};
		return GetDB()->GetGroupPermissions(group);
	}

	TArray<uint64> GetGroupMembers(const FString& group)
	{				
		return GetDB()->GetGroupMembers(group);
	}

	bool IsPlayerInGroup(uint64 steam_id, const FString& group)
	{
		return GetDB()->IsPlayerInGroup(steam_id, group);
	}

	std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group)
	{
		return GetDB()->AddPlayerToGroup(steam_id, group);
	}

	std::optional<std::string> RemovePlayerFromGroup(uint64 steam_id, const FString& group)
	{
		return GetDB()->RemovePlayerFromGroup(steam_id, group);
	}

	std::optional<std::string> AddGroup(const FString& group)
	{
		return GetDB()->AddGroup(group);
	}

	std::optional<std::string> RemoveGroup(const FString& group)
	{
		return GetDB()->RemoveGroup(group);
	}

	bool IsGroupHasPermission(const FString& group, const FString& permission)
	{
		return GetDB()->IsGroupHasPermission(group, permission);
	}

	bool IsPlayerHasPermission(uint64 steam_id, const FString& permission)
	{
		return GetDB()->IsPlayerHasPermission(steam_id, permission);
	}

	std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission)
	{
		return GetDB()->GroupGrantPermission(group, permission);
	}

	std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission)
	{
		return GetDB()->GroupRevokePermission(group, permission);
	}
}