#pragma once

#include <mysql++11.h>

#include "IDatabase.h"

#pragma comment(lib, "mysqlclient.lib")

class MySql : public IDatabase
{
public:
	explicit MySql(std::string server, std::string username, std::string password, std::string db_name, const unsigned int port,
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
			options.port = port;

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
			if (db_.query(fmt::format("INSERT INTO {} (SteamId) VALUES ({});", table_players_, steam_id)))
			{
				std::lock_guard<std::mutex> lg(playersMutex);
				permissionPlayers[steam_id] = "Default,";
				return true;
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return false;
	}

	bool IsPlayerExists(uint64 steam_id) override
	{
		return permissionPlayers.count(steam_id) > 0;
	}

	bool IsGroupExists(const FString& group) override
	{
		return permissionGroups.count(group.ToString()) > 0;
	}

	TArray<FString> GetPlayerGroups(uint64 steam_id) override
	{
		TArray<FString> groups;

		if (permissionPlayers.count(steam_id) > 0)
		{
			FString groups_fstr(permissionPlayers[steam_id]);
			groups_fstr.ParseIntoArray(groups, L",", true);
		}

		return groups;
	}

	TArray<FString> GetGroupPermissions(const FString& group) override
	{
		if (group.IsEmpty())
			return {};

		TArray<FString> permissions;

		if (permissionGroups.count(group.ToString()) > 0)
		{
			FString permissions_fstr(permissionGroups[group.ToString()]);
			permissions_fstr.ParseIntoArray(permissions, L",", true);
		}

		return permissions;
	}

	TArray<FString> GetAllGroups() override
	{
		TArray<FString> all_groups;

		for (auto& group : permissionGroups)
		{
			all_groups.Add(group.second.c_str());
		}

		return all_groups;
	}

	TArray<uint64> GetGroupMembers(const FString& group) override
	{
		TArray<uint64> members;

		for (auto& players : permissionPlayers)
		{
			if (Permissions::IsPlayerInGroup(players.first, group))
				members.Add(players.first);
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
			else
			{
				std::lock_guard<std::mutex> lg(playersMutex);
				std::string groups = fmt::format("{},{}", group.ToString(), permissionPlayers[steam_id]);
				permissionPlayers[steam_id] = groups;
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
			else
			{
				std::lock_guard<std::mutex> lg(playersMutex);
				permissionPlayers[steam_id] = new_groups.ToString();
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
			else
			{
				std::lock_guard<std::mutex> lg(groupsMutex);
				permissionGroups[group.ToString()] = "";
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
			else
			{
				std::lock_guard<std::mutex> lg(groupsMutex);
				permissionGroups.erase(group.ToString());
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
			else
			{
				std::lock_guard<std::mutex> lg(groupsMutex);
				std::string groupPermissions = fmt::format("{},{}", permission.ToString(), permissionGroups[group.ToString()]);
				permissionGroups[group.ToString()] = groupPermissions;
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
			else
			{
				std::lock_guard<std::mutex> lg(groupsMutex);
				permissionGroups[group.ToString()] = new_permissions.ToString();
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	void Init() override
	{
		groupsMutex.lock();
		permissionGroups = InitGroups();
		groupsMutex.unlock();

		playersMutex.lock();
		permissionPlayers = InitPlayers();
		playersMutex.unlock();
	}

	std::unordered_map<std::string, std::string> InitGroups() override
	{
		std::unordered_map<std::string, std::string> pGroups;

		try
		{
			db_.query(fmt::format("SELECT GroupName, Permissions FROM {};", table_groups_))
				.each([&pGroups](std::string groupName, std::string groupPermissions)
					{
						pGroups[groupName] = groupPermissions;
						return true;
					});
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return pGroups;
	}

	std::unordered_map<uint64, std::string> InitPlayers() override
	{
		std::unordered_map<uint64, std::string> pPlayers;

		try
		{
			db_.query(fmt::format("SELECT SteamId, PermissionGroups FROM {};", table_players_))
				.each([&pPlayers](uint64 steam_id, std::string groups)
					{
						pPlayers[steam_id] = groups;
						return true;
					});
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return pPlayers;
	}

private:
	daotk::mysql::connection db_;
	std::string table_players_;
	std::string table_groups_;
};
