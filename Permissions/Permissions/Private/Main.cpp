#include "Main.h"

#include <fstream>

#include <API/ARK/Ark.h>
#include <API/UE/Math/ColorList.h>

#include "../Public/Permissions.h"
#include "../Public/DBHelper.h"

#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*,
	AShooterCharacter*, bool);
DECLARE_HOOK(AShooterPlayerController_ClientNotifyAdmin, void, AShooterPlayerController*);

nlohmann::json config;

bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* new_player,
                                           UPrimalPlayerData* player_data, AShooterCharacter* player_character,
                                           bool is_from_login)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(new_player);

	if (!Permissions::DB::IsPlayerExists(steam_id))
	{
		try
		{
			auto& db = GetDB();

			db << "INSERT INTO Players (SteamId) VALUES (?);"
				<< steam_id;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
}

void Hook_AShooterPlayerController_ClientNotifyAdmin(AShooterPlayerController* player_controller)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

	if (!Permissions::IsPlayerInGroup(steam_id, "Admins"))
		Permissions::AddPlayerToGroup(steam_id, "Admins");

	AShooterPlayerController_ClientNotifyAdmin_original(player_controller);
}

void SendRconReply(RCONClientConnection* rcon_connection, int packet_id, const FString& msg)
{
	FString reply = msg + "\n";
	rcon_connection->SendMessageW(packet_id, 0, &reply);
}

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

	return Permissions::AddPlayerToGroup(steam_id, group);
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

	return Permissions::RemovePlayerFromGroup(steam_id, group);
}

void RemovePlayerFromGroupCmd(APlayerController* player_controller, FString* cmd, bool)
{
	const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	auto result = RemovePlayerFromGroup(*cmd);
	if (!result.has_value())
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully removed player");
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

std::optional<std::string> AddGroup(const FString& cmd)
{
	TArray<FString> parsed;
	cmd.ParseIntoArray(parsed, L" ", true);

	if (!parsed.IsValidIndex(1))
		return "Wrong syntax";

	const FString group = *parsed[1];

	return Permissions::AddGroup(group);
}

void AddGroupCmd(APlayerController* player_controller, FString* cmd, bool)
{
	const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	auto result = AddGroup(*cmd);
	if (!result.has_value())
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully added group");
	else
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
}

void AddGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	auto result = AddGroup(rcon_packet->Body);
	if (!result.has_value())
		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully added group");
	else
		SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
}

std::optional<std::string> RemoveGroup(const FString& cmd)
{
	TArray<FString> parsed;
	cmd.ParseIntoArray(parsed, L" ", true);

	if (!parsed.IsValidIndex(1))
		return "Wrong syntax";

	const FString group = *parsed[1];

	return Permissions::RemoveGroup(group);
}

void RemoveGroupCmd(APlayerController* player_controller, FString* cmd, bool)
{
	const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	auto result = RemoveGroup(*cmd);
	if (!result.has_value())
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully removed group");
	else
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
}

void RemoveGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	auto result = RemoveGroup(rcon_packet->Body);
	if (!result.has_value())
		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully removed group");
	else
		SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
}

std::optional<std::string> GroupGrantPermission(const FString& cmd)
{
	TArray<FString> parsed;
	cmd.ParseIntoArray(parsed, L" ", true);

	if (!parsed.IsValidIndex(2))
		return "Wrong syntax";

	const FString group = *parsed[1];
	const FString permission = *parsed[2];

	return Permissions::GroupGrantPermission(group, permission);
}

