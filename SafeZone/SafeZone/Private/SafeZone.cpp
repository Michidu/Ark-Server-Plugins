#include "../Public/SafeZone.h"

#include <ArkPermissions.h>

#include "SafeZones.h"
#include "SafeZoneManager.h"
#include "Structs.h"
#include "SzTools.h"

namespace SafeZones
{
	SafeZone::SafeZone(FString name, const FVector& position, int radius, bool prevent_pvp, bool prevent_structure_damage,
		bool prevent_building, bool kill_wild_dinos, bool prevent_leaving, bool prevent_entering, bool enable_events,
		bool screen_notifications, bool chat_notifications, const FLinearColor& success_color,
		const FLinearColor& fail_color, std::vector<FString> messages, bool cryopod_dinos, bool show_bubble, std::vector<float> bubble_colors,
		bool bEnableTeleport, const std::vector<float>& teleport_destination, const bool onlyKillAggresiveDinos, const bool prevent_friendly_fire,
		const bool prevent_wild_dino_damage)
		: name(std::move(name)),
		position(position),
		radius(radius),
		prevent_pvp(prevent_pvp),
		prevent_structure_damage(prevent_structure_damage),
		prevent_building(prevent_building),
		kill_wild_dinos(kill_wild_dinos),
		prevent_leaving(prevent_leaving),
		prevent_entering(prevent_entering),
		enable_events(enable_events),
		screen_notifications(screen_notifications),
		chat_notifications(chat_notifications),
		success_color(success_color),
		fail_color(fail_color),
		messages(std::move(messages)),
		cryopod_dinos(cryopod_dinos),
		show_bubble(show_bubble),
		bubble_color(bubble_colors[0], bubble_colors[1], bubble_colors[2]),
		bEnableOnEnterTeleport(bEnableTeleport),
		teleport_destination_on_enter(teleport_destination[0], teleport_destination[1], teleport_destination[2]),
		bOnlyKillAggressiveDinos(onlyKillAggresiveDinos),
		prevent_friendly_fire(prevent_friendly_fire),
		prevent_wild_dino_damage(prevent_wild_dino_damage)
	{

		ATriggerSphere* sphere = SafeZoneManager::Get().SpawnSphere(this->position, this->radius);
		if (sphere != nullptr)
		{
			sphere->TargetingTeamField() = SAFEZONES_TEAM;
			this->trigger_sphere = GetWeakReference(sphere);
			if (this->show_bubble)
			{
				SpawnBubble();
			}
		}
		else
		{
			Log::GetLog()->critical("Failed to initialize zone \"{}\"", API::Tools::Utf8Encode(*name));
		}

	}

	bool SafeZone::IsOverlappingActor(AActor* other) const
	{
		if (other)
		{
			return FVector::Distance(other->RootComponentField()->RelativeLocationField(), position) <= radius;
		}
		return false;
	}

	void SafeZone::SendNotification(AShooterPlayerController* player, const FString& message,
	                                const FLinearColor& color) const
	{
		if (screen_notifications)
		{
			const float display_scale = config.value("NotificationScale", 1.0f);
			const float display_time = config.value("NotificationDisplayTime", 5.0f);

			ArkApi::GetApiUtils().SendNotification(player, color, display_scale, display_time,
			                                       nullptr, *message);
		}

		if (chat_notifications)
		{
			ArkApi::GetApiUtils().SendChatMessage(player, name,
			                                      fmt::format(L"<RichColor Color=\"{0}, {1}, {2}, {3}\">{4}</>", color.R,
			                                                  color.G, color.B, color.A, *message).c_str());
		}
	}

