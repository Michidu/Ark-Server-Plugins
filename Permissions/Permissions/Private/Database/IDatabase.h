#pragma once

#include "../CachedPermission.h"
#ifdef PERMISSIONS_ARK
#include "../Public/ArkPermissions.h"
#else
#include "../Public/AtlasPermissions.h"
#endif

class IDatabase
{
protected:
	std::unordered_map<std::string, std::string> permissionGroups;
	std::unordered_map<uint64, CachedPermission> permissionPlayers;
	std::unordered_map<int, CachedPermission> permissionTribes;
	std::mutex playersMutex, groupsMutex, tribesMutex;

public:
	virtual ~IDatabase() = default;

	virtual bool IsFieldExists(std::string tableName, std::string fieldName) = 0;

	virtual bool IsPlayerExists(uint64 steam_id) = 0;
	virtual bool AddPlayer(uint64 steam_id) = 0;

	virtual bool IsGroupExists(const FString& group) = 0;
	virtual TArray<FString> GetPlayerGroups(uint64 steam_id, bool includeTimed = true) = 0;
	virtual CachedPermission HydratePlayerGroups(uint64 steam_id) = 0;
	virtual TArray<FString> GetGroupPermissions(const FString& group) = 0;
	virtual TArray<FString> GetAllGroups() = 0;
	virtual TArray<uint64> GetGroupMembers(const FString& group) = 0;
	virtual std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group) = 0;
	virtual std::optional<std::string> RemovePlayerFromGroup(uint64 steam_id, const FString& group) = 0;
	virtual std::optional<std::string> AddGroup(const FString& group) = 0;
	virtual std::optional<std::string> RemoveGroup(const FString& group) = 0;
	virtual std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission) = 0;
	virtual std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission) = 0;
	virtual std::optional<std::string> AddPlayerToTimedGroup(uint64 steam_id, const FString& group, int secs, int delaySecs) = 0;
	virtual std::optional<std::string> RemovePlayerFromTimedGroup(uint64 steam_id, const FString& group) = 0;
	virtual void UpdatePlayerGroupCallbacks(uint64 steamId, TArray<FString> groups) = 0;

	virtual bool IsTribeExists(int tribeId) = 0;
	virtual bool AddTribe(int tribeId) = 0;
	virtual TArray<FString> GetTribeGroups(int tribeId, bool includeTimed = true) = 0;
	virtual CachedPermission HydrateTribeGroups(int tribeId) = 0;
	virtual std::optional<std::string> AddTribeToGroup(int tribeId, const FString& group) = 0;
	virtual std::optional<std::string> RemoveTribeFromGroup(int tribeId, const FString& group) = 0;
	virtual std::optional<std::string> AddTribeToTimedGroup(int tribeId, const FString& group, int secs, int delaySecs) = 0;
	virtual std::optional<std::string> RemoveTribeFromTimedGroup(int tribeId, const FString& group) = 0;
	virtual void UpdateTribeGroupCallbacks(int tribeId, TArray<FString> groups) = 0;

	virtual void Init() = 0;
	virtual std::unordered_map<std::string, std::string> InitGroups() = 0;
	virtual std::unordered_map<uint64, CachedPermission> InitPlayers() = 0;
	virtual std::unordered_map<int, CachedPermission> InitTribes() = 0;
};
