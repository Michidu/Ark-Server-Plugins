#define _CRT_SECURE_NO_WARNINGS

#include "json.hpp"

#include "Database/SqlLiteDB.h"
#include "Database/MysqlDB.h"
#include "thread_pool.hpp"

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

// Manage all async calls
thread_pool pool;

namespace Permissions
{
	nlohmann::json config;
	time_t lastDatabaseSyncTime = time(0);
	int SyncFrequency = 60;

	FTribeData* GetTribeData(AShooterPlayerController* playerController)
	{
		FTribeData* result = nullptr;
		auto playerState = reinterpret_cast<AShooterPlayerState*>(playerController->PlayerStateField());
		if (playerState)
		{
#ifdef PERMISSIONS_ARK
			result = playerState->MyTribeDataField();
#else
			result = playerState->CurrentTribeDataPtrField();
#endif
		}
		return result;
	}

	int GetTribeId(AShooterPlayerController* playerController)
	{
		int tribeId = 0;

		auto playerState = reinterpret_cast<AShooterPlayerState*>(playerController->PlayerStateField());
		if (playerState)
		{
			FTribeData* tribeData = GetTribeData(playerController);
			if (tribeData)
				tribeId = tribeData->TribeIDField();
		}

		return tribeId;
	}

	TArray<FString> GetTribeDefaultGroups(FTribeData* tribeData) {
		TArray<FString> groups;
		if (tribeData) {
			auto world = ArkApi::GetApiUtils().GetWorld();
			auto tribeId = tribeData->TribeIDField();
			auto Tribemates = tribeData->MembersPlayerDataIDField();
			const auto& player_controllers = world->PlayerControllerListField();
			int tribematesOnline = 0;
			for (TWeakObjectPtr<APlayerController> player_controller : player_controllers)
			{
				AShooterPlayerController* pc = static_cast<AShooterPlayerController*>(player_controller.Get());
				if (pc)
				{
					auto playerId = pc->GetLinkedPlayerID();
					if (Tribemates.Contains(playerId)) {
						tribematesOnline++;
					}
				}
			}
			groups.Add(FString::Format("TribeSize:{}", Tribemates.Num()));
			groups.Add(FString::Format("TribeOnline:{}", tribematesOnline));
		}
		return groups;
	}

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

	// AddPlayerToTimedGroup
	std::optional<std::string> AddPlayerToTimedGroup(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(3))
			return "Wrong syntax, Should be AddPlayerToTimedGroup steamId hours delayHours";

		long secs = 0;
		long delaySecs = 0;
		uint64 steam_id;

		const FString group = *parsed[2];

		try
		{
			steam_id = std::stoull(*parsed[1]);
			secs = std::stof(*parsed[3]) * 3600;
			if (parsed.IsValidIndex(4)) {
				delaySecs = std::stof(*parsed[4]) * 3600;
				secs += delaySecs;
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return "Parsing error";
		}
		if (secs < 0 || delaySecs < 0) {
			return "Wrong syntax, Should be AddPlayerToTimedGroup steamId hours delayHours";
		}

		return AddPlayerToTimedGroup(steam_id, group, secs, delaySecs);
	}

	void AddPlayerToTimedGroupCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = AddPlayerToTimedGroup(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully added player to timed group.");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void AddPlayerToTimedGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = AddPlayerToTimedGroup(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully added player to timed group.");
		else
			SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
	}

	// RemovePlayerFromTimedGroup
	std::optional<std::string> RemovePlayerFromTimedGroup(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(2))
			return "Wrong syntax";

		uint64 steam_id = 0;

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

		return database->RemovePlayerFromTimedGroup(steam_id, group);
	}

	void RemovePlayerFromTimedGroupCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = RemovePlayerFromTimedGroup(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully removed player from timed group.");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void RemovePlayerFromTimedGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = RemovePlayerFromTimedGroup(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully aremoved player from timed group.");
		else
			SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
	}

	// AddTribeToGroup
	std::optional<std::string> AddTribeToGroup(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(2))
			return "Wrong syntax";

		int tribe_id;

		const FString group = *parsed[2];

		try
		{
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return "Parsing error";
		}

		return AddTribeToGroup(tribe_id, group);
	}

	void AddTribeToGroupCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = AddTribeToGroup(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully added tribe");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void AddTribeToGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = AddTribeToGroup(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully added tribe");
		else
			SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
	}

	// RemoveTribeFromGroup
	std::optional<std::string> RemoveTribeFromGroup(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(2))
			return "Wrong syntax";

		int tribe_id;

		const FString group = *parsed[2];

		try
		{
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return "Parsing error";
		}

		return database->RemoveTribeFromGroup(tribe_id, group);
	}

	void RemoveTribeFromGroupCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = RemoveTribeFromGroup(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green,
				"Successfully removed tribe");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void RemoveTribeFromGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = RemoveTribeFromGroup(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully removed tribe");
		else
			SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
	}

	// AddTribeToTimedGroup
	std::optional<std::string> AddTribeToTimedGroup(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(3))
			return "Wrong syntax, Should be AddTribeToTimedGroup tribeId hours delayHours";

		long secs = 0;
		long delaySecs = 0;
		int tribe_id;

		const FString group = *parsed[2];

		try
		{
			tribe_id = std::stoull(*parsed[1]);
			secs = std::stof(*parsed[3]) * 3600;
			if (parsed.IsValidIndex(4)) {
				delaySecs = std::stof(*parsed[4]) * 3600;
				secs += delaySecs;
			}
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return "Parsing error";
		}
		if (secs < 0 || delaySecs < 0) {
			return "Wrong syntax, Should be AddTribeToTimedGroup tribeId hours delayHours";
		}

		return AddTribeToTimedGroup(tribe_id, group, secs, delaySecs);
	}

	void AddTribeToTimedGroupCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = AddTribeToTimedGroup(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully added tribe to timed group.");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void AddTribeToTimedGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = AddTribeToTimedGroup(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully added tribe to timed group.");
		else
			SendRconReply(rcon_connection, rcon_packet->Id, result.value().c_str());
	}

	// RemoveTribeFromTimedGroup
	std::optional<std::string> RemoveTribeFromTimedGroup(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(2))
			return "Wrong syntax";

		int tribe_id = 0;

		const FString group = *parsed[2];

		try
		{
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return "Parsing error";
		}

		return database->RemoveTribeFromTimedGroup(tribe_id, group);
	}

	void RemoveTribeFromTimedGroupCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		auto result = RemoveTribeFromTimedGroup(*cmd);
		if (!result.has_value())
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully removed tribe from timed group.");
		else
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, result.value().c_str());
	}

	void RemoveTribeFromTimedGroupRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		auto result = RemoveTribeFromTimedGroup(rcon_packet->Body);
		if (!result.has_value())
			SendRconReply(rcon_connection, rcon_packet->Id, "Successfully aremoved tribe from timed group.");
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
	std::string getTimeLeft(int secs, int intervalsToShow) {
		int days, hours, mins;
		std::string timeLeft = "";
		int secsLeft = secs;
		int shown = 0;
		if (secsLeft > 0) {
			days = secsLeft / 86400;
			if (days > 0 && intervalsToShow > shown) {
				if (timeLeft.size() > 0) timeLeft += ", ";
				timeLeft += fmt::format("{} Day{}", (int)days, (days >= 1 ? "s" : ""));
				secsLeft -= days * 86400;
				shown++;
			}
			hours = secsLeft / 3600;
			if (hours > 0 && intervalsToShow > shown) {
				if (timeLeft.size() > 0) timeLeft += ", ";
				timeLeft += fmt::format("{} Hr{}", (int)hours, (hours >= 1 ? "s" : ""));
				secsLeft -= hours * 3600;
				shown++;
			}
			mins = secsLeft / 60;
			if (mins > 0 && intervalsToShow > shown) {
				if (timeLeft.size() > 0) timeLeft += ", ";
				timeLeft += fmt::format("{} Min{}", (int)mins, (mins >= 1 ? "s" : ""));
				secsLeft -= mins * 60;
				shown++;
			}
			if (secsLeft > 0 && intervalsToShow > shown) {
				if (timeLeft.size() > 0) timeLeft += ", ";
				timeLeft += fmt::format("{} Sec{}", (int)secsLeft, (secsLeft >= 1 ? "s" : ""));
				shown++;
			}
		}
		return timeLeft;
	}

	FString GetTribeGroupsStr(FString tribeDefaults, int tribe_id, bool forChat) {
		if (!database->IsTribeExists(tribe_id))
			return "";

		CachedPermission permissions = database->HydrateTribeGroups(tribe_id);

		FString groups_str = tribeDefaults;
		for (int32 Index = 0; Index != permissions.Groups.Num(); ++Index) {
			if (groups_str.Len() > 0) groups_str += ", ";
			groups_str += permissions.Groups[Index];
		}
		auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		for (int32 Index = 0; Index != permissions.TimedGroups.Num(); ++Index) {
			const TimedGroup& current_group = permissions.TimedGroups[Index];
			if (current_group.ExpireAtTime <= nowSecs) continue;
			if (groups_str.Len() > 0)
				groups_str += "\n";
			groups_str += current_group.GroupName;
			if (current_group.DelayUntilTime > 0 && current_group.DelayUntilTime > nowSecs) {
				auto diff = current_group.DelayUntilTime - nowSecs;
				groups_str += FString::Format(" - Activates in {}", getTimeLeft(diff, 2));
			}
			else if (current_group.ExpireAtTime > nowSecs) {
				auto diff = current_group.ExpireAtTime - nowSecs;
				groups_str += FString::Format(" - Ends in {}", getTimeLeft(diff, 2));
			}
		}
		if (groups_str.Len() > 0) {
			if (forChat) {
				groups_str = FString::Format("<RichColor Color=\"0.91, 0.85 , 0.09, 1\">Tribe Permissions: </>{}", groups_str.ToString());
			}
			else {
				groups_str = FString::Format("Tribe Permissions: {}", groups_str.ToString());
			}
		}
		return groups_str;
	}

	FString GetPlayerGroupsStr(uint64 steam_id, bool forChat) {
		if (!database->IsPlayerExists(steam_id))
			return "";

		CachedPermission permissions = database->HydratePlayerGroups(steam_id);

		FString groups_str;
		for (const FString& current_group : permissions.Groups)
		{
			if (groups_str.Len() > 0) groups_str += ", ";
			groups_str += current_group;
		}
		auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		for (const TimedGroup& current_group : permissions.TimedGroups)
		{
			if (current_group.ExpireAtTime < nowSecs) continue;
			if (groups_str.Len() > 0)
				groups_str += "\n";
			groups_str += current_group.GroupName;
			if (current_group.DelayUntilTime > 0 && current_group.DelayUntilTime > nowSecs) {
				auto diff = current_group.DelayUntilTime - nowSecs;
				groups_str += FString::Format(" - Activates in {}", getTimeLeft(diff, 2));
			}
			else if (current_group.ExpireAtTime > nowSecs) {
				auto diff = current_group.ExpireAtTime - nowSecs;
				groups_str += FString::Format(" - Ends in {}", getTimeLeft(diff, 2));
			}
		}

		auto world = ArkApi::GetApiUtils().GetWorld();
		if (world) {
			const auto& player_controllers = world->PlayerControllerListField();
			for (TWeakObjectPtr<APlayerController> player_controller : player_controllers)
			{
				AShooterPlayerController* shooter_pc = static_cast<AShooterPlayerController*>(player_controller.Get());

				auto tribeData = Permissions::GetTribeData(shooter_pc);
				if (tribeData) {
					auto tribeId = tribeData->TribeIDField();
					if (tribeId > 0) {
						auto defaultTribeGroups = GetTribeDefaultGroups(tribeData);
						FString defaults = "";
						for (auto tribeGroup : defaultTribeGroups) {
							if (defaults.Len() > 0) defaults += ", ";
							defaults += tribeGroup;
						}
						FString tribeStr = GetTribeGroupsStr(defaults, tribeId, forChat);
						if (groups_str.Len() > 0 && tribeStr.Len() > 0)
							groups_str += "\n";
						groups_str += tribeStr;
					}
				}
			}
		}

		return groups_str;
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

		return GetPlayerGroupsStr(steam_id, false);
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

	// TribeGroups
	FString TribeGroups(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (!parsed.IsValidIndex(1))
			return "";

		int tribe_id;

		try
		{
			tribe_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return "";
		}

		return GetTribeGroupsStr("", tribe_id, false);
	}

	void TribeGroupsCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const FString result = TribeGroups(*cmd);
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::White, *result);
	}

	void TribeGroupsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		const FString result = TribeGroups(rcon_packet->Body);
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

		FString groups_str = GetPlayerGroupsStr(steam_id, true);

		ArkApi::GetApiUtils().SendChatMessage(player_controller, L"Permissions", *groups_str);
	}

	void DatabaseSync()
	{
		if (difftime(time(0), lastDatabaseSyncTime) >= SyncFrequency)
		{
			pool.push_task(
				[]() { database->Init(); }
			);

			lastDatabaseSyncTime = time(0);
		}
	}

	void ReadConfig()
	{
		const std::string config_path = GetConfigPath();
		std::ifstream file{ config_path };
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
			SyncFrequency = config.value("ClusterSyncTime", 60);
			if (SyncFrequency < 20)
			{
				SyncFrequency = 20;
			}
		}
		catch (const std::exception& error)
		{
			Log::GetLog()->error(error.what());
			throw;
		}

		if (config.value("Database", "sqlite") == "mysql")
		{
			database = std::make_unique<MySql>(
				config.value("MysqlHost", ""),
				config.value("MysqlUser", ""),
				config.value("MysqlPass", ""),
				config.value("MysqlDB", ""),
				config.value("MysqlPort", 3306),
				config.value("MysqlPlayersTable", "Players"),
				config.value("MysqlGroupsTable", "PermissionGroups"),
				config.value("MysqlTribesTable", "TribePermissions"));
		}
		else
		{
			database = std::make_unique<SqlLite>(config.value("DbPathOverride", ""));
		}

		database->Init();
		lastDatabaseSyncTime = time(0);

		Hooks::Init();

		ArkApi::GetCommands().AddConsoleCommand("Permissions.Add", &AddPlayerToGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.Remove", &RemovePlayerFromGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.RemoveTimed", &RemovePlayerFromTimedGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.AddTimed", &AddPlayerToTimedGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.AddGroup", &AddGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.RemoveGroup", &RemoveGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.Grant", &GroupGrantPermissionCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.Revoke", &GroupRevokePermissionCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.PlayerGroups", &PlayerGroupsCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.GroupPermissions", &GroupPermissionsCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.ListGroups", &ListGroupsCmd);

		ArkApi::GetCommands().AddConsoleCommand("Permissions.AddTribe", &AddTribeToGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.RemoveTribe", &RemoveTribeFromGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.RemoveTribeTimed", &RemoveTribeFromTimedGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.AddTribeTimed", &AddTribeToTimedGroupCmd);
		ArkApi::GetCommands().AddConsoleCommand("Permissions.TribeGroups", &TribeGroupsCmd);

		ArkApi::GetCommands().AddRconCommand("Permissions.Add", &AddPlayerToGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.Remove", &RemovePlayerFromGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.AddTimed", &AddPlayerToTimedGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.RemoveTimed", &RemovePlayerFromTimedGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.AddGroup", &AddGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.RemoveGroup", &RemoveGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.Grant", &GroupGrantPermissionRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.Revoke", &GroupRevokePermissionRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.PlayerGroups", &PlayerGroupsRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.GroupPermissions", &GroupPermissionsRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.ListGroups", &ListGroupsRcon);

		ArkApi::GetCommands().AddRconCommand("Permissions.AddTribe", &AddTribeToGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.RemoveTribe", &RemoveTribeFromGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.AddTribeTimed", &AddTribeToTimedGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.RemoveTribeTimed", &RemoveTribeFromTimedGroupRcon);
		ArkApi::GetCommands().AddRconCommand("Permissions.TribeGroups", &TribeGroupsRcon);

		ArkApi::GetCommands().AddChatCommand("/groups", &ShowMyGroupsChat);

		ArkApi::GetCommands().AddOnTimerCallback("DatabaseSync", &DatabaseSync);

		pool.sleep_duration = 20000; // "if not set, default is 1ms which is overkill and will increase cpu usage a lot" - @Lethal 2021
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