#pragma once

#include <API/ARK/Ark.h>

class IDatabase
{
public:
	virtual ~IDatabase() = default;

	virtual void AddPlayer(uint64 steam_id) = 0;
	virtual bool IsPlayerExists(uint64 steam_id) = 0;
	virtual bool IsGroupExists(const FString& group) = 0;
	virtual bool IsPlayerInGroup(uint64 steam_id, const FString& group) = 0;
	virtual TArray<FString> GetPlayerGroups(uint64 steam_id) = 0;
	virtual TArray<FString> GetGroupPermissions(const FString& group) = 0;
	virtual TArray<FString> GetAllGroups() = 0;
	virtual TArray<uint64> GetGroupMembers(const FString& group) = 0;
	virtual std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group) = 0;
	virtual std::optional<std::string> RemovePlayerFromGroup(uint64 steam_id, const FString& group) = 0;
	virtual std::optional<std::string> AddGroup(const FString& group) = 0;
	virtual std::optional<std::string> RemoveGroup(const FString& group) = 0;
	virtual bool IsGroupHasPermission(const FString& group, const FString& permission) = 0;
	virtual bool IsPlayerHasPermission(uint64 steam_id, const FString& permission) = 0;
	virtual std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission) = 0;
	virtual std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission) = 0;
};
