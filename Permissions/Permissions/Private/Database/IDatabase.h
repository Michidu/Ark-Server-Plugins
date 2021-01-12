#pragma once

#ifdef PERMISSIONS_ARK
#include "../Public/ArkPermissions.h"
#else
#include "../Public/AtlasPermissions.h"
#endif

class IDatabase
{
public:
	std::unordered_map<std::string, std::string> permissionGroups;
	std::unordered_map<uint64, std::string> permissionPlayers;
	std::mutex playersMutex, groupsMutex;

	virtual ~IDatabase() = default;

	virtual bool AddPlayer(uint64 steam_id) = 0;
	virtual bool IsPlayerExists(uint64 steam_id) = 0;
	virtual bool IsGroupExists(const FString& group) = 0;
	virtual TArray<FString> GetPlayerGroups(uint64 steam_id) = 0;
	virtual TArray<FString> GetGroupPermissions(const FString& group) = 0;
	virtual TArray<FString> GetAllGroups() = 0;
	virtual TArray<uint64> GetGroupMembers(const FString& group) = 0;
	virtual std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group) = 0;
	virtual std::optional<std::string> RemovePlayerFromGroup(uint64 steam_id, const FString& group) = 0;
	virtual std::optional<std::string> AddGroup(const FString& group) = 0;
	virtual std::optional<std::string> RemoveGroup(const FString& group) = 0;
	virtual std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission) = 0;
	virtual std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission) = 0;

	virtual void Init() = 0;
	virtual std::unordered_map<std::string, std::string> InitGroups() = 0;
	virtual std::unordered_map<uint64, std::string> InitPlayers() = 0;
};
