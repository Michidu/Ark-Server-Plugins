#include "Hooks.h"

#include "SafeZoneManager.h"
#include "SafeZones.h"
#include "SzTools.h"


namespace SafeZones::Hooks
{
	DECLARE_HOOK(AShooterGameMode_InitGame, void, AShooterGameMode*, FString*, FString*, FString*);

	DECLARE_HOOK(APrimalStructure_IsAllowedToBuild, int, APrimalStructure*, APlayerController*, FVector, FRotator,
		FPlacementData*, bool, FRotator, bool);
	DECLARE_HOOK(APrimalCharacter_TakeDamage, float, APrimalCharacter*, float, FDamageEvent*, AController*, AActor*);
	DECLARE_HOOK(APrimalStructure_TakeDamage, float, APrimalStructure*, float, FDamageEvent*, AController*, AActor*);
	DECLARE_HOOK(APrimalDinoCharacter_CanCarryCharacter, bool, APrimalDinoCharacter*, APrimalCharacter*);

	DECLARE_HOOK(AShooterPlayerController_ServerRequestRespawnAtPoint_Impl, void, AShooterPlayerController *, int , int);
	DECLARE_HOOK(AShooterPlayerState_ServerRequestCreateNewPlayer_Impl, void, AShooterPlayerState *, DWORD64);

	DECLARE_HOOK(AActor_ReceiveActorBeginOverlap, void, AActor*, AActor*);
	DECLARE_HOOK(AActor_ReceiveActorEndOverlap, void, AActor*, AActor*);

	void Hook_AShooterGameMode_InitGame(AShooterGameMode* a_shooter_game_mode, FString* map_name, FString* options,
	                                    FString* error_message)
	{
		AShooterGameMode_InitGame_original(a_shooter_game_mode, map_name, options, error_message);

		SafeZones::Tools::Timer::Get().DelayExec(
			[]()
			{
				SafeZoneManager::Get().ClearAllTriggerSpheres();
				SafeZoneManager::Get().ReadSafeZones();
			},
			1, false);
	}

	int Hook_APrimalStructure_IsAllowedToBuild(APrimalStructure* _this, APlayerController* PC, FVector AtLocation,
	                                           FRotator AtRotation, FPlacementData* OutPlacementData,
	                                           bool bDontAdjustForMaxRange, FRotator PlayerViewRotation,
	                                           bool bFinalPlacement)
	{
		if (bFinalPlacement 
			&& PC 
			&& !SafeZoneManager::Get().CanBuild(PC, AtLocation, true))
			return 0;

		return APrimalStructure_IsAllowedToBuild_original(_this, PC, AtLocation, AtRotation, OutPlacementData,
		                                                  bDontAdjustForMaxRange, PlayerViewRotation, bFinalPlacement);
	}

	float Hook_APrimalCharacter_TakeDamage(APrimalCharacter* _this, float Damage, FDamageEvent* DamageEvent,
	                                       AController* EventInstigator, AActor* DamageCauser)
	{
		if (_this->IsA(AShooterCharacter::GetPrivateStaticClass()) &&
			SafeZoneManager::Get().CheckActorAction(_this, 1))
		{
			return 0;
		}
		if (_this->IsA(APrimalDinoCharacter::GetPrivateStaticClass()) &&
			_this->TargetingTeamField() > 50000 &&
			SafeZoneManager::Get().CheckActorAction(_this, 1))
		{
			return 0;
		}
		if (EventInstigator && EventInstigator->CharacterField() &&
			SafeZoneManager::Get().CheckActorAction(EventInstigator->CharacterField(), 1))
		{
			return 0;
		}

		return APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
	}

	float Hook_APrimalStructure_TakeDamage(APrimalStructure* _this, float Damage, FDamageEvent* DamageEvent,
	                                       AController* EventInstigator, AActor* DamageCauser)
	{
		return SafeZoneManager::Get().CheckActorAction(_this, 2)
			       ? 0
			       : APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
	}

	bool Hook_APrimalDinoCharacter_CanCarryCharacter(APrimalDinoCharacter* _this, APrimalCharacter* CanCarryPawn)
	{
		return SafeZoneManager::Get().CheckActorAction(_this, 1)
			       ? false
			       : APrimalDinoCharacter_CanCarryCharacter_original(_this, CanCarryPawn);
	}

	void TeleportPlayer(AShooterPlayerController* pc)
	{
		if (!pc
			|| !pc->CharacterField())
		{
			SafeZones::Tools::Timer::Get().DelayExec(&TeleportPlayer, 1, false, pc);
			return;
		}

		try
		{
			auto spawn_points = config.value("OverrideSpawnPoint", nlohmann::json::array());
			if (!spawn_points.empty())
			{
				const auto num = static_cast<size_t>(Tools::GetRandomNumber(0, static_cast<int>(spawn_points.size()) - 1));

				auto config_position = spawn_points[num];

				if (config_position.size() >= 3)
					pc->SetPlayerPos(config_position[0].get<float>(), config_position[1].get<float>(), config_position[2].get<float>());
			}
		}
		catch (const std::exception& ex)
		{
			Log::GetLog()->error("Error: {} [{}:{}]", ex.what(), __FUNCTION__, __LINE__);
		}
	}

