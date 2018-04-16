#pragma 
#include "hdr/sqlite_modern_cpp.h"

class SqlLite : public IDatabase
{
private:
	std::string DBConnection;
public:
	sqlite::database& LiteDB()
	{
		static sqlite::database db(DBConnection.empty()
			? ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/Permissions/ArkDB.db"
			: DBConnection);

		return db;
	}

	virtual void InitDB(const std::string& Host, const std::string& User, const std::string& Pass, const std::string& DBConnection, int Port)
	{
		this->DBConnection = DBConnection;
		try
		{
			auto& db = LiteDB();

			db << "create table if not exists Players ("
				"Id integer primary key autoincrement not null,"
				"SteamId integer default 0,"
				"Groups text default 'Default,' COLLATE NOCASE"
				");";
			db << "create table if not exists Groups ("
				"Id integer primary key autoincrement not null,"
				"GroupName text not null COLLATE NOCASE,"
				"Permissions text default '' COLLATE NOCASE"
				");";

			// Add default groups

			db << "INSERT INTO Groups(GroupName, Permissions)"
				"SELECT 'Admins', '*,'"
				"WHERE NOT EXISTS(SELECT 1 FROM Groups WHERE GroupName = 'Admins');";
			db << "INSERT INTO Groups(GroupName)"
				"SELECT 'Default'"
				"WHERE NOT EXISTS(SELECT 1 FROM Groups WHERE GroupName = 'Default');";

			// Load Cached Data
		/*	std::hash<std::string> HashString;
			TArray<FString> FStringArray;

			db << "SELECT GroupName, Permissions FROM Groups;"
				>> [&](const std::string& GroupName, const std::string& Permissions)
			{

				FString Permissions_fstr(Permissions.c_str());
				Permissions_fstr.ParseIntoArray(FStringArray, L",", true);
				TArray<size_t> PermArray;
				for (const auto& Perm : FStringArray)
				{
					PermArray.Add(HashString(Perm.ToString()));
				}
				GroupCache.insert(std::map<size_t, TArray<size_t>>::value_type(HashString(GroupName), PermArray));
			};

			db << "SELECT SteamId, Groups FROM Players;"
				>> [&](const uint64& SteamId, const std::string& Groups)
			{

				FString Groups_fstr(Groups.c_str());
				Groups_fstr.ParseIntoArray(FStringArray, L",", true);
				TArray<size_t> GroupArray;
				for (const auto& Group : FStringArray)
				{
					GroupArray.Add(HashString(Group.ToString()));
				}
				PlayerCache.insert(std::map<uint64, TArray<size_t>>::value_type(SteamId, GroupArray));
			};*/
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	virtual void AddPlayer(uint64 steam_id)
	{
		try
		{
			auto& db = LiteDB();
			db << "INSERT INTO Players (SteamId) VALUES (?);" << steam_id;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	virtual bool IsPlayerExists(uint64 steam_id)
	{
		auto& db = LiteDB();

		int count = 0;

		try
		{
			db << "SELECT count(1) FROM Players WHERE SteamId = ?;" << steam_id >> count;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
		return count != 0;
	}

	virtual bool IsGroupExists(const FString& group)
	{
		auto& db = LiteDB();

		int count = 0;

		try
		{
			db << "SELECT count(1) FROM Groups WHERE GroupName = ?;" << group.ToString() >> count;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}
		return count != 0;
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

		try
		{
			auto& db = LiteDB();

			std::string groups_str;
			db << "SELECT Groups FROM Players WHERE SteamId = ?;" << steam_id >> groups_str;

			FString groups_fstr(groups_str.c_str());

			groups_fstr.ParseIntoArray(groups, L",", true);
		}
		catch (const sqlite::sqlite_exception& exception)
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

		try
		{
			auto& db = LiteDB();

			std::string permissions_str;
			db << "SELECT Permissions FROM Groups WHERE GroupName = ?;" << group.ToString() >> permissions_str;

			FString permissions_fstr(permissions_str.c_str());

			permissions_fstr.ParseIntoArray(permissions, L",", true);
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
		return permissions;
	}

	virtual TArray<uint64> GetGroupMembers(const FString& group)
	{
		TArray<uint64> members;

		try
		{
			auto& db = LiteDB();
			db << "SELECT SteamId FROM Players;"
				>> [&](const uint64& SteamId)
			{
				if (IsPlayerInGroup(SteamId, group))
					members.Add(SteamId);
			};
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
		return members;
	}

	virtual std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group)
	{
		if (!IsPlayerExists(steam_id) || !IsGroupExists(group))
			return "Player or group does not exist";

		if (IsPlayerInGroup(steam_id, group))
			return "Player was already added";

		try
		{
			auto& db = LiteDB();
			db << "UPDATE Players SET Groups = Groups || ? || ',' WHERE SteamId = ?;" << group.ToString() << steam_id;
		}
		catch (const sqlite::sqlite_exception& exception)
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
			auto& db = LiteDB();
			db << "UPDATE Players SET Groups = ? WHERE SteamId = ?;" << new_groups.ToString() << steam_id;
		}
		catch (const sqlite::sqlite_exception& exception)
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
			auto& db = LiteDB();
			db << "INSERT INTO Groups (GroupName) VALUES (?);"
				<< group.ToString();
		}
		catch (const sqlite::sqlite_exception& exception)
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
			auto& db = LiteDB();

			db << "DELETE FROM Groups WHERE GroupName = ?;" << group.ToString();
		}
		catch (const sqlite::sqlite_exception& exception)
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
			auto& db = LiteDB();
			db << "UPDATE Groups SET Permissions = Permissions || ? || ',' WHERE GroupName = ?;" << permission.ToString() <<
				group.ToString();
		}
		catch (const sqlite::sqlite_exception& exception)
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
			auto& db = LiteDB();
			db << "UPDATE Groups SET Permissions = ? WHERE GroupName = ?;" << new_permissions.ToString() << group.ToString();
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return "Unexpected DB error";
		}
		return {};
	}
};