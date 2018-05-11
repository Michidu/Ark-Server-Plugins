#include "../Public/Permissions.h"

#include "Main.h"

namespace Permissions
{
	TArray<FString> GetPlayerGroups(uint64 steam_id)
	{
		return database->GetPlayerGroups(steam_id);
	}

	TArray<FString> GetGroupPermissions(const FString& group)
	{
		if (group.IsEmpty())
			return {};
		return database->GetGroupPermissions(group);
	}
	
	TArray<FString> GetAllGroups()
	{
		return database->GetAllGroups();
	}

	TArray<uint64> GetGroupMembers(const FString& group)
	{
		return database->GetGroupMembers(group);
	}

	bool IsPlayerInGroup(uint64 steam_id, const FString& group)
	{
		return database->IsPlayerInGroup(steam_id, group);
	}

	std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group)
	{
		return database->AddPlayerToGroup(steam_id, group);
	}

	std::optional<std::string> RemovePlayerFromGroup(uint64 steam_id, const FString& group)
	{
		return database->RemovePlayerFromGroup(steam_id, group);
	}

	std::optional<std::string> AddGroup(const FString& group)
	{
		return database->AddGroup(group);
	}

	std::optional<std::string> RemoveGroup(const FString& group)
	{
		return database->RemoveGroup(group);
	}

	bool IsGroupHasPermission(const FString& group, const FString& permission)
	{
		return database->IsGroupHasPermission(group, permission);
	}

	bool IsPlayerHasPermission(uint64 steam_id, const FString& permission)
	{
		return database->IsPlayerHasPermission(steam_id, permission);
	}

	std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission)
	{
		return database->GroupGrantPermission(group, permission);
	}

	std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission)
	{
		return database->GroupRevokePermission(group, permission);
	}
}
