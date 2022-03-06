#include "SafeZoneManager.h"

#include <ArkPermissions.h>

#include "SafeZones.h"

namespace SafeZones
{
	SafeZoneManager& SafeZoneManager::Get()
	{
		static SafeZoneManager instance;
		return instance;
	}

	void SafeZoneManager::ReadSafeZones()
	{
		FString map_name;
		ArkApi::GetApiUtils().GetShooterGameMode()->GetMapName(&map_name);

		const auto& safe_zones = config.value("SafeZones", nlohmann::json::array());
		for (const auto& safe_zone : safe_zones)
		{
			const FString for_map = FString(safe_zone.value("ForMap", ""));
			if (for_map != L""
				&& for_map.Find(map_name) == -1)
			{
				continue;
			}

			auto config_position = safe_zone.value("Position", std::vector<float>{0, 0, 0});
			auto config_success_color = safe_zone.value("SuccessNotificationColor", std::vector<float>{0, 0, 0});
			auto config_fail_color = safe_zone.value("FailNotificationColor", std::vector<float>{0, 0, 0});

			FString name = FString(ArkApi::Tools::Utf8Decode(safe_zone.value("Name", "")));

			FVector position{config_position[0], config_position[1], config_position[2]};
			int radius = safe_zone.value("Radius", 0);
			bool prevent_pvp = safe_zone.value("PreventPVP", false);
			bool prevent_structure_damage = safe_zone.value("PreventStructureDamage", false);
			bool prevent_building = safe_zone.value("PreventBuilding", false);
			bool kill_wild_dinos = safe_zone.value("KillWildDinos", false);
			bool prevent_leaving = safe_zone.value("PreventLeaving", false);
			bool prevent_entering = safe_zone.value("PreventEntering", false);
			const bool prevent_friendly_fire = safe_zone.value("PreventFriendlyFire", false);
			const bool prevent_wild_damage = safe_zone.value("PreventWildDinoDamage", false);

			bool enable_events = safe_zone.value("EnableEvents", false);
			bool screen_notifications = safe_zone.value("ScreenNotifications", false);
			bool chat_notifications = safe_zone.value("ChatNotifications", false);

			bool cryopod_dinos = safe_zone.value("GiveDinosInCryopod", false);
			bool show_bubble = safe_zone.value("ShowBubble", false);

			bool only_kill_aggressive = safe_zone.value("OnlyKillAggressiveDinos", false);

			std::vector<float> bubble_colors = safe_zone.value("BubbleColor", std::vector<float>{1, 0, 0});

			bool enable_teleport = safe_zone.value("TeleportOnEnterZone", nlohmann::json::object()).value("Enabled", false);
			std::vector<float> teleport_destination = safe_zone.value("TeleportOnEnterZone", nlohmann::json::object()).value("TeleportToDestination", std::vector<float>{ 0.f, 0.f, 0.f });

			FLinearColor success_color{
				config_success_color[0], config_success_color[1], config_success_color[2], config_success_color[3]
			};
			FLinearColor fail_color{config_fail_color[0], config_fail_color[1], config_fail_color[2], config_fail_color[3]};

			std::vector<FString> messages;
			for (const auto& msg : safe_zone.value("Messages", nlohmann::json::array()))
			{
				messages.emplace_back(FString(ArkApi::Tools::Utf8Decode(msg)));
			}

			// Add zone name to the default zones list
			default_safezones_.Add(name);

			CreateSafeZone(std::make_shared<SafeZone>(name, position, radius, prevent_pvp, prevent_structure_damage,
			                                          prevent_building, kill_wild_dinos, prevent_leaving, prevent_entering,
			                                          enable_events, screen_notifications, chat_notifications, success_color,
			                                          fail_color, messages, cryopod_dinos, show_bubble, bubble_colors,
													  enable_teleport, teleport_destination, only_kill_aggressive,
													  prevent_friendly_fire, prevent_wild_damage));
		}
	}

	bool SafeZoneManager::CreateSafeZone(const std::shared_ptr<SafeZone>& safe_zone)
	{
		const auto find_zone = FindZoneByName(safe_zone->name);
		if (find_zone)
			return false;

		all_safezones_.Add(safe_zone);

		return true;
	}

