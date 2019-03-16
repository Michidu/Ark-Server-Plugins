#include "PlayerRewards.h"

#include <Points.h>
#include "Stats.h"

DECLARE_HOOK(AShooterCharacter_Die, bool, AShooterCharacter*, float, FDamageEvent*, AController*, AActor*);

namespace PlayerRewards
{
	bool Hook_AShooterCharacter_Die(AShooterCharacter* _this, float KillingDamage, FDamageEvent* DamageEvent,
	                                AController* Killer, AActor* DamageCauser)
	{
		if (Killer && !Killer->IsLocalController() && Killer->IsA(AShooterPlayerController::GetPrivateStaticClass()) &&
			_this->TargetingTeamField() != Killer->TargetingTeamField() && _this->GetPlayerData())
		{
			FUniqueNetIdSteam* steam_net_id = static_cast<FUniqueNetIdSteam*>(
				_this->GetPlayerData()->MyDataField()->UniqueIDField().UniqueNetId.Get());
			const uint64 victim_steam_id = steam_net_id->UniqueNetId;

			const uint64 killer_steam_id = ArkApi::IApiUtils::GetSteamIdFromController(Killer);

			const bool player_rewards_enabled = config["PlayerRewards"]["Enabled"];
			if (player_rewards_enabled)
			{
				const std::string message_type = config["PlayerRewards"].value("MessageType", "notification");

				const int receive_points = config["PlayerRewards"]["ReceivePoints"];
				if (receive_points < 0)
				{
					const bool negative_points = config["PlayerRewards"].value("NegativePoints", false);
					if (negative_points || ArkShop::Points::GetPoints(killer_steam_id) - abs(receive_points) >= 0)
					{
						ArkShop::Points::SpendPoints(abs(receive_points), killer_steam_id);

						AShooterPlayerController* killer_controller = ArkApi::GetApiUtils().FindPlayerFromSteamId(killer_steam_id);
						if (killer_controller)
						{
							if (message_type == "notification")
							{
								const float display_scale = config["PlayerRewards"].value("NotificationScale", 1.3f);
								const float display_time = config["PlayerRewards"].value("NotificationDisplayTime", 10.0f);

								ArkApi::GetApiUtils().SendNotification(killer_controller, FColorList::Red, display_scale, display_time,
								                                       nullptr, *GetText("LostPoints"), abs(receive_points));
							}
							else if (message_type == "chat")
							{
								ArkApi::GetApiUtils().SendChatMessage(killer_controller, GetText("Sender"), *GetText("LostPoints"),
								                                      abs(receive_points));
							}
						}
					}
				}
				else
				{
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
