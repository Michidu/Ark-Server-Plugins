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

	DECLARE_HOOK(AShooterCharacter_CanDragCharacter, bool, AShooterCharacter*, APrimalCharacter*);
	DECLARE_HOOK(APrimalCharacter_CanBeCarried, bool, APrimalCharacter*, APrimalCharacter*);
	DECLARE_HOOK(AShooterCharacter_AllowGrappling_Implementation, bool, AShooterCharacter*);
	DECLARE_HOOK(APrimalBuff_Grappled_CanPullChar_Implementation, bool, APrimalBuff_Grappled*, APrimalCharacter*, const bool);
	DECLARE_HOOK(AShooterProjectile_OnImpact, void, AShooterProjectile*, FHitResult*, bool);
	DECLARE_HOOK(AShooterGameMode_IsTribeWar, bool, AShooterGameMode*, int, int);
	DECLARE_HOOK(AShooterGameMode_StartNewShooterPlayer, void, AShooterGameMode*, APlayerController*, bool, bool, FPrimalPlayerCharacterConfigStruct*, UPrimalPlayerData*);
	DECLARE_HOOK(APrimalDinoCharacter_BeginPlay, void, APrimalDinoCharacter*);

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
		if (_this
			&& !_this->IsDead())
		{
			if (SafeZoneManager::Get().CheckActorAction(_this, 1, DamageCauser))
				return 0.f;

			// else prevent pvp is not false, so for pve servers we proceed to implement "is tribe war"

			if (SafeZoneManager::ShouldDoTribeWarCheck()
				&& _this
				&& DamageCauser)
			{
				const int& thisTeamId = _this->TargetingTeamField();
				const int& thatTeamId = DamageCauser->TargetingTeamField();

				auto& playerPos = _this->RootComponentField()->RelativeLocationField();
				std::shared_ptr<SafeZone> nearestZone = SafeZoneManager::Get().GetNearestZone(playerPos);

				if (nearestZone
					&& nearestZone->IsOverlappingActor(_this)
					&& thisTeamId > 50000
					&& thatTeamId > 50000)
				{
					nearestZone->AddTribesPairForTribeWarCheck(thisTeamId, thatTeamId);
					const float res = APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
					nearestZone->RemoveTribesFromTribeWarPair(thisTeamId, thatTeamId);
					return res;
				}
			}

		}
		return APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
	}

	float Hook_APrimalStructure_TakeDamage(APrimalStructure* _this, float Damage, FDamageEvent* DamageEvent,
		AController* EventInstigator, AActor* DamageCauser)
	{
		if (SafeZoneManager::Get().CheckActorAction(_this, 2, DamageCauser))
		{
			return 0.f;
		}
		else
		{
			if (SafeZoneManager::ShouldDoTribeWarCheck()
				&& _this
				&& DamageCauser)
			{
				const int& thisTeamId = _this->TargetingTeamField();
				const int& thatTeamId = DamageCauser->TargetingTeamField();

				auto& Pos = _this->RootComponentField()->RelativeLocationField();
				std::shared_ptr<SafeZone> nearestZone = SafeZoneManager::Get().GetNearestZone(Pos);

				if (nearestZone
					&& nearestZone->IsOverlappingActor(_this)
					&& thisTeamId > 50000
					&& thatTeamId > 50000)
				{
					nearestZone->AddTribesPairForTribeWarCheck(thisTeamId, thatTeamId);
					const float res = APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
					nearestZone->RemoveTribesFromTribeWarPair(thisTeamId, thatTeamId);
					return res;
				}
			}
		}

		return APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
	}

	bool Hook_APrimalDinoCharacter_CanCarryCharacter(APrimalDinoCharacter* _this, APrimalCharacter* CanCarryPawn)
	{
		return SafeZoneManager::Get().CheckActorAction(CanCarryPawn, 1, _this)
			       ? false
			       : APrimalDinoCharacter_CanCarryCharacter_original(_this, CanCarryPawn);
	}

	bool  Hook_APrimalCharacter_CanBeCarried(APrimalCharacter* _this, APrimalCharacter* ByCarrier)
	{
		return SafeZoneManager::Get().CheckActorAction(_this, 1, ByCarrier) 
			? false 
			: APrimalCharacter_CanBeCarried_original(_this, ByCarrier);
	}

	bool  Hook_AShooterCharacter_CanDragCharacter(AShooterCharacter* _this, APrimalCharacter* Character)
	{
		return SafeZoneManager::Get().CheckActorAction(Character, 1, _this)
			? false
			: AShooterCharacter_CanDragCharacter_original(_this, Character);
	}

	bool  Hook_AShooterCharacter_AllowGrappling_Implementation(AShooterCharacter* _this)
	{
		return SafeZoneManager::Get().CheckActorAction(_this, 1)
			? false
			: AShooterCharacter_AllowGrappling_Implementation_original(_this);
	}

	bool Hook_APrimalBuff_Grappled_CanPullChar_Implementation(APrimalBuff_Grappled* _this, APrimalCharacter* ForChar, const bool bForStart)
	{
		return SafeZoneManager::Get().CheckActorAction(ForChar, 1, _this->OwnerField())
			? false
			: APrimalBuff_Grappled_CanPullChar_Implementation_original(_this, ForChar, bForStart);
	}

	void Hook_AShooterProjectile_OnImpact(AShooterProjectile* _this, FHitResult* HitResult, bool bFromReplication)
	{
		if (_this
			&& HitResult
			&& _this->IsA(APrimalProjectileGrapplingHook::GetPrivateStaticClass()))
		{
			AActor* hitActor = HitResult->GetActor();
			if (hitActor
				&& hitActor->IsA(APrimalCharacter::GetPrivateStaticClass()))
			{
				if (SafeZoneManager::Get().CheckActorAction(hitActor, 1, _this->DamageCauserField()))
				{
					_this->Destroy(false, false);
					return;
				}
			}
		}
		AShooterProjectile_OnImpact_original(_this, HitResult, bFromReplication);
	}

	void TeleportPlayer(AShooterPlayerController* pc, FString spawnName)
	{
		if (!pc
			|| !pc->CharacterField())
		{
			SafeZones::Tools::Timer::Get().DelayExec(&TeleportPlayer, 1, false, pc, spawnName);
			return;
		}

		try
		{
			auto spawn_points = config.value("OverrideSpawnPoint", nlohmann::json::object());

			FString map_name("");
			ArkApi::GetApiUtils().GetShooterGameMode()->GetMapName(&map_name);

			auto map_spawn_points = spawn_points.value(map_name.ToString(), nlohmann::json::object());

			if (!map_spawn_points.empty())
			{
				auto spawn_list = map_spawn_points.value(spawnName.ToString(), nlohmann::json::array());
				const int idx = FMath::RandRange(0, (spawn_list.size() - 1));

				const std::vector<float> spawn = spawn_list[idx];

				if (spawn.size() >= 3)
				{
					pc->SetPlayerPos(spawn[0], spawn[1], spawn[2]);
				}
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

		if (_this
			&& spawnPointID < 0)
		{
			TArray<FString>* spawnLocations = UPrimalGameData::BPGetGameData()->GetPlayerSpawnRegions(ArkApi::GetApiUtils().GetWorld());

			if (spawnLocations
				&& spawnLocations->IsValidIndex(spawnRegionIndex))
			{
				const FString str = (*spawnLocations)[spawnRegionIndex];

				TeleportPlayer(_this, str);
			}
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

	bool  Hook_AShooterGameMode_IsTribeWar(AShooterGameMode* _this, int TribeID1, int TribeID2)
	{
		if (SafeZoneManager::Get().CheckIfTribesAreInTribeWar(TribeID1, TribeID2))
			return true;

		return AShooterGameMode_IsTribeWar_original(_this, TribeID1, TribeID2);
	}

	void  Hook_AShooterGameMode_StartNewShooterPlayer(AShooterGameMode* _this, APlayerController* NewPlayer, bool bForceCreateNewPlayerData, bool bIsFromLogin, FPrimalPlayerCharacterConfigStruct* charConfig, UPrimalPlayerData* ArkPlayerData)
	{
		AShooterGameMode_StartNewShooterPlayer_original(_this, NewPlayer, bForceCreateNewPlayerData, bIsFromLogin, charConfig, ArkPlayerData);

		int spawnReg = -1;

		if (NewPlayer
			&& charConfig
			&& !bIsFromLogin)
		{
			spawnReg = charConfig->PlayerSpawnRegionIndex;
			Tools::Timer::Get().DelayExec(
				[](TWeakObjectPtr<AShooterPlayerController> PC, const int spawnRegion)
				{
					if (PC)
					{
						UPrimalGameData* pgd = UPrimalGameData::BPGetGameData();
						TArray<FString>* spawns = pgd->GetPlayerSpawnRegions(ArkApi::GetApiUtils().GetWorld());
						if (spawns
							&& spawns->IsValidIndex(spawnRegion))
						{
							TeleportPlayer(PC, (*spawns)[spawnRegion]);
						}
					}
				},
				0, false, GetWeakReference(static_cast<AShooterPlayerController*>(NewPlayer)), spawnReg);
		}
	}

	void Hook_APrimalDinoCharacter_BeginPlay(APrimalDinoCharacter* _this)
	{
		APrimalDinoCharacter_BeginPlay_original(_this);

		Tools::Timer::Get().DelayExec(
			std::bind([](TWeakObjectPtr<APrimalDinoCharacter> _this)
			{
				if (_this
					&& !_this->IsDead()
					&& !_this->BPIsTamed())
				{
					for (auto& safe_zone : SafeZoneManager::Get().GetAllSafeZones())
					{
						if (safe_zone->IsOverlappingActor(_this))
						{
							safe_zone->OnEnterSafeZone(_this);
							break;
						}
					}
				}

			}, GetWeakReference(_this)), 2, false);
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

		hooks.SetHook("AActor.ReceiveActorBeginOverlap", &Hook_AActor_ReceiveActorBeginOverlap, &AActor_ReceiveActorBeginOverlap_original);
		hooks.SetHook("AActor.ReceiveActorEndOverlap", &Hook_AActor_ReceiveActorEndOverlap, &AActor_ReceiveActorEndOverlap_original);

		hooks.SetHook("AShooterCharacter.CanDragCharacter", &Hook_AShooterCharacter_CanDragCharacter, &AShooterCharacter_CanDragCharacter_original);
		hooks.SetHook("APrimalCharacter.CanBeCarried", &Hook_APrimalCharacter_CanBeCarried, &APrimalCharacter_CanBeCarried_original);
		hooks.SetHook("AShooterCharacter.AllowGrappling_Implementation", &Hook_AShooterCharacter_AllowGrappling_Implementation, &AShooterCharacter_AllowGrappling_Implementation_original);
		hooks.SetHook("APrimalBuff_Grappled.CanPullChar_Implementation", &Hook_APrimalBuff_Grappled_CanPullChar_Implementation, &APrimalBuff_Grappled_CanPullChar_Implementation_original);
		hooks.SetHook("AShooterProjectile.OnImpact", &Hook_AShooterProjectile_OnImpact, &AShooterProjectile_OnImpact_original);
		hooks.SetHook("AShooterGameMode.IsTribeWar", &Hook_AShooterGameMode_IsTribeWar, &AShooterGameMode_IsTribeWar_original);
		hooks.SetHook("AShooterGameMode.StartNewShooterPlayer", &Hook_AShooterGameMode_StartNewShooterPlayer, &AShooterGameMode_StartNewShooterPlayer_original);
		hooks.SetHook("APrimalDinoCharacter.BeginPlay", &Hook_APrimalDinoCharacter_BeginPlay, &APrimalDinoCharacter_BeginPlay_original);
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

		hooks.DisableHook("AActor.ReceiveActorBeginOverlap", &Hook_AActor_ReceiveActorBeginOverlap);
		hooks.DisableHook("AActor.ReceiveActorEndOverlap", &Hook_AActor_ReceiveActorEndOverlap);

		hooks.DisableHook("AShooterCharacter.CanDragCharacter", &Hook_AShooterCharacter_CanDragCharacter);
		hooks.DisableHook("APrimalCharacter.CanBeCarried", &Hook_APrimalCharacter_CanBeCarried);
		hooks.DisableHook("AShooterCharacter.AllowGrappling_Implementation", &Hook_AShooterCharacter_AllowGrappling_Implementation);
		hooks.DisableHook("APrimalBuff_Grappled.CanPullChar_Implementation", &Hook_APrimalBuff_Grappled_CanPullChar_Implementation);
		hooks.DisableHook("AShooterProjectile.OnImpact", &Hook_AShooterProjectile_OnImpact);
		hooks.DisableHook("AShooterGameMode.IsTribeWar", &Hook_AShooterGameMode_IsTribeWar);
		hooks.DisableHook("AShooterGameMode.StartNewShooterPlayer", &Hook_AShooterGameMode_StartNewShooterPlayer);
		hooks.DisableHook("APrimalDinoCharacter.BeginPlay", &Hook_APrimalDinoCharacter_BeginPlay);
	}
}
