#pragma once

#include <mysql++11.h>

#include "IDatabase.h"

#pragma comment(lib, "mysqlclient.lib")

class MySql : public IDatabase
{
public:
	explicit MySql(std::string server, std::string username, std::string password, std::string db_name,
	               std::string table_players, std::string table_groups)
		: table_players_(move(table_players)),
		  table_groups_(move(table_groups))
	{
		try
		{
			daotk::mysql::connect_options options;
			options.server = move(server);
			options.username = move(username);
			options.password = move(password);
			options.dbname = move(db_name);
			options.autoreconnect = true;
			options.timeout = 30;

			bool result = db_.open(options);
			if (!result)
			{
				Log::GetLog()->critical("Failed to open connection!");
				return;
			}

			result = db_.query(fmt::format("CREATE TABLE IF NOT EXISTS {} ("
			                               "Id INT NOT NULL AUTO_INCREMENT,"
			                               "SteamId BIGINT(11) NOT NULL,"
			                               "PermissionGroups VARCHAR(256) NOT NULL DEFAULT 'Default,',"
			                               "PRIMARY KEY(Id),"
			                               "UNIQUE INDEX SteamId_UNIQUE (SteamId ASC));", table_players_));
			result |= db_.query(fmt::format("CREATE TABLE IF NOT EXISTS {} ("
			                                "Id INT NOT NULL AUTO_INCREMENT,"
			                                "GroupName VARCHAR(128) NOT NULL,"
			                                "Permissions VARCHAR(768) NOT NULL DEFAULT '',"
			                                "PRIMARY KEY(Id),"
			                                "UNIQUE INDEX GroupName_UNIQUE (GroupName ASC));", table_groups_));

			// Add default groups

			result |= db_.query(fmt::format("INSERT INTO {} (GroupName, Permissions)"
			                                "SELECT 'Admins', '*,'"
			                                "WHERE NOT EXISTS(SELECT 1 FROM {} WHERE GroupName = 'Admins');",
			                                table_groups_,
			                                table_groups_));
			result |= db_.query(fmt::format("INSERT INTO {} (GroupName)"
			                                "SELECT 'Default'"
			                                "WHERE NOT EXISTS(SELECT 1 FROM {} WHERE GroupName = 'Default');",
			                                table_groups_,
			                                table_groups_));

			if (!result)
			{
				Log::GetLog()->critical("({} {}) Failed to create table!", __FILE__, __FUNCTION__);
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->critical("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	bool AddPlayer(uint64 steam_id) override
	{
		try
		{
			return db_.query(fmt::format("INSERT INTO {} (SteamId) VALUES ({});", table_players_, steam_id));
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool IsPlayerExists(uint64 steam_id) override
	{
		try
		{
			const auto result = db_.query(fmt::format("SELECT count(1) FROM {} WHERE SteamId = {};", table_players_,
			                                          steam_id))
			                       .get_value<int>();
			return result > 0;
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
	}

	bool IsGroupExists(const FString& group) override
	{
		try
		{
			const auto result = db_.query(fmt::format("SELECT count(1) FROM {} WHERE GroupName = '{}';", table_groups_,
			                                          group.ToString()))
			                       .get_value<int>();

			return result > 0;
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return false;
	}

	TArray<FString> GetPlayerGroups(uint64 steam_id) override
	{
		TArray<FString> groups;

		try
		{
			const auto permission_groups = db_.query(
				                                  fmt::format(
					                                  "SELECT PermissionGroups FROM {} WHERE SteamId = {};",
					                                  table_players_, steam_id))
			                                  .get_value<daotk::mysql::optional_type<std::string>>();

			if (permission_groups)
			{
				FString groups_fstr(*permission_groups);
				groups_fstr.ParseIntoArray(groups, L",", true);
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

		try
		{
			const std::string permission_groups = db_.query(fmt::format(
				                                         "SELECT Permissions FROM {} WHERE GroupName = '{}';",
				                                         table_groups_, group.ToString()))
			                                         .get_value<std::string>();

			FString permissions_fstr(permission_groups);
			permissions_fstr.ParseIntoArray(permissions, L",", true);
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

		try
		{
			db_.query(fmt::format("SELECT GroupName FROM {};", table_groups_))
			   .each([&all_groups](std::string group)
			   {
				   all_groups.Add(group.c_str());
				   return true;
			   });
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

		try
		{
			db_.query(fmt::format("SELECT SteamId FROM {};", table_players_))
			   .each([&members, &group](uint64 steam_id)
			   {
				   if (Permissions::IsPlayerInGroup(steam_id, group))
					   members.Add(steam_id);

				   return true;
			   });
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return members;
	}

	std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group) override
	{
		if (!IsPlayerExists(steam_id))
			AddPlayer(steam_id);

		if (!IsGroupExists(group))
			return  "Group does not exist";

		if (Permissions::IsPlayerInGroup(steam_id, group))
			return "Player was already added";

		try
		{
			const bool res = db_.query(fmt::format(
				"UPDATE {} SET PermissionGroups = concat(PermissionGroups, '{},') WHERE SteamId = {};",
				table_players_, group.ToString(), steam_id));
			if (!res)
			{
				return "Unexpected DB error";
			}
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

		if (!Permissions::IsPlayerInGroup(steam_id, group))
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
			const bool res = db_.query(fmt::format("UPDATE {} SET PermissionGroups = '{}' WHERE SteamId = {};",
			                                       table_players_, new_groups.ToString(), steam_id));
			if (!res)
			{
				return "Unexpected DB error";
			}
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
			const bool res = db_.query(fmt::format("INSERT INTO {} (GroupName) VALUES ('{}');", table_groups_,
			                                       group.ToString()));
			if (!res)
			{
				return "Unexpected DB error";
			}
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
			const bool res = db_.query(fmt::format("DELETE FROM {} WHERE GroupName = '{}';", table_groups_,
			                                       group.ToString()));
			if (!res)
			{
				return "Unexpected DB error";
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission) override
	{
		if (!IsGroupExists(group))
			return "Group does not exist";

		if (Permissions::IsGroupHasPermission(group, permission))
			return "Group already has this permission";

		try
		{
			const bool res = db_.query(fmt::format(
				"UPDATE {} SET Permissions = concat(Permissions, '{},') WHERE GroupName = '{}';",
				table_groups_, permission.ToString(), group.ToString()));
			if (!res)
			{
				return "Unexpected DB error";
			}
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

		if (!Permissions::IsGroupHasPermission(group, permission))
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
			const bool res = db_.query(fmt::format("UPDATE {} SET Permissions = '{}' WHERE GroupName = '{}';",
			                                       table_groups_, new_permissions.ToString(), group.ToString()));
			if (!res)
			{
				return "Unexpected DB error";
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

private:
	daotk::mysql::connection db_;
	std::string table_players_;
	std::string table_groups_;
};
