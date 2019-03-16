#include "DinoRewards.h"

#include <Points.h>
#include "Stats.h"

DECLARE_HOOK(APrimalDinoCharacter_Die, bool, APrimalDinoCharacter*, float, FDamageEvent*, AController*, AActor*);

namespace DinoRewards
{
	FString GetDinoBlueprint(UObjectBase* dino)
	{
		if (dino && dino->ClassField())
		{
			FString path_name;
			dino->ClassField()->GetDefaultObject(true)->GetFullName(&path_name, nullptr);

			if (int find_index = 0; path_name.FindChar(' ', find_index))
			{
				path_name = "Blueprint'" + path_name.Mid(find_index + 1,
				                                         path_name.Len() - (find_index + (path_name.EndsWith(
					                                                                          "_C", ESearchCase::CaseSensitive)
					                                                                          ? 3
					                                                                          : 1))) + "'";
				return path_name.Replace(L"Default__", L"", ESearchCase::CaseSensitive);
			}
		}

		return FString("");
	}

	nlohmann::basic_json<> GetDinoConfig(const std::string& blueprint)
	{
		auto dinos = config["DinoRewards"]["Dinos"];
		for (const auto& dino_entry : dinos)
		{
			const std::string bp = dino_entry["Blueprint"];
			if (bp == blueprint)
				return dino_entry;
		}

		return nlohmann::json::value_type::object();
	}

	bool Hook_APrimalDinoCharacter_Die(APrimalDinoCharacter* _this, float KillingDamage, FDamageEvent* DamageEvent,
	                                   AController* Killer, AActor* DamageCauser)
	{
		if (Killer && !Killer->IsLocalController() && Killer->IsA(AShooterPlayerController::GetPrivateStaticClass()) &&
			_this->TargetingTeamField() != Killer->TargetingTeamField())
		{
			const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(Killer);

			const bool dino_rewards_enabled = config["DinoRewards"]["Enabled"];
			if (dino_rewards_enabled)
			{
				const bool unclaim_points = config["DinoRewards"].value("GiveUnclaimPoints", false);
				if (!unclaim_points && _this->OwningPlayerIDField() == 0 && _this->TargetingTeamField() >= 50000)
				{
					//
				}
				else
				{
					FString bp = GetDinoBlueprint(_this);

					const auto dino_entry = GetDinoConfig(bp.ToString());
					if (!dino_entry.empty())
					{
						UPrimalCharacterStatusComponent* char_comp = _this->MyCharacterStatusComponentField();
						if (char_comp)
						{
							const int dino_level = char_comp->BaseCharacterLevelField() + char_comp->ExtraCharacterLevelField();

							auto award = dino_entry["Award"];
							for (const auto& award_entry : award)
							{
								const int min_level = award_entry["MinLevel"];
								const int max_level = award_entry["MaxLevel"];

								if (dino_level >= min_level && dino_level <= max_level)
								{
									const bool negative_points = config["PlayerRewards"].value("NegativePoints", false);

									const int points = award_entry["Points"];
									if (points >= 0)
									{
										ArkShop::Points::AddPoints(points, steam_id);
									}
									else if (negative_points || ArkShop::Points::GetPoints(steam_id) - abs(points) >= 0)
									{
										ArkShop::Points::SpendPoints(abs(points), steam_id);

										AShooterPlayerController* killer_controller = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
										if (killer_controller)
										{
											const std::string message_type = config["PlayerRewards"].value("MessageType", "notification");

											if (message_type == "notification")
											{
												const float display_scale = config["PlayerRewards"].value("NotificationScale", 1.3f);
												const float display_time = config["PlayerRewards"].value("NotificationDisplayTime", 10.0f);

												ArkApi::GetApiUtils().SendNotification(killer_controller, FColorList::Red, display_scale, display_time,
												                                       nullptr, *GetText("LostPoints"), abs(points));
											}
											else if (message_type == "chat")
											{
												ArkApi::GetApiUtils().SendChatMessage(killer_controller, GetText("Sender"), *GetText("LostPoints"),
												                                      abs(points));
											}
										}
									}

									break;
								}
							}
						}
					}
				}
			}

			if (_this->TargetingTeamField() < 50000)
				Stats::AddWildDinoKill(steam_id);
			else
				Stats::AddTamedDinoKill(steam_id);
		}

		return APrimalDinoCharacter_Die_original(_this, KillingDamage, DamageEvent, Killer, DamageCauser);
	}

	void Init()
	{
		ArkApi::GetHooks().SetHook("APrimalDinoCharacter.Die", &Hook_APrimalDinoCharacter_Die,
		                           &APrimalDinoCharacter_Die_original);
	}

	void Unload()
	{
		ArkApi::GetHooks().DisableHook("APrimalDinoCharacter.Die", &Hook_APrimalDinoCharacter_Die);
	}
}
