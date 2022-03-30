#pragma once

#include <mysql++11.h>

#include "IDatabase.h"

#pragma comment(lib, "mysqlclient.lib")

class MySql : public IDatabase
{
public:
	explicit MySql(std::string server, std::string username, std::string password, std::string db_name, const unsigned int port,
		std::string table_players, std::string table_groups, std::string table_tribes)
		: table_players_(move(table_players)), table_tribes_(move(table_tribes)),
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
				"TimedPermissionGroups VARCHAR(256) NOT NULL DEFAULT '',"
				"PRIMARY KEY(Id),"
				"UNIQUE INDEX SteamId_UNIQUE (SteamId ASC));", table_players_));
			result = db_.query(fmt::format("CREATE TABLE IF NOT EXISTS {} ("
				"Id INT NOT NULL AUTO_INCREMENT,"
				"TribeId BIGINT(11) NOT NULL,"
				"PermissionGroups VARCHAR(256) NOT NULL DEFAULT '',"
				"TimedPermissionGroups VARCHAR(256) NOT NULL DEFAULT '',"
				"PRIMARY KEY(Id),"
				"UNIQUE INDEX SteamId_UNIQUE (TribeId ASC));", table_tribes_));
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

			upgradeDatabase(db_name);

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

	bool IsFieldExists(std::string tableName, std::string fieldName) override
	{
		try
		{
			auto result = db_.query(fmt::format(
				"SHOW COLUMNS FROM {} LIKE '{}';",
				tableName, fieldName)).count();
			return result > 0;
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({}) Unexpected DB error {}", __FUNCTION__, exception.what());
			return false;
		}
	}

	bool IsPlayerExists(uint64 steam_id) override
	{
		std::lock_guard<std::mutex> lg(playersMutex);
		if (permissionPlayers.find(steam_id) == permissionPlayers.end())
			return false;

		return true;
	}

	bool AddPlayer(uint64 steam_id) override
	{
		try
		{
			if (db_.query(fmt::format("INSERT INTO {} (SteamId, PermissionGroups) VALUES ('{}', '{}');", table_players_, steam_id, "Default,")))
			{
				std::lock_guard<std::mutex> lg(playersMutex);
				permissionPlayers[steam_id] = CachedPermission("Default,", "");
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

	bool IsGroupExists(const FString& group) override
	{
		std::lock_guard<std::mutex> lg(groupsMutex);
		if (permissionGroups.find(group.ToString()) == permissionGroups.end())
			return false;

		return true;
	}

	TArray<FString> GetPlayerGroups(uint64 steam_id, bool includeTimed = true) override
	{
		TArray<FString> groups;

		if (IsPlayerExists(steam_id))
		{
			std::lock_guard<std::mutex> lg(playersMutex);
			if (includeTimed)
			{
				auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				groups = permissionPlayers[steam_id].getGroups(nowSecs);
			}
			else
			{
				groups = permissionPlayers[steam_id].Groups;
			}
		}

		return groups;
	}

	CachedPermission HydratePlayerGroups(uint64 steam_id) override
	{
		std::lock_guard<std::mutex> lg(playersMutex);
		return permissionPlayers[steam_id];
	}

	TArray<FString> GetGroupPermissions(const FString& group) override
	{
		if (group.IsEmpty())
			return {};

		TArray<FString> permissions;

		if (IsGroupExists(group))
		{
			std::lock_guard<std::mutex> lg(groupsMutex);
			FString permissions_fstr(permissionGroups[group.ToString()]);
			permissions_fstr.ParseIntoArray(permissions, L",", true);
		}

		return permissions;
	}

	TArray<FString> GetAllGroups() override
	{
		TArray<FString> all_groups;
		std::unordered_map<std::string, std::string> localGroups;

		groupsMutex.lock();
		localGroups = permissionGroups;
		groupsMutex.unlock();

		for (auto& group : localGroups)
		{
			all_groups.Add(group.first.c_str());
		}

		return all_groups;
	}

	TArray<uint64> GetGroupMembers(const FString& group) override
	{
		TArray<uint64> members;
		std::unordered_map<uint64, CachedPermission> localPlayers;

		playersMutex.lock();
		localPlayers = permissionPlayers;
		playersMutex.unlock();

		for (auto& players : localPlayers)
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
			auto groups = GetPlayerGroups(steam_id, false);
			groups.AddUnique(group);

			FString query_groups("");

			for (const FString& f : groups)
				query_groups += f + ",";

			const bool res = db_.query(fmt::format(
				"UPDATE {} SET PermissionGroups = '{}' WHERE SteamId = {};",
				table_players_, query_groups.ToString(), steam_id));
			if (!res)
			{
				return "Unexpected DB error";
			}
			else
			{
				std::lock_guard<std::mutex> lg(playersMutex);
				permissionPlayers[steam_id].Groups.AddUnique(group);
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

		TArray<FString> groups = GetPlayerGroups(steam_id, false);

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
				permissionPlayers[steam_id].Groups.Remove(group);
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

	std::optional<std::string> AddPlayerToTimedGroup(uint64 steam_id, const FString& group, int secs, int delaySecs) override
	{
		if (!IsPlayerExists(steam_id))
			AddPlayer(steam_id);

		if (!IsGroupExists(group))
			return  "Group does not exist";

		TArray<TimedGroup> groups;
		if (IsPlayerExists(steam_id))
		{
			playersMutex.lock();
			groups = permissionPlayers[steam_id].TimedGroups;
			playersMutex.unlock();
		}
		for (int32 Index = groups.Num() - 1; Index >= 0; --Index)
		{
			const TimedGroup& current_group = groups[Index];
			if (current_group.GroupName.Equals(group)) {
				groups.RemoveAt(Index);
				continue;
			}
		}
		if (Permissions::IsPlayerInGroup(steam_id, group))
			return "Player is already permanetly in this group.";

		long long ExpireAtSecs = 0;
		long long delayUntilSecs = 0;
		if (delaySecs > 0) {
			delayUntilSecs = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::seconds(delaySecs)).time_since_epoch()).count();
		}
		ExpireAtSecs = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::seconds(secs)).time_since_epoch()).count();

		groups.Add(TimedGroup{ group, delayUntilSecs, ExpireAtSecs });
		FString new_groups;
		for (const TimedGroup& current_group : groups)
		{
			new_groups += FString::Format("{};{};{},", current_group.DelayUntilTime, current_group.ExpireAtTime, current_group.GroupName.ToString());
		}
		try
		{
			const bool res = db_.query(fmt::format("UPDATE {} SET TimedPermissionGroups = '{}' WHERE SteamId = {};",
				table_players_, new_groups.ToString(), steam_id));
			if (!res)
			{
				return "Unexpected DB error";
			}
			else
			{
				std::lock_guard<std::mutex> lg(playersMutex);
				permissionPlayers[steam_id].TimedGroups = groups;
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	std::optional<std::string> RemovePlayerFromTimedGroup(uint64 steam_id, const FString& group) override
	{
		if (!IsPlayerExists(steam_id) || !IsGroupExists(group))
			return "Player or group does not exist";

		playersMutex.lock();
		TArray<TimedGroup> groups = permissionPlayers[steam_id].TimedGroups;
		playersMutex.unlock();

		FString new_groups;

		int32 groupIndex = INDEX_NONE;
		for (int32 Index = 0; Index != groups.Num(); ++Index)
		{
			const TimedGroup& current_group = groups[Index];
			if (current_group.GroupName != group)
				new_groups += FString::Format("{};{};{},", current_group.DelayUntilTime, current_group.ExpireAtTime, current_group.GroupName.ToString());
			else
				groupIndex = Index;
		}
		if (groupIndex == INDEX_NONE)
			return "Player is not in timed group";

		try
		{
			const bool res = db_.query(fmt::format("UPDATE {} SET TimedPermissionGroups = '{}' WHERE SteamId = {};",
				table_players_, new_groups.ToString(), steam_id));
			if (!res)
			{
				return "Unexpected DB error";
			}
			else
			{
				std::lock_guard<std::mutex> lg(playersMutex);
				permissionPlayers[steam_id].TimedGroups.RemoveAt(groupIndex);
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	void UpdatePlayerGroupCallbacks(uint64 steamId, TArray<FString> groups) override
	{
		std::lock_guard<std::mutex> lg(playersMutex);
		permissionPlayers[steamId].CallbackGroups = groups;
	}

	bool IsTribeExists(int tribeId) override
	{
		bool found = false;

		std::lock_guard<std::mutex> lg(tribesMutex);
		if (permissionTribes.find(tribeId) == permissionTribes.end())
			found = false;
		else
			found = true;

		return found;
	}

	bool AddTribe(int tribeId) override
	{
		try
		{
			if (db_.query(fmt::format("INSERT INTO {} (TribeId) VALUES ({});", table_tribes_, tribeId)))
			{
				std::lock_guard<std::mutex> lg(tribesMutex);
				permissionTribes[tribeId] = CachedPermission("", "");
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

	TArray<FString> GetTribeGroups(int tribeId, bool includeTimed = true) override
	{
		TArray<FString> groups;

		if (IsTribeExists(tribeId))
		{
			std::lock_guard<std::mutex> lg(tribesMutex);
			if (includeTimed)
			{
				auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				groups = permissionTribes[tribeId].getGroups(nowSecs);
			}
			else
			{
				groups = permissionTribes[tribeId].Groups;
			}
		}

		return groups;
	}

	CachedPermission HydrateTribeGroups(int tribeId) override
	{
		std::lock_guard<std::mutex> lg(tribesMutex);
		return permissionTribes[tribeId];
	}

	std::optional<std::string> AddTribeToGroup(int tribeId, const FString& group) override
	{
		if (!IsTribeExists(tribeId))
			AddTribe(tribeId);

		if (!IsGroupExists(group))
			return  "Group does not exist";

		if (Permissions::IsTribeInGroup(tribeId, group))
			return "Tribe was already added";

		try
		{
			auto groups = GetTribeGroups(tribeId, false);
			groups.AddUnique(group);

			FString query_groups("");

			for (const FString& f : groups)
				query_groups += f + ",";

			const bool res = db_.query(fmt::format(
				"UPDATE {} SET PermissionGroups = '{}' WHERE TribeId = {};",
				table_tribes_, query_groups.ToString(), tribeId));
			if (!res)
			{
				return "Unexpected DB error";
			}
			else
			{
				std::lock_guard<std::mutex> lg(tribesMutex);
				permissionTribes[tribeId].Groups.Add(group);
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	std::optional<std::string> RemoveTribeFromGroup(int tribeId, const FString& group) override
	{
		if (!IsTribeExists(tribeId) || !IsGroupExists(group))
			return "Tribe or group does not exist";

		if (!Permissions::IsTribeInGroup(tribeId, group))
			return "Tribe is not in group";

		TArray<FString> groups = GetTribeGroups(tribeId, false);

		FString new_groups;

		for (const FString& current_group : groups)
		{
			if (current_group != group)
				new_groups += current_group + ",";
		}

		try
		{
			const bool res = db_.query(fmt::format("UPDATE {} SET PermissionGroups = '{}' WHERE TribeId = {};",
				table_tribes_, new_groups.ToString(), tribeId));
			if (!res)
			{
				return "Unexpected DB error";
			}
			else
			{
				std::lock_guard<std::mutex> lg(tribesMutex);
				permissionTribes[tribeId].Groups.Remove(group);
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	std::optional<std::string> AddTribeToTimedGroup(int tribeId, const FString& group, int secs, int delaySecs) override
	{
		if (!IsTribeExists(tribeId))
			AddTribe(tribeId);

		if (!IsGroupExists(group))
			return  "Group does not exist";

		TArray<TimedGroup> groups;
		if (IsTribeExists(tribeId))
		{
			tribesMutex.lock();
			groups = permissionTribes[tribeId].TimedGroups;
			tribesMutex.unlock();
		}
		for (int32 Index = groups.Num() - 1; Index >= 0; --Index)
		{
			const TimedGroup& current_group = groups[Index];
			if (current_group.GroupName.Equals(group)) {
				groups.RemoveAt(Index);
				continue;
			}
		}
		if (Permissions::IsTribeInGroup(tribeId, group))
			return "Tribe is already permanetly in this group.";

		long long ExpireAtSecs = 0;
		long long delayUntilSecs = 0;
		if (delaySecs > 0) {
			delayUntilSecs = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::seconds(delaySecs)).time_since_epoch()).count();
		}
		ExpireAtSecs = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::seconds(secs)).time_since_epoch()).count();

		groups.Add(TimedGroup{ group, delayUntilSecs, ExpireAtSecs });
		FString new_groups;
		for (const TimedGroup& current_group : groups)
		{
			new_groups += FString::Format("{};{};{},", current_group.DelayUntilTime, current_group.ExpireAtTime, current_group.GroupName.ToString());
		}
		try
		{
			const bool res = db_.query(fmt::format("UPDATE {} SET TimedPermissionGroups = '{}' WHERE TribeId = {};",
				table_tribes_, new_groups.ToString(), tribeId));
			if (!res)
			{
				return "Unexpected DB error";
			}
			else
			{
				std::lock_guard<std::mutex> lg(tribesMutex);
				permissionTribes[tribeId].TimedGroups = groups;
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	std::optional<std::string> RemoveTribeFromTimedGroup(int tribeId, const FString& group) override
	{
		if (!IsTribeExists(tribeId) || !IsGroupExists(group))
			return "Tribe or group does not exist";

		tribesMutex.lock();
		TArray<TimedGroup> groups = permissionTribes[tribeId].TimedGroups;
		tribesMutex.unlock();

		FString new_groups;

		int32 groupIndex = INDEX_NONE;
		for (int32 Index = 0; Index != groups.Num(); ++Index)
		{
			const TimedGroup& current_group = groups[Index];
			if (current_group.GroupName != group)
				new_groups += FString::Format("{};{};{},", current_group.DelayUntilTime, current_group.ExpireAtTime, current_group.GroupName.ToString());
			else
				groupIndex = Index;
		}
		if (groupIndex == INDEX_NONE)
			return "Tribe is not in timed group";

		try
		{
			const bool res = db_.query(fmt::format("UPDATE {} SET TimedPermissionGroups = '{}' WHERE TribeId = {};",
				table_tribes_, new_groups.ToString(), tribeId));
			if (!res)
			{
				return "Unexpected DB error";
			}
			else
			{
				std::lock_guard<std::mutex> lg(tribesMutex);
				permissionTribes[tribeId].TimedGroups.RemoveAt(groupIndex);
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}

		return {};
	}

	void UpdateTribeGroupCallbacks(int tribeId, TArray<FString> groups) override
	{
		std::lock_guard<std::mutex> lg(tribesMutex);
		permissionTribes[tribeId].CallbackGroups = groups;
	}

	void Init() override
	{
		auto pGroups = InitGroups();
		groupsMutex.lock();
		permissionGroups = pGroups;
		groupsMutex.unlock();

		auto pPlayers = InitPlayers();
		playersMutex.lock();
		permissionPlayers = pPlayers;
		playersMutex.unlock();

		auto pTribes = InitTribes();
		tribesMutex.lock();
		permissionTribes = pTribes;
		tribesMutex.unlock();
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

	std::unordered_map<uint64, CachedPermission> InitPlayers() override
	{
		std::unordered_map<uint64, CachedPermission> pPlayers;

		try
		{
			db_.query(fmt::format("SELECT SteamId, PermissionGroups, TimedPermissionGroups FROM {};", table_players_))
				.each([&pPlayers](uint64 steam_id, std::string groups, std::string timedGroups)
					{
						pPlayers[steam_id] = CachedPermission(FString(groups), FString(timedGroups));
						return true;
					});
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return pPlayers;
	}

	std::unordered_map<int, CachedPermission> InitTribes() override
	{
		std::unordered_map<int, CachedPermission> pTribes;

		try
		{
			db_.query(fmt::format("SELECT TribeId, PermissionGroups, TimedPermissionGroups FROM {};", table_tribes_))
				.each([&pTribes](int tribeId, std::string groups, std::string timedGroups)
					{
						pTribes[tribeId] = CachedPermission(FString(groups), FString(timedGroups));
						return true;
					});
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return pTribes;
	}

	void upgradeDatabase(std::string db_name)
	{
		if (!IsFieldExists(table_players_, "TimedPermissionGroups"))
		{
			if (!db_.query(fmt::format("ALTER TABLE {} ADD COLUMN TimedPermissionGroups VARCHAR(256) DEFAULT '' AFTER PermissionGroups;", table_players_)))
			{
				Log::GetLog()->critical("({} {}) Failed to update Permissions {} table!", __FILE__, __FUNCTION__, table_players_);
			}
		}
	}

private:
	daotk::mysql::connection db_;
	std::string table_players_;
	std::string table_tribes_;
	std::string table_groups_;
};
