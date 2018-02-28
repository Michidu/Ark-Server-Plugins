#include "PlayerRewards.h"

#include <Points.h>
#include "Stats.h"

DECLARE_HOOK(AShooterCharacter_Die, bool, AShooterCharacter*, float, FDamageEvent*, AController*, AActor*);

namespace PlayerRewards
{
	bool Hook_AShooterCharacter_Die(AShooterCharacter* _this, float KillingDamage, FDamageEvent* DamageEvent,
	                                AController* Killer, AActor* DamageCauser)
	{
		if (Killer && !Killer->IsLocalController() && Killer->IsA(AShooterPlayerController::StaticClass()) &&
			_this->TargetingTeamField()() != Killer->TargetingTeamField()() && _this->GetPlayerData())
		{
			FUniqueNetIdSteam* steam_net_id = static_cast<FUniqueNetIdSteam*>(
				_this->GetPlayerData()->MyDataField()()->UniqueIDField()().UniqueNetId.Object);
			const uint64 victim_steam_id = steam_net_id->UniqueNetId;

			const uint64 killer_steam_id = ArkApi::IApiUtils::GetSteamIdFromController(Killer);

			const bool player_rewards_enabled = config["PlayerRewards"]["Enabled"];
			if (player_rewards_enabled)
			{
				const int receive_points = config["PlayerRewards"]["ReceivePoints"];
				const int additional_points_percent = config["PlayerRewards"]["AdditionalPointsPercent"];
				const int lose_points_percent = config["PlayerRewards"]["LosePointsPercent"];

				const int victim_points = ArkShop::Points::GetPoints(victim_steam_id);

				const int final_points = receive_points + victim_points * additional_points_percent / 100;
				const int lose_points = final_points * lose_points_percent / 100;

				if (victim_points > lose_points && ArkShop::Points::AddPoints(final_points, killer_steam_id) &&
					ArkShop::Points::SpendPoints(lose_points, victim_steam_id))
				{
					AShooterPlayerController* victim_controller = ArkApi::GetApiUtils().FindPlayerFromSteamId(victim_steam_id);
					if (victim_controller)
						ArkApi::GetApiUtils().SendChatMessage(victim_controller, GetText("Sender"), *GetText("LostPoints"), lose_points);
				}
			}

			Stats::AddPlayerKill(killer_steam_id);
			Stats::AddPlayerDeath(victim_steam_id);
		}

		return AShooterCharacter_Die_original(_this, KillingDamage, DamageEvent, Killer, DamageCauser);
	}

	void Init()
	{
		ArkApi::GetHooks().SetHook("AShooterCharacter.Die", &Hook_AShooterCharacter_Die,
		                           &AShooterCharacter_Die_original);
	}

	void Unload()
	{
		ArkApi::GetHooks().DisableHook("AShooterCharacter.Die", &Hook_AShooterCharacter_Die);
	}
}
