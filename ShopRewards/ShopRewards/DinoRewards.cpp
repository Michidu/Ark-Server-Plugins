#include "DinoRewards.h"

#include <Points.h>
#include "Stats.h"

DECLARE_HOOK(APrimalDinoCharacter_Die, bool, APrimalDinoCharacter*, float, FDamageEvent*, AController*, AActor*);

namespace DinoRewards
{
	FString GetDinoBlueprint(UObjectBase* dino)
	{
		if (dino && dino->ClassField()())
		{
			FString path_name;
			dino->ClassField()()->GetDefaultObject(true)->GetFullName(&path_name, nullptr);

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
		if (Killer && !Killer->IsLocalController() && Killer->IsA(AShooterPlayerController::StaticClass()) &&
			_this->TargetingTeamField()() != Killer->TargetingTeamField()())
		{
			const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(Killer);

			const bool dino_rewards_enabled = config["DinoRewards"]["Enabled"];
			if (dino_rewards_enabled)
			{
				FString bp = GetDinoBlueprint(_this);

				const auto dino_entry = GetDinoConfig(bp.ToString());
				if (!dino_entry.empty())
				{
					UPrimalCharacterStatusComponent* char_comp = _this->MyCharacterStatusComponentField()();
					if (char_comp)
					{
						const int dino_level = char_comp->BaseCharacterLevelField()() + char_comp->ExtraCharacterLevelField()();

						auto award = dino_entry["Award"];
						for (const auto& award_entry : award)
						{
							const int min_level = award_entry["MinLevel"];
							const int max_level = award_entry["MaxLevel"];

							if (dino_level >= min_level && dino_level <= max_level)
							{
								const int points = award_entry["Points"];

								ArkShop::Points::AddPoints(points, steam_id);
								break;
							}
						}
					}
				}
			}

			if (_this->TargetingTeamField()() < 50000)
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
