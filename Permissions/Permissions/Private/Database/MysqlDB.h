#pragma once
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include "MysqlDBObj.h"
#pragma comment(lib, "mysqlclient.lib")
#pragma comment(lib, "sqlpp-mysql.lib")

namespace mysql = sqlpp::mysql;

class Mysql : public IDatabase
{
private:
	std::shared_ptr<mysql::connection_config> config;
	mysql::connection& GetDB()
	{
		static mysql::connection db(config);
		return db;
	}
public:
	virtual void InitDB(const std::string& Host, const std::string& User, const std::string& Pass, const std::string& DB, int Port)
	{
		try
		{
			config = std::make_shared<mysql::connection_config>();
			config->host = Host;
			config->user = User;
			config->password = Pass;
			config->database = DB;
			config->port = Port;
			config->debug = true;
			config->auto_reconnect = true;

			GetDB().execute("CREATE TABLE IF NOT EXISTS `Players` ("
				"`Id` INT NOT NULL AUTO_INCREMENT,"
				"`SteamId` BIGINT(11) NOT NULL,"
				"`Groups` VARCHAR(256) NOT NULL DEFAULT '',"
				"PRIMARY KEY(`Id`),"
				"UNIQUE INDEX `SteamId_UNIQUE` (`SteamId` ASC)); ");

			GetDB().execute("CREATE TABLE IF NOT EXISTS `Groups` ("
				"`Id` INT NOT NULL AUTO_INCREMENT,"
				"`GroupName` VARCHAR(128) NOT NULL,"
				"`Permissions` VARCHAR(256) NOT NULL DEFAULT '',"
				"PRIMARY KEY(`Id`),"
				"UNIQUE INDEX `GroupName_UNIQUE` (`GroupName` ASC)); ");

			// Add default groups
			GetDB().execute("INSERT INTO Groups(GroupName, Permissions)"
				"SELECT 'Admins', '*,'"
				"WHERE NOT EXISTS(SELECT 1 FROM Groups WHERE GroupName = 'Admins');");
			GetDB().execute("INSERT INTO Groups(GroupName)"
				"SELECT 'Default'"
				"WHERE NOT EXISTS(SELECT 1 FROM Groups WHERE GroupName = 'Default');");
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	virtual void AddPlayer(uint64 steam_id)
	{
		try
		{
			GetDB().execute(fmt::format("INSERT INTO Players (SteamId) VALUES ({});", steam_id));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}


	virtual bool IsPlayerExists(uint64 steam_id)
	{
		const auto& players = Players{};
		try
		{
			return GetDB()(sqlpp::select(count(players.id)).from(players).where(players.steamid == steam_id)).front().count > 0;
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
		return false;
	}

	virtual bool IsGroupExists(const FString& group)
	{
		const auto& groups = Groups{};
		try
		{
			return GetDB()(sqlpp::select(count(groups.id)).from(groups).where(groups.groupname == group.ToString())).front().count > 0;
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
		return false;
	}

	virtual bool IsPlayerInGroup(uint64 steam_id, const FString& group)
	{
		TArray<FString> groups = GetPlayerGroups(steam_id);

		for (const auto& current_group : groups)
		{
			if (current_group == group)
				return true;
		}
		return false;
	}

	virtual TArray<FString> GetPlayerGroups(uint64 steam_id)
	{
		TArray<FString> groups;
		const auto& players = Players{};
		try
		{
			for (const auto& row : GetDB()(sqlpp::select().columns(players.groups).from(players).where(players.steamid == steam_id)))
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

	virtual TArray<FString> GetGroupPermissions(const FString& group)
	{
		if (group.IsEmpty())
			return {};

		TArray<FString> permissions;
		const auto& groups = Groups{};
		try
		{
			for (const auto& row : GetDB()(sqlpp::select().columns(groups.permissions).from(groups).where(groups.groupname == group.ToString())))
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

	virtual TArray<uint64> GetGroupMembers(const FString& group)
	{
		TArray<uint64> members;
		const auto& players = Players{};
		try
		{
			for (const auto& row : GetDB()(sqlpp::select().columns(players.steamid).from(players).unconditionally()))
			{
				if (IsPlayerInGroup(row.steamid, group))
					members.Add(row.steamid);
			};
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
		return members;
	}

	virtual std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group)
	{
		if (!IsPlayerExists(steam_id)) 
			return "Player or group does not exist";
		if (!IsGroupExists(group))
			return "Player or group does not exist111111";
		if (IsPlayerInGroup(steam_id, group))
			return "Player was already added";
		try
		{
			GetDB().execute(fmt::format("UPDATE Players SET Groups = concat(Groups, '{},') WHERE SteamId = {};",  group.ToString(), steam_id));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}
		return {};
	}

	virtual std::optional<std::string> RemovePlayerFromGroup(uint64 steam_id, const FString& group)
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
			GetDB().execute(fmt::format("UPDATE Players SET Groups = '{}' WHERE SteamId = {};", new_groups.ToString(), steam_id));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}
		return {};
	}

	virtual std::optional<std::string> AddGroup(const FString& group)
	{
		if (IsGroupExists(group))
			return "Group already exists";

		try
		{
			GetDB().execute(fmt::format("INSERT INTO Groups (GroupName) VALUES ('{}');", group.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}
		return {};
	}

	virtual std::optional<std::string> RemoveGroup(const FString& group)
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
			GetDB().execute(fmt::format("DELETE FROM Groups WHERE GroupName = '?';", group.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}
		return {};
	}

	virtual bool IsGroupHasPermission(const FString& group, const FString& permission)
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

	virtual bool IsPlayerHasPermission(uint64 steam_id, const FString& permission)
	{
		TArray<FString> groups = GetPlayerGroups(steam_id);

		for (const auto& current_group : groups)
		{
			if (IsGroupHasPermission(current_group, permission) || IsGroupHasPermission(current_group, "*"))
				return true;
		}
		return false;
	}

	virtual std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission)
	{
		if (!IsGroupExists(group))
			return "Group does not exist";

		if (IsGroupHasPermission(group, permission))
			return "Group already has this permission";

		try
		{
			GetDB().execute(fmt::format("UPDATE Groups SET Permissions = concat(Permissions, '{},') WHERE GroupName = '{}';", permission.ToString(), group.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}
		return {};
	}

	virtual std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission)
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
			GetDB().execute(fmt::format("UPDATE Groups SET Permissions = '?' WHERE GroupName = '?';", new_permissions.ToString(), group.ToString()));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}
		return {};
	}
};