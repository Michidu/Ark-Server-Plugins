#include "Hooks.h"

#include "Main.h"

namespace Permissions::Hooks
{
	DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*,
	UPrimalPlayerData*, AShooterCharacter*, bool);
	DECLARE_HOOK(AShooterPlayerController_ClientNotifyAdmin, void, AShooterPlayerController*);

	bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* new_player,
	                                           UPrimalPlayerData* player_data, AShooterCharacter* player_character,
	                                           bool is_from_login)
	{
		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(new_player);

		if (!database->IsPlayerExists(steam_id))
		{
			const bool res = database->AddPlayer(steam_id);
			if (!res)
			{
				Log::GetLog()->error("({} {}) Couldn't add player", __FILE__, __FUNCTION__);
			}
		}

		return AShooterGameMode_HandleNewPlayer_original(_this, new_player, player_data, player_character,
		                                                 is_from_login);
	}

	void Hook_AShooterPlayerController_ClientNotifyAdmin(AShooterPlayerController* player_controller)
	{
		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		if (!IsPlayerInGroup(steam_id, "Admins"))
			database->AddPlayerToGroup(steam_id, "Admins");

		AShooterPlayerController_ClientNotifyAdmin_original(player_controller);
	}

	void Init()
	{
		ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation",
		                           &Hook_AShooterGameMode_HandleNewPlayer,
		                           &AShooterGameMode_HandleNewPlayer_original);

		ArkApi::GetHooks().SetHook("AShooterPlayerController.ClientNotifyAdmin",
		                           &Hook_AShooterPlayerController_ClientNotifyAdmin,
		                           &AShooterPlayerController_ClientNotifyAdmin_original);
	}
}
