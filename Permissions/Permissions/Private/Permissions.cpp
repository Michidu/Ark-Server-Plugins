#ifdef PERMISSIONS_ARK
#include "../Public/ArkPermissions.h"
#else
#include "../Public/AtlasPermissions.h"
#endif

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

	bool _IsPlayerInGroup(uint64 steam_id, const FString& group)
	{
		TArray<FString> groups = GetPlayerGroups(steam_id);

		for (const auto& current_group : groups)
		{
			if (current_group == group)
				return true;
		}

		return false;
	}

	bool IsPlayerInGroup(uint64 steam_id, const FString& group)
	{
		if (use_cache)
		{
			const int cache_player_in_group = Cache::IsPlayerInGroup(steam_id, group);

			if (cache_player_in_group == -1)
				Cache::AddPlayerToGroup(steam_id, group, _IsPlayerInGroup(steam_id, group));

			return cache_player_in_group == 1;
		}

		return _IsPlayerInGroup(steam_id, group);
	}

	std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group)
	{
		if(use_cache)
			Cache::RemovePlayer(steam_id);

		return database->AddPlayerToGroup(steam_id, group);
	}

	std::optional<std::string> RemovePlayerFromGroup(uint64 steam_id, const FString& group)
	{
		if (use_cache)
			Cache::RemovePlayer(steam_id);

		return database->RemovePlayerFromGroup(steam_id, group);
	}

	std::optional<std::string> AddGroup(const FString& group)
	{
		if (use_cache)
			Cache::ClearAll();

		return database->AddGroup(group);
	}

	std::optional<std::string> RemoveGroup(const FString& group)
	{
		if (use_cache)
			Cache::ClearAll();

		return database->RemoveGroup(group);
	}

	bool IsGroupHasPermission(const FString& group, const FString& permission)
	{
		if (!database->IsGroupExists(group))
			return false;

		TArray<FString> permissions = GetGroupPermissions(group);

		for (const auto& current_perm : permissions)
		{
			if (current_perm == permission)
				return true;
		}

		return false;
	}

	bool _IsPlayerHasPermission(uint64 steam_id, const FString& permission)
	{
		TArray<FString> groups = GetPlayerGroups(steam_id);

		for (const auto& current_group : groups)
		{
			if (IsGroupHasPermission(current_group, permission) || IsGroupHasPermission(current_group, "*"))
				return true;
		}

		return false;
	}

	bool IsPlayerHasPermission(uint64 steam_id, const FString& permission)
	{
		if (use_cache)
		{
			const int cache_player_in_group = Cache::IsPlayerHasPermission(steam_id, permission);

			if (cache_player_in_group == -1)
				Cache::AddPlayerToPermission(steam_id, permission, _IsPlayerHasPermission(steam_id, permission));

			return cache_player_in_group == 1;
		}

		return _IsPlayerHasPermission(steam_id, permission);
	}

	std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission)
	{
		if (use_cache)
			Cache::ClearAll();

		return database->GroupGrantPermission(group, permission);
	}

	std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission)
	{
		if (use_cache)
			Cache::ClearAll();

		return database->GroupRevokePermission(group, permission);
	}
}
