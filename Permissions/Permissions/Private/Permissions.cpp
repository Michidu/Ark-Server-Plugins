#ifdef PERMISSIONS_ARK
#include "../Public/ArkPermissions.h"
#else
#include "../Public/AtlasPermissions.h"
#endif

#include "Main.h"

namespace Permissions
{

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
				auto playerId = pc->GetLinkedPlayerID();
				if (Tribemates.Contains(playerId)) {
					tribematesOnline++;
				}
			}
			groups.Add(FString::Format("TribeSize:{}", Tribemates.Num()));
			groups.Add(FString::Format("TribeOnline:{}", tribematesOnline));
		}
		return groups;
	}
	TArray<FString> GetPlayerGroups(uint64 steam_id)
	{
		auto world = ArkApi::GetApiUtils().GetWorld();
		TArray<FString> groups = database->GetPlayerGroups(steam_id);
		auto shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (shooter_pc) {
			auto tribeData = GetTribeData(shooter_pc);
			if (tribeData) {
				auto tribeId = tribeData->TribeIDField();
				auto tribeGroups = GetTribeGroups(tribeId);
				for (auto tribeGroup : tribeGroups) {
					if (!groups.Contains(tribeGroup)) {
						groups.Add(tribeGroup);
					}
				}
				auto defaultTribeGroups = GetTribeDefaultGroups(tribeData);
				for (auto tribeGroup : defaultTribeGroups) {
					if (!groups.Contains(tribeGroup)) {
						groups.Add(tribeGroup);
					}
				}
			}
		}
		return groups;
	}

	TArray<FString> GetTribeGroups(int tribeId)
	{
		return database->GetTribeGroups(tribeId);
	}

	TArray<FString> GetGroupPermissions(const FString& group)
	{
		if (group.IsEmpty())
			return {};
		return database->GetGroupPermissions(group);
	}

	TArray<FString> GetAllGroups()
	{
		return database->GetAllGroups();
	}

	TArray<uint64> GetGroupMembers(const FString& group)
	{
		return database->GetGroupMembers(group);
	}

	bool IsPlayerInGroup(uint64 steam_id, const FString& group)
	{
		TArray<FString> groups = GetPlayerGroups(steam_id);

		for (const auto& current_group : groups)
		{
			if (current_group == group)
				return true;
		}

		return false;
	}

	bool IsTribeInGroup(int tribeId, const FString& group)
	{
		TArray<FString> groups = GetTribeGroups(tribeId);

		for (const auto& current_group : groups)
		{
			if (current_group == group)
				return true;
		}

		return false;
	}

	std::optional<std::string> AddPlayerToGroup(uint64 steam_id, const FString& group)
	{
		return database->AddPlayerToGroup(steam_id, group);
	}

	std::optional<std::string> RemovePlayerFromGroup(uint64 steam_id, const FString& group)
	{
		return database->RemovePlayerFromGroup(steam_id, group);
	}

	std::optional<std::string> AddPlayerToTimedGroup(uint64 steam_id, const FString& group, int secs, int delaySecs)
	{
		return database->AddPlayerToTimedGroup(steam_id, group, secs, delaySecs);
	}

	std::optional<std::string> RemovePlayerFromTimedGroup(uint64 steam_id, const FString& group)
	{
		return database->RemovePlayerFromTimedGroup(steam_id, group);
	}

	std::optional<std::string> AddTribeToGroup(int tribeId, const FString& group)
	{
		return database->AddTribeToGroup(tribeId, group);
	}

	std::optional<std::string> RemoveTribeFromGroup(int tribeId, const FString& group)
	{
		return database->RemoveTribeFromGroup(tribeId, group);
	}

	std::optional<std::string> AddTribeToTimedGroup(int tribeId, const FString& group, int secs, int delaySecs)
	{
		return database->AddTribeToTimedGroup(tribeId, group, secs, delaySecs);
	}

	std::optional<std::string> RemoveTribeFromTimedGroup(int tribeId, const FString& group)
	{
		return database->RemoveTribeFromTimedGroup(tribeId, group);
	}

	std::optional<std::string> AddGroup(const FString& group)
	{
		return database->AddGroup(group);
	}

	std::optional<std::string> RemoveGroup(const FString& group)
	{
		return database->RemoveGroup(group);
	}

	bool IsGroupHasPermission(const FString& group, const FString& permission)
	{
		if (!database->IsGroupExists(group))
			return false;

		TArray<FString> permissions = GetGroupPermissions(group);

		for (const auto& current_perm : permissions)
		{
			if (current_perm == permission)
				return true;
		}

		return false;
	}

	bool IsPlayerHasPermission(uint64 steam_id, const FString& permission)
	{
		TArray<FString> groups = GetPlayerGroups(steam_id);

		for (const auto& current_group : groups)
		{
			if (IsGroupHasPermission(current_group, permission) || IsGroupHasPermission(current_group, "*"))
				return true;
		}

		return false;
	}

	bool IsTribeHasPermission(int tribeId, const FString& permission)
	{
		TArray<FString> groups = GetTribeGroups(tribeId);

		for (const auto& current_group : groups)
		{
			if (IsGroupHasPermission(current_group, permission) || IsGroupHasPermission(current_group, "*"))
				return true;
		}

		return false;
	}

	std::optional<std::string> GroupGrantPermission(const FString& group, const FString& permission)
	{
		return database->GroupGrantPermission(group, permission);
	}

	std::optional<std::string> GroupRevokePermission(const FString& group, const FString& permission)
	{
		return database->GroupRevokePermission(group, permission);
	}
}
