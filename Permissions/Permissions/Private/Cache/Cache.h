#pragma once

namespace Permissions::Cache
{
	struct permissions_cache_player_permissions
	{
		size_t permission_hash;
		bool active;

		permissions_cache_player_permissions(size_t _permission_hash, bool _active)
		{
			permission_hash = _permission_hash;
			active = _active;
		}
	};

	struct permission_cache_player_groups
	{
		size_t group_hash;
		bool active;

		permission_cache_player_groups(size_t _group_hash, bool _active)
		{
			group_hash = _group_hash;
			active = _active;
		}
	};

	struct permission_cache_player
	{
		std::vector<permission_cache_player_groups> groups;
		std::vector<permissions_cache_player_permissions> permissions;

		permission_cache_player(permission_cache_player_groups group)
		{
			groups.push_back(group);
		}

		permission_cache_player(permissions_cache_player_permissions permission)
		{
			permissions.push_back(permission);
		}
	};

	inline std::hash<std::string> hasher;
	inline std::unordered_map<uint64, std::unique_ptr<permission_cache_player>> player_cache;

	inline void AddPlayerToGroup(const uint64& steam_id, const FString& group, const bool active)
	{
		const size_t group_hash = hasher(group.ToString()); 
		
		auto cache_info = std::find_if(player_cache.begin(), player_cache.end(), [&steam_id](const auto& perm_info) { return perm_info.first == steam_id; });
		if (cache_info != player_cache.end())
			cache_info->second->groups.push_back(permission_cache_player_groups(group_hash, active));
		else
			player_cache[steam_id] = std::make_unique<permission_cache_player>(permission_cache_player(permission_cache_player_groups(group_hash, active)));
	}

	inline void AddPlayerToPermission(const uint64& steam_id, const FString& permission, const bool active)
	{
		const size_t permission_hash = hasher(permission.ToString());

		auto cache_info = std::find_if(player_cache.begin(), player_cache.end(), [&steam_id](const auto& perm_info) { return perm_info.first == steam_id; });
		if (cache_info != player_cache.end())
			cache_info->second->permissions.push_back(permissions_cache_player_permissions(permission_hash, active));
		else
			player_cache[steam_id] = std::make_unique<permission_cache_player>(permission_cache_player(permissions_cache_player_permissions(permission_hash, active)));
	}

	inline void RemovePlayer(const uint64& steam_id)
	{
		player_cache.erase(steam_id);
	}

	inline void ClearAll()
	{
		player_cache.clear();
	}

	inline const int IsPlayerInGroup(const uint64& steam_id, const FString& group)
	{
		const auto& cache_info = std::find_if(player_cache.begin(), player_cache.end(), [&steam_id](const auto& perm_info) { return perm_info.first == steam_id; });
		if (cache_info == player_cache.end())
			return -1;

		const size_t group_hash = hasher(group.ToString());

		const auto& cache_group_perm = std::find_if(cache_info->second->groups.begin(), cache_info->second->groups.end(), [&group_hash](const auto& group_info) { return group_info.group_hash == group_hash; });
		if (cache_group_perm != cache_info->second->groups.end())
			return cache_group_perm->active ? 1 : 0;

		return -1;
	}

	inline const int IsPlayerHasPermission(const uint64& steam_id, const FString& permission)
	{
		const auto& cache_info = std::find_if(player_cache.begin(), player_cache.end(), [&steam_id](const auto& perm_info) { return perm_info.first == steam_id; });
		if (cache_info == player_cache.end())
			return -1;

		const size_t permission_hash = hasher(permission.ToString());

		const auto& cache_perm = std::find_if(cache_info->second->permissions.begin(), cache_info->second->permissions.end(), [&permission_hash](const auto& perm_info) { return perm_info.permission_hash == permission_hash; });
		if (cache_perm != cache_info->second->permissions.end())
			return cache_perm->active ? 1 : 0;

		return -1;
	}
}