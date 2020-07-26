#include "Hooks.h"

#include "Main.h"

namespace Permissions::Hooks
{
	DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*,
	UPrimalPlayerData*, AShooterCharacter*, bool);
	DECLARE_HOOK(AShooterPlayerController_ClientNotifyAdmin, void, AShooterPlayerController*);
	DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);


	void CheckAdmin(const uint64 steam_id)
	{
		if (!IsPlayerInGroup(steam_id, "Admins"))
			database->AddPlayerToGroup(steam_id, "Admins");
	}

	bool Hook_AShooterGameMode_HandleNewPlayer(AShooterGameMode* _this, AShooterPlayerController* player_controller,
	                                           UPrimalPlayerData* player_data, AShooterCharacter* player_character,
	                                           bool is_from_login)
	{
		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		if (!database->IsPlayerExists(steam_id))
		{
			const bool res = database->AddPlayer(steam_id);
			if (!res)
			{
				Log::GetLog()->error("({} {}) Couldn't add player", __FILE__, __FUNCTION__);
			}

			CheckAdmin(steam_id);
		}

		return AShooterGameMode_HandleNewPlayer_original(_this, player_controller, player_data, player_character,
		                                                 is_from_login);
	}

	void Hook_AShooterPlayerController_ClientNotifyAdmin(AShooterPlayerController* player_controller)
	{		
		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);
		
		CheckAdmin(steam_id);

		AShooterPlayerController_ClientNotifyAdmin_original(player_controller);
	}
	
	void _cdecl Hook_AShooterGameMode_Logout(AShooterGameMode* _this, AController* controller)
	{
		if (use_cache && controller && controller->IsA(AShooterPlayerController::StaticClass()))
		{
			AShooterPlayerController* player_controller = static_cast<AShooterPlayerController*>(controller);
			const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);
			Cache::RemovePlayer(steam_id);
		}
		AShooterGameMode_Logout_original(_this, controller);
	}

	void Init()
	{
		ArkApi::GetHooks().SetHook("AShooterGameMode.HandleNewPlayer_Implementation",
		                           &Hook_AShooterGameMode_HandleNewPlayer,
		                           &AShooterGameMode_HandleNewPlayer_original);

		ArkApi::GetHooks().SetHook("AShooterPlayerController.ClientNotifyAdmin",
		                           &Hook_AShooterPlayerController_ClientNotifyAdmin,
		                           &AShooterPlayerController_ClientNotifyAdmin_original);

		ArkApi::GetHooks().SetHook("AShooterGameMode.Logout",
								   &Hook_AShooterGameMode_Logout,
								   &AShooterGameMode_Logout_original);

	}
}
