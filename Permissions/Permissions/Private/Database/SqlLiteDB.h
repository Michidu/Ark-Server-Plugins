#pragma once

#include <SQLiteCpp/Database.h>

#include "IDatabase.h"
#include "../Main.h"

class SqlLite : public IDatabase
{
public:
	explicit SqlLite(const std::string& path)
		: db_(path.empty()
			? Permissions::GetDbPath()
			: path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
	{
		try
		{
			db_.exec("PRAGMA journal_mode=WAL;");

			db_.exec("create table if not exists Players ("
				"Id integer primary key autoincrement not null,"
				"SteamId integer default 0,"
				"Groups text default 'Default,' COLLATE NOCASE"
				");");
			db_.exec("create table if not exists Groups ("
				"Id integer primary key autoincrement not null,"
				"GroupName text not null COLLATE NOCASE,"
				"Permissions text default '' COLLATE NOCASE"
				");");

			// Add default groups

			db_.exec("INSERT INTO Groups(GroupName, Permissions)"
				"SELECT 'Admins', '*,'"
				"WHERE NOT EXISTS(SELECT 1 FROM Groups WHERE GroupName = 'Admins');");
			db_.exec("INSERT INTO Groups(GroupName)"
				"SELECT 'Default'"
				"WHERE NOT EXISTS(SELECT 1 FROM Groups WHERE GroupName = 'Default');");
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	bool AddPlayer(uint64 steam_id) override
	{
		try
		{
			SQLite::Statement query(db_, "INSERT INTO Players (SteamId) VALUES (?);");
			query.bind(1, static_cast<int64>(steam_id));
			query.exec();

			std::lock_guard<std::mutex> lg(playersMutex);
			permissionPlayers[steam_id] = "Default, ";

			return true;
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
			SQLite::Statement query(db_, "UPDATE Players SET Groups = Groups || ? || ',' WHERE SteamId = ?;");
			query.bind(1, group.ToString());
			query.bind(2, static_cast<int64>(steam_id));
			query.exec();

			std::lock_guard<std::mutex> lg(playersMutex);
			std::string groups = fmt::format("{},{}", group.ToString(), permissionPlayers[steam_id]);
			permissionPlayers[steam_id] = groups;
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
			SQLite::Statement query(db_, "UPDATE Players SET Groups = ? WHERE SteamId = ?;");
			query.bind(1, new_groups.ToString());
			query.bind(2, static_cast<int64>(steam_id));
			query.exec();

			std::lock_guard<std::mutex> lg(playersMutex);
			permissionPlayers[steam_id] = new_groups.ToString();
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
			SQLite::Statement query(db_, "INSERT INTO Groups (GroupName) VALUES (?);");
			query.bind(1, group.ToString());
			query.exec();

			std::lock_guard<std::mutex> lg(groupsMutex);
			permissionGroups[group.ToString()] = "";
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
			SQLite::Statement query(db_, "DELETE FROM Groups WHERE GroupName = ?;");
			query.bind(1, group.ToString());
			query.exec();

			std::lock_guard<std::mutex> lg(groupsMutex);
			permissionGroups.erase(group.ToString());
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
			SQLite::Statement
				query(db_, "UPDATE Groups SET Permissions = Permissions || ? || ',' WHERE GroupName = ?;");
			query.bind(1, permission.ToString());
			query.bind(2, group.ToString());
			query.exec();

			std::lock_guard<std::mutex> lg(groupsMutex);
			std::string groupPermissions = fmt::format("{},{}", permission.ToString(), permissionGroups[group.ToString()]);
			permissionGroups[group.ToString()] = groupPermissions;
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
			SQLite::Statement query(db_, "UPDATE Groups SET Permissions = ? WHERE GroupName = ?;");
			query.bind(1, new_permissions.ToString());
			query.bind(2, group.ToString());
			query.exec();

			std::lock_guard<std::mutex> lg(groupsMutex);
			permissionGroups[group.ToString()] = new_permissions.ToString();
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
			SQLite::Statement query(db_, "SELECT GroupName, Permissions FROM Groups;");
			while (query.executeStep())
			{
				pGroups[query.getColumn(0).getText()] = query.getColumn(1).getText();
			}
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
			SQLite::Statement query(db_, "SELECT SteamId, Groups FROM Players;");
			if (query.executeStep())
			{
				pPlayers[query.getColumn(0).getInt64()] = query.getColumn(1).getText();
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return pPlayers;
	}

private:
	SQLite::Database db_;
};
