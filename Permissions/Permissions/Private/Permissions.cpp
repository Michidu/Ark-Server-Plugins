#ifdef PERMISSIONS_ARK
#include "../Public/ArkPermissions.h"
#else
#include "../Public/AtlasPermissions.h"
#endif

#include "Main.h"

struct PermissionCallback
{
	PermissionCallback(FString command, bool onlyCheckOnline, bool cacheBySteamId, bool cacheByTribe, std::function<TArray<FString>(uint64*, int*)> callback)
		: command(std::move(command)),
		cacheBySteamId(std::move(cacheBySteamId)),
		cacheByTribe(std::move(cacheByTribe)),
		onlyCheckOnline(std::move(onlyCheckOnline)),
		callback(std::move(callback))
	{
	}

	FString command;
	bool cacheBySteamId, cacheByTribe, onlyCheckOnline;
	std::function<TArray<FString>(uint64*, int*)> callback;
};

namespace Permissions
{
	std::vector<std::shared_ptr<PermissionCallback>> playerPermissionCallbacks;
	void AddPlayerPermissionCallback(FString CallbackName, bool onlyCheckOnline, bool cacheBySteamId, bool cacheByTribe, const std::function<TArray<FString>(uint64*, int*)>& callback) {
		playerPermissionCallbacks.push_back(std::make_shared<PermissionCallback>(CallbackName, onlyCheckOnline, cacheBySteamId, cacheByTribe, callback));
	}
	void RemovePlayerPermissionCallback(FString CallbackName) {
		auto iter = std::find_if(playerPermissionCallbacks.begin(), playerPermissionCallbacks.end(),
			[&CallbackName](const std::shared_ptr<PermissionCallback>& data) -> bool {return data->command == CallbackName; });

		if (iter != playerPermissionCallbacks.end())
		{
			playerPermissionCallbacks.erase(std::remove(playerPermissionCallbacks.begin(), playerPermissionCallbacks.end(), *iter), playerPermissionCallbacks.end());
		}
	}
	TArray<FString> GetCallbackGroups(uint64 steamId, int tribeId, bool isOnline) {
		TArray<FString> groups;
		for (const auto& permissionCallback : playerPermissionCallbacks)
		{
			bool cache = false;
			if (permissionCallback->onlyCheckOnline) continue;
			if (permissionCallback->cacheBySteamId && database->IsPlayerExists(steamId))
			{
				CachedPermission permissions = database->HydratePlayerGroups(steamId);

				if (permissions.hasCheckedCallbacks)
				{
					for (auto group : permissions.CallbackGroups)
					{
						if (!groups.Contains(group))
							groups.Add(group);
					}
					continue;
				}
				cache = true;
			}
			if (permissionCallback->cacheByTribe && database->IsTribeExists(tribeId))
			{
				CachedPermission permissions = database->HydrateTribeGroups(tribeId);

				if (permissions.hasCheckedCallbacks)
				{
					for (auto group : permissions.CallbackGroups)
					{
						if (!groups.Contains(group))
							groups.Add(group);
					}
					continue;
				}
				cache = true;
			}
			auto callbackGroups = permissionCallback->callback(&steamId, &tribeId);
			if (cache && callbackGroups.Num() > 0)
			{
				if (permissionCallback->cacheBySteamId && database->IsPlayerExists(steamId))
				{
					database->UpdatePlayerGroupCallbacks(steamId, callbackGroups);
				}
				if (permissionCallback->cacheByTribe && database->IsTribeExists(tribeId))
				{
					database->UpdateTribeGroupCallbacks(tribeId, callbackGroups);
				}
			}
			for (auto group : callbackGroups)
			{
				if (!groups.Contains(group))
					groups.Add(group);
			}
		}
		return groups;
	}

	TArray<FString> GetPlayerGroups(uint64 steam_id)
	{
		auto world = ArkApi::GetApiUtils().GetWorld();
		TArray<FString> groups = database->GetPlayerGroups(steam_id);
		auto shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		int tribeId = -1;
		bool isOnline = false;
		if (shooter_pc) {
			auto tribeData = GetTribeData(shooter_pc);
			isOnline = true;
			if (tribeData) {
				tribeId = tribeData->TribeIDField();
				auto tribeGroups = GetTribeGroups(tribeId);
				for (auto tribeGroup : tribeGroups) {
					if (!groups.Contains(tribeGroup)) {
						groups.Add(tribeGroup);
					}
				}
				auto defaultTribeGroups = GetTribeDefaultGroups(tribeData);
				for (auto tribeGroup : defaultTribeGroups) {
					if (!groups.Contains(tribeGroup)) {
						groups.Add(tribeGroup);
					}
				}
			}
		}
		auto callbackGroups = GetCallbackGroups(steam_id, tribeId, isOnline);
		for (auto group : callbackGroups) {
			if (!groups.Contains(group))
				groups.Add(group);
		}
		return groups;
	}

	TArray<FString> GetTribeGroups(int tribeId)
	{
		return database->GetTribeGroups(tribeId);
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
		TArray<FString> groups = GetPlayerGroups(steam_id);

		for (const auto& current_group : groups)
		{
			if (current_group == group)
				return true;
		}

		return false;
	}

	bool IsTribeInGroup(int tribeId, const FString& group)
	{
		TArray<FString> groups = GetTribeGroups(tribeId);

		for (const auto& current_group : groups)
		{
			if (current_group == group)
				return true;
		}

		return false;
	}

	std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group)
	{
		return database->AddPlayerToGroup(steam_id, group);
	}

	std::optional<std::string> RemovePlayerFromGroup(uint64 steam_id, const FString& group)
	{
		return database->RemovePlayerFromGroup(steam_id, group);
	}

	std::optional<std::string> AddPlayerToTimedGroup(uint64 steam_id, const FString& group, int secs, int delaySecs)
	{
		return database->AddPlayerToTimedGroup(steam_id, group, secs, delaySecs);
	}

	std::optional<std::string> RemovePlayerFromTimedGroup(uint64 steam_id, const FString& group)
	{
		return database->RemovePlayerFromTimedGroup(steam_id, group);
	}

	std::optional<std::string> AddTribeToGroup(int tribeId, const FString& group)
	{
		return database->AddTribeToGroup(tribeId, group);
	}

	std::optional<std::string> RemoveTribeFromGroup(int tribeId, const FString& group)
	{
		return database->RemoveTribeFromGroup(tribeId, group);
	}

	std::optional<std::string> AddTribeToTimedGroup(int tribeId, const FString& group, int secs, int delaySecs)
	{
		return database->AddTribeToTimedGroup(tribeId, group, secs, delaySecs);
	}

	std::optional<std::string> RemoveTribeFromTimedGroup(int tribeId, const FString& group)
	{
		return database->RemoveTribeFromTimedGroup(tribeId, group);
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

	bool IsPlayerHasPermission(uint64 steam_id, const FString& permission)
	{
		TArray<FString> groups = GetPlayerGroups(steam_id);

		for (const auto& current_group : groups)
		{
			if (IsGroupHasPermission(current_group, permission) || IsGroupHasPermission(current_group, "*"))
				return true;
		}

		return false;
	}

	bool IsTribeHasPermission(int tribeId, const FString& permission)
	{
		TArray<FString> groups = GetTribeGroups(tribeId);

		for (const auto& current_group : groups)
		{
			if (IsGroupHasPermission(current_group, permission) || IsGroupHasPermission(current_group, "*"))
				return true;
		}

		return false;
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