void GroupGrantPermissionCmd(APlayerController* player_controller, FString* cmd, bool)
{
	const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	auto result = GroupGrantPermission(*cmd);
	if (!result.has_value())
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully granted permission");
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

std::optional<std::string> GroupRevokePermission(const FString& cmd)
{
	TArray<FString> parsed;
	cmd.ParseIntoArray(parsed, L" ", true);

	if (!parsed.IsValidIndex(2))
		return "Wrong syntax";

	const FString group = *parsed[1];
	const FString permission = *parsed[2];

	return Permissions::GroupRevokePermission(group, permission);
}

void GroupRevokePermissionCmd(APlayerController* player_controller, FString* cmd, bool)
{
	const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	auto result = GroupRevokePermission(*cmd);
	if (!result.has_value())
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully revoked permission");
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

	TArray<FString> groups = Permissions::GetPlayerGroups(steam_id);

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

FString GroupPermissions(const FString& cmd)
{
	TArray<FString> parsed;
	cmd.ParseIntoArray(parsed, L" ", true);

	if (!parsed.IsValidIndex(1))
		return "";

	const FString group = *parsed[1];

	TArray<FString> permissions = Permissions::GetGroupPermissions(group);

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

void ShowMyGroupsChat(AShooterPlayerController* player_controller, FString*, EChatSendMode::Type)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

	TArray<FString> groups = Permissions::GetPlayerGroups(steam_id);

	FString groups_str;

	for (const FString& current_group : groups)
	{
		groups_str += current_group + ", ";
	}

	if (!groups_str.IsEmpty())
		groups_str.RemoveAt(groups_str.Len() - 2, 2);

	ArkApi::GetApiUtils().SendChatMessage(player_controller, L"Permissions", *groups_str);
}

sqlite::database& GetDB()
{
	static sqlite::database db(config.value("DbPathOverride", "").empty()
		                           ? ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/Permissions/ArkDB.db"
		                           : config.value("DbPathOverride", ""));

	return db;
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/Permissions/config.json";
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

	auto& db = GetDB();

	db << "create table if not exists Players ("
		"Id integer primary key autoincrement not null,"
		"SteamId integer default 0,"
		"Groups text default 'Default,'"
		");";
	db << "create table if not exists Groups ("
		"Id integer primary key autoincrement not null,"
		"GroupName text not null,"
		"Permissions text default ''"
		");";

	// Add default groups

	db << "INSERT INTO Groups(GroupName, Permissions)"
		"SELECT 'Admins', '*,'"
		"WHERE NOT EXISTS(SELECT 1 FROM Groups WHERE GroupName = 'Admins');";
	db << "INSERT INTO Groups(GroupName)"
		"SELECT 'Default'"
		"WHERE NOT EXISTS(SELECT 1 FROM Groups WHERE GroupName = 'Default');";

	ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation", &Hook_AShooterGameMode_HandleNewPlayer,
	                           &AShooterGameMode_HandleNewPlayer_original);
	ArkApi::GetHooks().SetHook("AShooterPlayerController.ClientNotifyAdmin", &Hook_AShooterPlayerController_ClientNotifyAdmin,
	                           &AShooterPlayerController_ClientNotifyAdmin_original);

	ArkApi::GetCommands().AddConsoleCommand("Permissions.Add", &AddPlayerToGroupCmd);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.Remove", &RemovePlayerFromGroupCmd);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.AddGroup", &AddGroupCmd);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.RemoveGroup", &RemoveGroupCmd);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.Grant", &GroupGrantPermissionCmd);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.Revoke", &GroupRevokePermissionCmd);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.PlayerGroups", &PlayerGroupsCmd);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.GroupPermissions", &GroupPermissionsCmd);

	ArkApi::GetCommands().AddRconCommand("Permissions.Add", &AddPlayerToGroupRcon);
	ArkApi::GetCommands().AddRconCommand("Permissions.Remove", &RemovePlayerFromGroupRcon);
	ArkApi::GetCommands().AddRconCommand("Permissions.AddGroup", &AddGroupRcon);
	ArkApi::GetCommands().AddRconCommand("Permissions.RemoveGroup", &RemoveGroupRcon);
	ArkApi::GetCommands().AddRconCommand("Permissions.Grant", &GroupGrantPermissionRcon);
	ArkApi::GetCommands().AddRconCommand("Permissions.Revoke", &GroupRevokePermissionRcon);
	ArkApi::GetCommands().AddRconCommand("Permissions.PlayerGroups", &PlayerGroupsRcon);
	ArkApi::GetCommands().AddRconCommand("Permissions.GroupPermissions", &GroupPermissionsRcon);

	ArkApi::GetCommands().AddChatCommand("/groups", &ShowMyGroupsChat);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
