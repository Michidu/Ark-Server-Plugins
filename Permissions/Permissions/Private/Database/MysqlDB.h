#pragma once

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include "MysqlDBObj.h"

#pragma comment(lib, "mysqlclient.lib")
#pragma comment(lib, "sqlpp-mysql.lib")

namespace mysql = sqlpp::mysql;

class MySql : public IDatabase
{
public:
	explicit MySql(const std::shared_ptr<mysql::connection_config>& config)
		: db_(config)
	{
		try
		{
			db_.execute("CREATE TABLE IF NOT EXISTS Players ("
				"Id INT NOT NULL AUTO_INCREMENT,"
				"SteamId BIGINT(11) NOT NULL,"
				"PermissionGroups VARCHAR(256) NOT NULL DEFAULT '',"
				"PRIMARY KEY(Id),"
				"UNIQUE INDEX SteamId_UNIQUE (SteamId ASC)); ");
			db_.execute("CREATE TABLE IF NOT EXISTS PermissionGroups ("
				"Id INT NOT NULL AUTO_INCREMENT,"
				"GroupName VARCHAR(128) NOT NULL,"
				"Permissions VARCHAR(256) NOT NULL DEFAULT '',"
				"PRIMARY KEY(Id),"
				"UNIQUE INDEX GroupName_UNIQUE (GroupName ASC)); ");

			// Add default groups

			db_.execute("INSERT INTO PermissionGroups(GroupName, Permissions)"
				"SELECT 'Admins', '*,'"
				"WHERE NOT EXISTS(SELECT 1 FROM PermissionGroups WHERE GroupName = 'Admins');");
			db_.execute("INSERT INTO PermissionGroups(GroupName)"
				"SELECT 'Default'"
				"WHERE NOT EXISTS(SELECT 1 FROM PermissionGroups WHERE GroupName = 'Default');");
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	void AddPlayer(uint64 steam_id) override
	{
		try
		{
			db_.execute(fmt::format("INSERT INTO Players (SteamId) VALUES ({});", steam_id));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	bool IsPlayerExists(uint64 steam_id) override
	{
		const auto& players = Players{};

		try
		{
			return db_(sqlpp::select(count(players.id)).from(players).where(players.steamid == steam_id)).front().count > 0;
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool IsGroupExists(const FString& group) override
	{
		const auto& groups = Groups{};
		try
		{
			return db_(sqlpp::select(count(groups.id)).from(groups).where(groups.groupname == group.ToString()))
			       .front().count > 0;
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return false;
	}

	bool IsPlayerInGroup(uint64 steam_id, const FString& group) override
	{
		TArray<FString> groups = GetPlayerGroups(steam_id);

		for (const auto& current_group : groups)
		{
			if (current_group == group)
				return true;
		}

		return false;
	}

	TArray<FString> GetPlayerGroups(uint64 steam_id) override
	{
		TArray<FString> groups;
		const auto& players = Players{};
		try
		{
			for (const auto& row : db_(
				     sqlpp::select().columns(players.groups).from(players).where(players.steamid == steam_id)))
			{
				FString groups_fstr(row.groups.text);
				groups_fstr.ParseIntoArray(groups, L",", true);
				break;
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return groups;
	}

	TArray<FString> GetGroupPermissions(const FString& group) override
	{
		if (group.IsEmpty())
			return {};

		TArray<FString> permissions;
		const auto& groups = Groups{};
		try
		{
			for (const auto& row : db_(
				     sqlpp::select().columns(groups.permissions).from(groups).where(groups.groupname == group.ToString())))
			{
				FString permissions_fstr(row.permissions.text);
				permissions_fstr.ParseIntoArray(permissions, L",", true);
				break;
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return permissions;
	}

	TArray<FString> GetAllGroups() override
	{
		TArray<FString> all_groups;
		const auto& groups = Groups{};
		try
		{
			for (const auto& row : db_(
				     sqlpp::select().columns(groups.groupname).from(groups).where(groups.id > -1)))
				// Bug within library: doesn't compile without 'where'
			{
				all_groups.Add(row.groupname.text);
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return all_groups;
	}

	TArray<uint64> GetGroupMembers(const FString& group) override
	{
		TArray<uint64> members;
		const auto& players = Players{};
		try
		{
			for (const auto& row : db_(sqlpp::select().columns(players.steamid).from(players).unconditionally()))
			{
				if (IsPlayerInGroup(row.steamid, group))
					members.Add(row.steamid);
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return members;
	}

	std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group) override
	{
		if (!IsPlayerExists(steam_id) || !IsGroupExists(group))
			return "Player or group does not exist";

		if (IsPlayerInGroup(steam_id, group))
			return "Player was already added";

		try
		{
			db_.execute(fmt::format("UPDATE Players SET PermissionGroups = concat(PermissionGroups, '{},') WHERE SteamId = {};",
			                        group.ToString(), steam_id));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	std::optional<std::string> RemovePlayerFromGroup(uint64 steam_id, const FString& group) override
	{
		if (!IsPlayerExists(steam_id) || !IsGroupExists(group))
			return "Player or group does not exist";

		if (!IsPlayerInGroup(steam_id, group))
			return "Player is not in group";

		TArray<FString> groups = GetPlayerGroups(steam_id);

		FString new_groups;

		for (const FString& current_group : groups)
		{
			if (current_group != group)
				new_groups += current_group + ",";
		}

		try
		{
			db_.execute(fmt::format("UPDATE Players SET PermissionGroups = '{}' WHERE SteamId = {};", new_groups.ToString(),
			                        steam_id));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	std::optional<std::string> AddGroup(const FString& group) override
	{
		if (IsGroupExists(group))
			return "Group already exists";

		try
		{
			db_.execute(fmt::format("INSERT INTO PermissionGroups (GroupName) VALUES ('{}');", group.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	std::optional<std::string> RemoveGroup(const FString& group) override
	{
		if (!IsGroupExists(group))
			return "Group does not exist";

		// Remove all players from this group

		TArray<uint64> group_members = GetGroupMembers(group);
		for (uint64 player : group_members)
		{
			RemovePlayerFromGroup(player, group);
		}

		// Delete group

		try
		{
			db_.execute(fmt::format("DELETE FROM PermissionGroups WHERE GroupName = '?';", group.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	bool IsGroupHasPermission(const FString& group, const FString& permission) override
	{
		if (!IsGroupExists(group))
			return false;

		TArray<FString> permissions = GetGroupPermissions(group);

		for (const auto& current_perm : permissions)
		{
			if (current_perm == permission)
				return true;
		}

		return false;
	}

	bool IsPlayerHasPermission(uint64 steam_id, const FString& permission) override
	{
		TArray<FString> groups = GetPlayerGroups(steam_id);

		for (const auto& current_group : groups)
		{
			if (IsGroupHasPermission(current_group, permission) || IsGroupHasPermission(current_group, "*"))
				return true;
		}

		return false;
	}

	std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission) override
	{
		if (!IsGroupExists(group))
			return "Group does not exist";

		if (IsGroupHasPermission(group, permission))
			return "Group already has this permission";

		try
		{
			db_.execute(fmt::format(
				"UPDATE PermissionGroups SET Permissions = concat(Permissions, '{},') WHERE GroupName = '{}';",
				permission.ToString(), group.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission) override
	{
		if (!IsGroupExists(group))
			return "Group does not exist";

		if (!IsGroupHasPermission(group, permission))
			return "Group does not have this permission";

		TArray<FString> permissions = GetGroupPermissions(group);

		FString new_permissions;

		for (const FString& current_perm : permissions)
		{
			if (current_perm != permission)
				new_permissions += current_perm + ",";
		}

		try
		{
			db_.execute(fmt::format("UPDATE PermissionGroups SET Permissions = '?' WHERE GroupName = '?';",
			                        new_permissions.ToString(),
			                        group.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

private:
	mysql::connection db_;
};
