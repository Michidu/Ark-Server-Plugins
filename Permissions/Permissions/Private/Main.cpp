#include "Main.h"

#include <fstream>

#include <API/ARK/Ark.h>
#include <API/UE/Math/ColorList.h>

#include "../Public/Permissions.h"
#include "../Public/DBHelper.h"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*,
	AShooterCharacter*, bool);

bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* new_player,
                                           UPrimalPlayerData* player_data, AShooterCharacter* player_character,
                                           bool is_from_login)
{
	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(new_player);

	if (!Permissions::DB::IsPlayerExists(steam_id))
	{
		auto& db = GetDB();

		try
		{
			db << "INSERT INTO Players (SteamId) VALUES (?);"
				<< steam_id;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
		}
	}

	return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character, is_from_login);
}

void AddPlayerToGroup(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		uint64 steam_id;

		const FString group = *parsed[2];

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = Permissions::AddPlayerToGroup(steam_id, group);
		if (result)
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully added player");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't add player");
	}
}

void RemovePlayerFromGroup(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		uint64 steam_id;

		const FString group = *parsed[2];

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = Permissions::RemovePlayerFromGroup(steam_id, group);
		if (result)
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully removed player");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't remove player");
	}
}

void AddGroup(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		const FString group = *parsed[1];

		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = Permissions::AddGroup(group);
		if (result)
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully added group");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't add group");
	}
}

void RemoveGroup(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		const FString group = *parsed[1];

		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = Permissions::RemoveGroup(group);
		if (result)
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully removed group");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't remove group");
	}
}

void GroupGrantPermission(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		const FString group = *parsed[1];
		const FString permission = *parsed[2];

		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = Permissions::GroupGrantPermission(group, permission);
		if (result)
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully granted permission");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't grant permission");
	}
}

void GroupRevokePermission(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		const FString group = *parsed[1];
		const FString permission = *parsed[2];

		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = Permissions::GroupRevokePermission(group, permission);
		if (result)
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully revoked permission");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't revoke permission");
	}
}

void PlayerGroups(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		uint64 steam_id;

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		const FString groups = Permissions::GetPlayerGroups(steam_id);

		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::White, *groups);
	}
}

void GroupPermissions(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		const FString group = *parsed[1];

		const FString permissions = Permissions::GetGroupPermissions(group);

		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::White, *permissions);
	}
}

sqlite::database& GetDB()
{
	static sqlite::database db(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/Permissions/ArkDB.db");
	return db;
}

void Load()
{
	Log::Get().Init("Permission");

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

	ArkApi::GetCommands().AddConsoleCommand("Permissions.Add", &AddPlayerToGroup);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.Remove", &RemovePlayerFromGroup);

	ArkApi::GetCommands().AddConsoleCommand("Permissions.AddGroup", &AddGroup);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.RemoveGroup", &RemoveGroup);

	ArkApi::GetCommands().AddConsoleCommand("Permissions.Grant", &GroupGrantPermission);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.Revoke", &GroupRevokePermission);

	ArkApi::GetCommands().AddConsoleCommand("Permissions.PlayerGroups", &PlayerGroups);
	ArkApi::GetCommands().AddConsoleCommand("Permissions.GroupPermissions", &GroupPermissions);
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
