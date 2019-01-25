#define _CRT_SECURE_NO_WARNINGS

#include "json.hpp"

#include "Database/SqlLiteDB.h"
#include "Database/MysqlDB.h"

#include "Main.h"

#include <fstream>

#ifdef PERMISSIONS_ARK
#include "../Public/ArkPermissions.h"
#else
#include "../Public/AtlasPermissions.h"
#endif

#include "Hooks.h"
#include "Helper.h"

#ifdef PERMISSIONS_ARK
#pragma comment(lib, "ArkApi.lib")
#else
#pragma comment(lib, "AtlasApi.lib")
#endif

namespace Permissions
{
	nlohmann::json config;

	// AddPlayerToGroup

	std::optional<std::string> AddPlayerToGroup(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(2))
			return "Wrong syntax";

		uint64 steam_id;

		const FString group = *parsed[2];

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return "Parsing error";
		}

		return AddPlayerToGroup(steam_id, group);
	}

	void AddPlayerToGroupCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = AddPlayerToGroup(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully added player");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void AddPlayerToGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = AddPlayerToGroup(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully added player");
		else
			SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
	}

	// RemovePlayerFromGroup

	std::optional<std::string> RemovePlayerFromGroup(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(2))
			return "Wrong syntax";

		uint64 steam_id;

		const FString group = *parsed[2];

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return "Parsing error";
		}

		return database->RemovePlayerFromGroup(steam_id, group);
	}

	void RemovePlayerFromGroupCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = RemovePlayerFromGroup(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green,
			                                        "Successfully removed player");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void RemovePlayerFromGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = RemovePlayerFromGroup(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully removed player");
		else
			SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
	}

	// AddGroup

	std::optional<std::string> AddGroupCommand(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(1))
			return "Wrong syntax";

		const FString group = *parsed[1];

		return database->AddGroup(group);
	}

	void AddGroupCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = AddGroupCommand(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully added group");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void AddGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = AddGroupCommand(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully added group");
		else
			SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
	}

	// RemoveGroup

	std::optional<std::string> RemoveGroupCommand(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(1))
			return "Wrong syntax";

		const FString group = *parsed[1];

		return database->RemoveGroup(group);
	}

	void RemoveGroupCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = RemoveGroupCommand(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().
				SendServerMessage(shooter_controller, FColorList::Green, "Successfully removed group");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void RemoveGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = RemoveGroupCommand(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully removed group");
		else
			SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
	}

	// GroupGrantPermission

	std::optional<std::string> GroupGrantPermission(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(2))
			return "Wrong syntax";

		const FString group = *parsed[1];
		const FString permission = *parsed[2];

		return database->GroupGrantPermission(group, permission);
	}

	void GroupGrantPermissionCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = GroupGrantPermission(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green,
			                                        "Successfully granted permission");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void GroupGrantPermissionRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = GroupGrantPermission(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully granted permission");
		else
			SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
	}

	// GroupRevokePermission

	std::optional<std::string> GroupRevokePermission(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(2))
			return "Wrong syntax";

		const FString group = *parsed[1];
		const FString permission = *parsed[2];

		return database->GroupRevokePermission(group, permission);
	}

	void GroupRevokePermissionCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = GroupRevokePermission(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green,
			                                        "Successfully revoked permission");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void GroupRevokePermissionRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = GroupRevokePermission(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully revoked permission");
		else
			SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
	}

	// PlayerGroups

	FString PlayerGroups(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(1))
			return "";

		uint64 steam_id;

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return "";
		}

		TArray<FString> groups = database->GetPlayerGroups(steam_id);

		FString groups_str;

		for (const FString& current_group : groups)
		{
			groups_str += current_group + ",";
		}

		if (!groups_str.IsEmpty())
			groups_str.RemoveAt(groups_str.Len() - 1);

		return groups_str;
	}

	void PlayerGroupsCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const FString result = PlayerGroups(*cmd);
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::White, *result);
	}

	void PlayerGroupsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		const FString result = PlayerGroups(rcon_packet->Body);
		SendRconReply(rcon_connection, rcon_packet->Id, *result);
	}

	// GroupPermissions

	FString GroupPermissions(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(1))
			return "";

		const FString group = *parsed[1];

		TArray<FString> permissions = database->GetGroupPermissions(group);

		FString permissions_str;

		for (const FString& current_perm : permissions)
		{
			permissions_str += current_perm + ",";
		}

		if (!permissions_str.IsEmpty())
			permissions_str.RemoveAt(permissions_str.Len() - 1);

		return permissions_str;
	}

	void GroupPermissionsCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const FString result = GroupPermissions(*cmd);
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::White, *result);
	}

	void GroupPermissionsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		const FString result = GroupPermissions(rcon_packet->Body);
		SendRconReply(rcon_connection, rcon_packet->Id, *result);
	}

	// ListGroups

	FString ListGroups()
	{
		FString groups;

		int i = 1;

		TArray<FString> all_groups = database->GetAllGroups();
		for (const auto& group : all_groups)
		{
			FString permissions;

			TArray<FString> group_permissions = database->GetGroupPermissions(group);
			for (const auto& permission : group_permissions)
			{
				permissions += permission + L"; ";
			}

			groups += FString::Format(L"{0}) {1} - {2}\n", i++, group.ToString(), permissions.ToString());
		}

		return groups;
	}

	void ListGroupsCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const FString result = ListGroups();
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::White, *result);
	}

	void ListGroupsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		const FString result = ListGroups();
		SendRconReply(rcon_connection, rcon_packet->Id, *result);
	}

	// Chat commands

	void ShowMyGroupsChat(AShooterPlayerController* player_controller, FString*, EChatSendMode::Type)
	{
		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		TArray<FString> groups = database->GetPlayerGroups(steam_id);

		FString groups_str;

		for (const FString& current_group : groups)
		{
			groups_str += current_group + ", ";
		}

		if (!groups_str.IsEmpty())
			groups_str.RemoveAt(groups_str.Len() - 2, 2);

		ArkApi::GetApiUtils().SendChatMessage(player_controller, L"Permissions", *groups_str);
	}

	void ReadConfig()
	{
		const std::string config_path = GetConfigPath();
		std::ifstream file{config_path};
		if (!file.is_open())
			throw std::runtime_error("Can't open config.json");

		file >> config;

		file.close();
	}

	void Load()
	{
		Log::Get().Init("Permission");

		try
		{
			ReadConfig();
		}
		catch (const std::exception& error)
		{
			Log::GetLog()->error(error.what());
			throw;
		}

		if (config.value("Database", "sqlite") == "mysql")
		{
			database = std::make_unique<MySql>(config.value("MysqlHost", ""),
			                                   config.value("MysqlUser", ""),
			                                   config.value("MysqlPass", ""),
			                                   config.value("MysqlDB", ""),
			                                   config.value("MysqlPlayersTable", "Players"),
			                                   config.value("MysqlGroupsTable", "PermissionGroups"));
		}
		else
		{
			database = std::make_unique<SqlLite>(config.value("DbPathOverride", ""));
		}

		Hooks::Init();

		ArkApi::GetCommands().AddConsoleCommand("Permissions.Add", &AddPlayerToGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.Remove", &RemovePlayerFromGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.AddGroup", &AddGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.RemoveGroup", &RemoveGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.Grant", &GroupGrantPermissionCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.Revoke", &GroupRevokePermissionCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.PlayerGroups", &PlayerGroupsCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.GroupPermissions", &GroupPermissionsCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.ListGroups", &ListGroupsCmd);

		ArkApi::GetCommands().AddRconCommand("Permissions.Add", &AddPlayerToGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.Remove", &RemovePlayerFromGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.AddGroup", &AddGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.RemoveGroup", &RemoveGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.Grant", &GroupGrantPermissionRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.Revoke", &GroupRevokePermissionRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.PlayerGroups", &PlayerGroupsRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.GroupPermissions", &GroupPermissionsRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.ListGroups", &ListGroupsRcon);

		ArkApi::GetCommands().AddChatCommand("/groups", &ShowMyGroupsChat);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Permissions::Load();
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