	void SafeZone::OnEnterSafeZone(AActor* other_actor)
	{
		if (!other_actor
			|| !enable_events)
			return;

		AShooterPlayerController* player = nullptr;

		if (other_actor->IsA(AShooterCharacter::GetPrivateStaticClass()))
		{
			player = ArkApi::GetApiUtils().FindControllerFromCharacter(
				static_cast<AShooterCharacter*>(other_actor));
		}
		else if (other_actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
		{
			APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(other_actor);
			if (dino->RiderField().Get() != nullptr)
			{
				player = ArkApi::GetApiUtils().FindControllerFromCharacter(dino->RiderField().Get());
			}
			else if (kill_wild_dinos && dino->TargetingTeamField() < 50000)
			{
				if (bOnlyKillAggressiveDinos)
				{ 
					APrimalDinoAIController* dinoAI = static_cast<APrimalDinoAIController*>(dino->ControllerField());
					if (dinoAI
						&& dinoAI->bUseOverlapTargetCheckField())
					{
						dino->TagsField().Add(SZ_IGNORE_TAG);
						dino->Suicide();
					}
				}
				else
				{
					dino->TagsField().Add(SZ_IGNORE_TAG);
					dino->Suicide();
				}
			}
		}

		if (player)
		{
			if (prevent_entering)
			{
				if (!CanJoinZone(player))
				{
					if (SafeZones::Tools::ShouldSendNotification(other_actor, position, radius))
						SendNotification(player, messages[3], fail_color);
					DoPawnPush((APrimalCharacter*)other_actor, false);
					return;
				}
			}

			if (SafeZones::Tools::ShouldSendNotification(other_actor, position, radius))
				SendNotification(player, FString::Format(*messages[0], *name),
					success_color);

			CheckTeleportForCharacter((APrimalCharacter*)other_actor);
			LogEvent(player, true);
		}

		// Execute callbacks
		for (const auto& callback : on_actor_begin_overlap)
		{
			callback(other_actor);
		}
	}

	void SafeZone::OnLeaveSafeZone(AActor* other_actor)
	{
		if (!other_actor
			|| !enable_events)
			return;

		AShooterPlayerController* player = nullptr;

		if (other_actor->IsA(AShooterCharacter::GetPrivateStaticClass()))
		{
			player = ArkApi::GetApiUtils().FindControllerFromCharacter(
				static_cast<AShooterCharacter*>(other_actor));
		}
		else if (other_actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
		{
			APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(other_actor);
			if (dino->RiderField().Get() != nullptr)
			{
				player = ArkApi::GetApiUtils().FindControllerFromCharacter(dino->RiderField().Get());
			}
		}
		if (player)
		{
			if (prevent_leaving)
			{
				if (SafeZones::Tools::ShouldSendNotification(other_actor, position, radius))
					SendNotification(player, messages[4], fail_color);

				DoPawnPush((APrimalCharacter*)other_actor, true);

				return;
			}

			if (SafeZones::Tools::ShouldSendNotification(other_actor, position, radius))
				SendNotification(player, FString::Format(*messages[1], *name),
					fail_color);

			LogEvent(player, false);
		}


		// Execute callbacks
		for (const auto& callback : on_actor_end_overlap)
		{
			callback(other_actor);
		}
	}

	bool SafeZone::CanJoinZone(AShooterPlayerController* player) const
	{
		bool result = true;

		if (prevent_entering)
		{
			const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player);

			result = Permissions::IsPlayerHasPermission(steam_id, "SafeZones." + name);
		}

		for (const auto& callback : can_join_zone)
		{
			result |= callback(player);
		}

		return result;
	}

	void SafeZone::LogEvent(AShooterPlayerController* actor, bool bIsEnter) const
	{
		if (config.value("LogEnterAndLeave", false))
		{
			if (actor)
			{
				FString pName;
				actor->GetPlayerCharacterName(&pName);
				Log::GetLog()->info("{0} has {2} '{1}'", API::Tools::Utf8Encode(*pName), API::Tools::Utf8Encode(*name),
					bIsEnter ? "entered" : "left");
			}
		}
	}

	TArray<AActor*> SafeZone::GetActorsInsideZone() const
	{
		TArray<AActor*> overlapping;

		auto sphere_ptr = trigger_sphere;
		ATriggerSphere* sphere = sphere_ptr.Get();
		if (sphere
			&& sphere->CollisionComponentField().Object)
		{
			sphere->CollisionComponentField()->GetOverlappingActors(&overlapping, nullptr);
		}

		return overlapping;
	}

	void SafeZones::SafeZone::DoPawnPush(APrimalCharacter* Pawn, bool bIsLeavePrevention)
	{
		ATriggerSphere* Sphere = trigger_sphere.Get();
		if (Sphere
			&& Pawn)
		{
			UCharacterMovementComponent* MovementComp = Pawn->CharacterMovementField().Object;

			const FVector sphere_loc = Sphere->RootComponentField()->RelativeLocationField();
			const FVector pawn_loc = Pawn->RootComponentField()->RelativeLocationField();

			FVector new_velocity;

			if (!bIsLeavePrevention)
				new_velocity = FVector(pawn_loc - sphere_loc);
			else
				new_velocity = FVector(sphere_loc - pawn_loc);

			new_velocity.Normalize();

			MovementComp->VelocityField() = FVector(0, 0, 0);

			FVector tp_loc = pawn_loc +
				(FVector(new_velocity.X, new_velocity.Y, new_velocity.Z)
					* FVector(500, 500, 300));

			if (Pawn->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
			{
				static_cast<APrimalDinoCharacter*>(Pawn)->SetFlight(false, true);
			}
			else if (Pawn->IsA(AShooterCharacter::GetPrivateStaticClass()))
			{
				Pawn->StopJumping();
			}

			Pawn->TeleportTo(&tp_loc, &Pawn->RootComponentField()->RelativeRotationField(), false, true);

			Pawn->TagsField().Add(FName("IgnoreSzNotification", EFindName::FNAME_Add));
			SafeZones::Tools::Timer::Get().DelayExec([](TWeakObjectPtr<APrimalCharacter> PawnPtr)
				{
					int idx = 0;
					TArray<FName>& tags = PawnPtr.Get()->TagsField();
					for (int i = 0; i < tags.Num(); i++)
					{
						FName& tag = tags[i];
						if (tag.ToString() == FString("IgnoreSzNotification"))
						{
							tags.RemoveAt(i);
						}
					}
				}
			, 0, false, GetWeakReference(Pawn));
		}
	}

	void SafeZone::SpawnBubble()
	{
		static FString path("Blueprint'/Game/Extinction/CoreBlueprints/HordeCrates/StorageBox_HordeShield.StorageBox_HordeShield'");
		static UClass* bubble_class = UVictoryCore::BPLoadClass(&path);

		if (!bubble_class)
		{
			Log::GetLog()->error("Failed to spawn bubble for zone {}", name.ToString());
			return;
		}
		
		FRotator rot(0, 0, 180);
		FActorSpawnParameters params;
		params.bDeferBeginPlay = true;

		AActor* actor = ArkApi::GetApiUtils().GetWorld()->SpawnActor(bubble_class, &position, &rot, &params);

		if (!actor)
		{
			Log::GetLog()->error("Failed to spawn bubble for zone {}", name.ToString());
			return;
		}

		APrimalStructureItemContainer* structure = static_cast<APrimalStructureItemContainer*>(actor);

		// Shield color
		UProperty* color_prop = structure->FindProperty(FName("ShieldColor", EFindName::FNAME_Add));
		color_prop->Set(structure, bubble_color);
		structure->MulticastProperty(FName("ShieldColor", EFindName::FNAME_Add));

		structure->BeginPlay();
		structure->bDisableActivationUnderwater() = false;
		structure->SetContainerActive(true);
		structure->bCanBeDamaged() = false;
		structure->bDestroyOnStasis() = false;

		// Vars to disable some stuff of the shield so it doesn't bug
		UProperty* level_prop = structure->FindProperty(FName("CurrentLevel", EFindName::FNAME_Add));
		level_prop->Set(structure, 1);
		
		UProperty* bool_prop = structure->FindProperty(FName("bIntermissionShield", EFindName::FNAME_Add));
		bool_prop->Set(structure, true);

		UFunction* netUpdateFunc = actor->FindFunctionChecked(FName("NetRefreshRadiusScale", EFindName::FNAME_Add));
		if (netUpdateFunc)
		{ 
			int offset = 0;

			if (radius <= 1500)
				offset = 35;
			else if (radius <= 5000)
				offset = 25;
			else if (radius <= 10000)
				offset = 20;
			else
				offset = -10;

			int arg = ((radius / 8) - offset) / 10;

			actor->ProcessEvent(netUpdateFunc, &arg);
		}

		actor->TargetingTeamField() = SAFEZONES_TEAM;
		actor->ForceReplicateNow(false, false);
	}

	void SafeZone::AddTribesPairForTribeWarCheck(const int Id1, const int Id2)
	{
		tribe_war_checks.Add(TPair<int, int>{Id1, Id2});
	}

	void SafeZone::RemoveTribesFromTribeWarPair(const int Id1, const int Id2)
	{
		tribe_war_checks.RemoveAll(
			[&](const TPair<int, int>& pair) -> bool
			{
				const int pair1 = pair.Get<0>();
				const int pair2 = pair.Get<1>();

				return (Id1 == pair1 && Id2 == pair2)
					|| (Id1 == pair2 && Id2 == pair1);
			}
		);
	}

	bool SafeZone::AreTribesInTribeWarCheck(const int Id1, const int Id2)
	{
		return tribe_war_checks.ContainsByPredicate(
			[&](const TPair<int, int>& pair) -> bool
			{
				const int pair1 = pair.Get<0>();
				const int pair2 = pair.Get<1>();

				return (Id1 == pair1 && Id2 == pair2)
					|| (Id1 == pair2 && Id2 == pair1);
			}
		);
	}

	void SafeZone::CheckTeleportForCharacter(APrimalCharacter* character)
	{
		if (bEnableOnEnterTeleport
			&& character)
		{
			if (character->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
			{
				static_cast<APrimalDinoCharacter*>(character)->SetFlight(false, true);
			}

			character->StopJumping();
			character->TeleportTo(&teleport_destination_on_enter, &character->RootComponentField()->RelativeRotationField(), false, true);
		}
	}
}