	bool SafeZoneManager::RemoveSafeZone(const FString& name)
	{
		const auto find_zone = FindZoneByName(name);
		if (!find_zone)
			return false;


		all_safezones_.RemoveSingle(find_zone);
		default_safezones_.RemoveSingle(name);

		return true;
	}

	ATriggerSphere* SafeZoneManager::SpawnSphere(FVector& location, int radius)
	{
		FActorSpawnParameters spawn_parameters;
		FRotator rotation{ 0, 0, 0 };

		UClass* sphere_class = ATriggerSphere::GetClass();

		if (sphere_class)
		{
			AActor* actor = ArkApi::GetApiUtils().GetWorld()->SpawnActor(sphere_class, &location, &rotation, &spawn_parameters);

			if (!actor)
			{
				return nullptr;
			}

			ATriggerSphere* trigger_sphere = static_cast<ATriggerSphere*>(actor);

			if (!trigger_sphere)
			{
				return nullptr;
			}

			TArray<UActorComponent*> comps;
			trigger_sphere->GetComponents(&comps);

			for (auto* comp : comps)
			{
				if (comp
					&& comp->IsA(UPrimitiveComponent::GetPrivateStaticClass()))
				{
					UPrimitiveComponent* prim = static_cast<UPrimitiveComponent*>(comp);
					prim->SetIsReplicated(true);
					prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					prim->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
				}
			}

			trigger_sphere->bPreventActorStasis() = true;
			auto& box_comp = trigger_sphere->CollisionComponentField();

			USphereComponent* comp = static_cast<USphereComponent*>(box_comp.Object);

			if (comp)
			{
				comp->bGenerateOverlapEvents() = true;
				comp->bForceOverlapEvents() = true;
				comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				comp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
				comp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Overlap);
				comp->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
				comp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Overlap);
				comp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel7, ECollisionResponse::ECR_Overlap);
				comp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel8, ECollisionResponse::ECR_Overlap);
				comp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel25, ECollisionResponse::ECR_Overlap);

				comp->SetSphereRadius((float)radius, true);

				trigger_sphere->UpdateOverlaps(true);

				trigger_sphere->ForceReplicateNow(false, false);
				return trigger_sphere;
			}
			else
			{
				trigger_sphere->Destroy(false, false);
				return nullptr;
			}
		}

		return nullptr;
	}


	bool SafeZoneManager::CanBuild(APlayerController* player, const FVector& location, bool notification)
	{
		const bool admins_ignore = config.value("AdminsIgnoreRestrictions", false);

		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player);
		if (admins_ignore && Permissions::IsPlayerInGroup(steam_id, "Admins"))
			return true;

		for (const auto& safe_zone : all_safezones_)
		{
			if (safe_zone->prevent_building && FVector::Distance(safe_zone->position, location) <= safe_zone->radius)
			{
				if (FString msg = safe_zone->messages[2];
					!msg.IsEmpty() && notification)
				{
					safe_zone->SendNotification(static_cast<AShooterPlayerController*>(player), msg, safe_zone->fail_color);
				}

				return false;
			}
		}

		return true;
	}

	bool SafeZoneManager::CheckActorAction(AActor* actor, int type, AActor* CausedByActor)
	{
		if (!actor
			|| !CausedByActor)
			return false;

		if (actor->ActorHasTag(SZ_IGNORE_TAG))
			return false;

		const uint64 teamid_damaged = actor->TargetingTeamField();
		const uint64 teamid_causer = CausedByActor->TargetingTeamField();

		if (actor
			&& actor->IsA(APrimalCharacter::GetPrivateStaticClass()))
		{
			if (static_cast<APrimalCharacter*>(actor)->IsDead())
			{
				return false;
			}
		}

		bool is_protected = false;

		for (const auto& safe_zone : all_safezones_)
		{
			if (safe_zone->IsOverlappingActor(actor))
			{
				switch (type)
				{
				case 1:
					if (safe_zone->prevent_wild_dino_damage
						&& (teamid_causer < 50000 || teamid_damaged < 50000))
					{
						is_protected = true;
					}
					else if (safe_zone->prevent_friendly_fire
						&& (teamid_damaged == teamid_causer || ArkApi::GetApiUtils().GetShooterGameMode()->AreTribesAllied(teamid_damaged, teamid_causer)))
					{
						is_protected = true;
					}
					else if (safe_zone->prevent_pvp
						&& teamid_damaged != teamid_causer
						&& teamid_damaged >= 50000
						&& teamid_causer >= 50000)
					{
						is_protected = true;
					}
					break;
				case 2:
					is_protected = safe_zone->prevent_structure_damage;
					break;
				}
				break;
			}
		}

		return is_protected;
	}

	std::shared_ptr<SafeZone> SafeZoneManager::FindZoneByName(const FString& name)
	{
		const auto safe_zone = all_safezones_.FindByPredicate([&name](const auto& safe_zone)
		{
			return safe_zone->name == name;
		});

		if (!safe_zone)
			return nullptr;

		return *safe_zone;
	}

	std::shared_ptr<SafeZone> SafeZoneManager::CheckActorOverlap(AActor* _this)
	{
		if (_this)
		{
			auto* zone = all_safezones_.FindByPredicate(
				[_this](const auto& safe_zone) -> bool
				{
					return safe_zone->trigger_sphere.Get() == _this;
				}
			);
			
			if (zone)
				return *zone;
			else
				return nullptr;
		}

		return nullptr;
	}

	void SafeZoneManager::ClearAllTriggerSpheres()
	{
		TArray<AActor*> out;
		UGameplayStatics::GetAllActorsOfClass(ArkApi::GetApiUtils().GetWorld(), AActor::GetPrivateStaticClass(), &out);

		UClass* sphere_class = ATriggerSphere::GetClass();
		for (AActor* actor : out)
		{
			if (actor
				&& actor->IsA(sphere_class))
			{
				if (actor->TargetingTeamField() == SAFEZONES_TEAM)
					actor->Destroy(false, false);
			}
		}

		out.Empty();

		static FString path("Blueprint'/Game/Extinction/CoreBlueprints/HordeCrates/StorageBox_HordeShield.StorageBox_HordeShield'");

		static UClass* bubble_class = UVictoryCore::BPLoadClass(&path);

		UGameplayStatics::GetAllActorsOfClass(ArkApi::GetApiUtils().GetWorld(), bubble_class, &out);

		for (AActor* actor : out)
		{
			if (actor
				&& actor->IsA(bubble_class))
			{
				if (actor->TargetingTeamField() == SAFEZONES_TEAM)
					actor->Destroy(false, false);
			}
		}
	}

	std::shared_ptr<SafeZone> SafeZoneManager::GetNearestZone(const FVector& pos)
	{
		std::shared_ptr<SafeZone> zone = nullptr;

		for (auto& safeZone : all_safezones_)
		{
			if (zone == nullptr)
				zone = safeZone;
			else
			{
				if (FVector::Distance(safeZone->position, pos) < FVector::Distance(zone->position, pos))
				{
					zone = safeZone;
				}
			}
		}

		return zone;
	}

	bool SafeZoneManager::ShouldDoTribeWarCheck()
	{
		return ArkApi::GetApiUtils().GetShooterGameMode()->bServerPVEField();
	}

	bool SafeZoneManager::CheckIfTribesAreInTribeWar(const int Id1, const int Id2)
	{
		for (auto& zone : all_safezones_)
			if (zone
				&& zone->AreTribesInTribeWarCheck(Id1, Id2))
				return true;

		return false;
	}

	TArray<std::shared_ptr<SafeZone>>& SafeZoneManager::GetAllSafeZones()
	{
		return all_safezones_;
	}

	TArray<FString>& SafeZoneManager::GetDefaultSafeZones()
	{
		return default_safezones_;
	}

	// Free function
	ISafeZoneManager& GetSafeZoneManager()
	{
		return SafeZoneManager::Get();
	}
}