	void Hook_AShooterPlayerController_ServerRequestRespawnAtPoint_Impl(AShooterPlayerController* _this, int spawnPointID,
	                                                                    int spawnRegionIndex)
	{
		AShooterPlayerController_ServerRequestRespawnAtPoint_Impl_original(_this, spawnPointID, spawnRegionIndex);

		if (spawnPointID == -1)
		{
			TeleportPlayer(_this);
		}
	}

	void CheckPlayerTeleport(TWeakObjectPtr<AShooterPlayerController> pcPtr)
	{
		if (pcPtr)
		{
			TeleportPlayer(pcPtr.Get());
		}
		else
		{
			SafeZones::Tools::Timer::Get().DelayExec(&CheckPlayerTeleport, 1, false, pcPtr);
		}
	}

	void Hook_AShooterPlayerState_ServerRequestCreateNewPlayer_Impl(AShooterPlayerState* _this,
	                                                                DWORD64 PlayerCharacterConfig)
	{
		AShooterPlayerState_ServerRequestCreateNewPlayer_Impl_original(_this, PlayerCharacterConfig);

		AShooterPlayerController* pc = _this->GetShooterController();
		if (pc)
		{
			SafeZones::Tools::Timer::Get().DelayExec(&CheckPlayerTeleport, 0, false, GetWeakReference(pc));
		}
	}

	void Hook_AActor_ReceiveActorBeginOverlap(AActor* _this, AActor* OtherActor)
	{
		if (_this)
		{
			auto safe_zone = SafeZoneManager::Get().CheckActorOverlap(_this);

			if (safe_zone)
			{
				safe_zone->OnEnterSafeZone(OtherActor);
			}
		}
		AActor_ReceiveActorBeginOverlap_original(_this, OtherActor);
	}

	void Hook_AActor_ReceiveActorEndOverlap(AActor* _this, AActor* OtherActor)
	{
		if (_this)
		{
			auto safe_zone = SafeZoneManager::Get().CheckActorOverlap(_this);

			if (safe_zone)
			{
				safe_zone->OnLeaveSafeZone(OtherActor);
			}
		}
		AActor_ReceiveActorEndOverlap_original(_this, OtherActor);
	}

	void InitHooks()
	{
		auto& hooks = ArkApi::GetHooks();

		hooks.SetHook("AShooterGameMode.InitGame", &Hook_AShooterGameMode_InitGame,
		              &AShooterGameMode_InitGame_original);

		hooks.SetHook("APrimalStructure.IsAllowedToBuild", &Hook_APrimalStructure_IsAllowedToBuild,
		              &APrimalStructure_IsAllowedToBuild_original);
		hooks.SetHook("APrimalCharacter.TakeDamage", &Hook_APrimalCharacter_TakeDamage,
		              &APrimalCharacter_TakeDamage_original);
		hooks.SetHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage,
		              &APrimalStructure_TakeDamage_original);
		hooks.SetHook("APrimalDinoCharacter.CanCarryCharacter", &Hook_APrimalDinoCharacter_CanCarryCharacter,
		              &APrimalDinoCharacter_CanCarryCharacter_original);

		hooks.SetHook("AShooterPlayerController.ServerRequestRespawnAtPoint_Implementation",
		              &Hook_AShooterPlayerController_ServerRequestRespawnAtPoint_Impl,
		              &AShooterPlayerController_ServerRequestRespawnAtPoint_Impl_original);
		hooks.SetHook("AShooterPlayerState.ServerRequestCreateNewPlayer_Implementation",
		              &Hook_AShooterPlayerState_ServerRequestCreateNewPlayer_Impl,
		              &AShooterPlayerState_ServerRequestCreateNewPlayer_Impl_original);

		hooks.SetHook("AActor.ReceiveActorBeginOverlap", &Hook_AActor_ReceiveActorBeginOverlap, &AActor_ReceiveActorBeginOverlap_original);
		hooks.SetHook("AActor.ReceiveActorEndOverlap", &Hook_AActor_ReceiveActorEndOverlap, &AActor_ReceiveActorEndOverlap_original);
	}

	void RemoveHooks()
	{
		auto& hooks = ArkApi::GetHooks();

		hooks.DisableHook("AShooterGameMode.InitGame", &Hook_AShooterGameMode_InitGame);

		hooks.DisableHook("APrimalStructure.IsAllowedToBuild", &Hook_APrimalStructure_IsAllowedToBuild);
		hooks.DisableHook("APrimalCharacter.TakeDamage", &Hook_APrimalCharacter_TakeDamage);
		hooks.DisableHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage);
		hooks.DisableHook("APrimalDinoCharacter.CanCarryCharacter", &Hook_APrimalDinoCharacter_CanCarryCharacter);

		hooks.DisableHook("AShooterPlayerController.ServerRequestRespawnAtPoint_Implementation",
		                  &Hook_AShooterPlayerController_ServerRequestRespawnAtPoint_Impl);
		hooks.DisableHook("AShooterPlayerState.ServerRequestCreateNewPlayer_Implementation",
		                  &Hook_AShooterPlayerState_ServerRequestCreateNewPlayer_Impl);

		hooks.DisableHook("AActor.ReceiveActorBeginOverlap", &Hook_AActor_ReceiveActorBeginOverlap);
		hooks.DisableHook("AActor.ReceiveActorEndOverlap", &Hook_AActor_ReceiveActorEndOverlap);
	}
}
