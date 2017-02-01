#include "RconPluginPrivatePCH.h"
#include "AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <iostream>
#include "API/Base.h"

AShooterPlayerController* FindPlayerControllerFromSteamId(unsigned __int64 steamId)
{
	AShooterPlayerController* result = nullptr;

	auto playerControllers = Ark::GetWorld()->GetPlayerControllerListField();
	for (auto playerController : playerControllers)
	{
		APlayerState* playerState = playerController->GetPlayerStateField();
		__int64 currentSteamId = playerState->GetUniqueIdField()->UniqueNetId->GetUniqueNetIdField();

		if (currentSteamId == steamId)
		{
			AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(playerController.Get());

			result = aShooterPC;
			break;
		}
	}

	return result;
}

void GiveItemNum(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(5))
	{
		__int64 steamId = FCString::Atoi64(*Parsed[1]);

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			int itemId = FCString::Atoi(*Parsed[2]);
			int quantity = FCString::Atoi(*Parsed[3]);
			float quality = FCString::Atof(*Parsed[4]);
			bool forceBP = Parsed[5] != "0";

			aShooterPC->GiveItemNum(itemId, quantity, quality, forceBP);

			// Send a reply
			FString reply = "Successfully gave items\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void AddExperience(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(4))
	{
		__int64 steamId = FCString::Atoi64(*Parsed[1]);

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			float howMuch = FCString::Atof(*Parsed[2]);
			bool fromTribeShare = Parsed[3] != "0";
			bool bPreventSharing = Parsed[4] != "0";

			aShooterPC->AddExperience(howMuch, fromTribeShare, bPreventSharing);

			// Send a reply
			FString reply = "Successfully added experience\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void SetPlayerPos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(4))
	{
		__int64 steamId = FCString::Atoi64(*Parsed[1]);

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			float x = FCString::Atof(*Parsed[2]);
			float y = FCString::Atof(*Parsed[3]);
			float z = FCString::Atof(*Parsed[4]);

			aShooterPC->SetPlayerPos(x, y, z);

			// Send a reply
			FString reply = "Successfully teleported player\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void GetPlayerPos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(1))
	{
		__int64 steamId = FCString::Atoi64(*Parsed[1]);

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			FVector pos = aShooterPC->GetDefaultActorLocationField();

			// Send a reply
			FString reply = pos.ToString() + "\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void TeleportToPlayer(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(2))
	{
		__int64 steamId = FCString::Atoi64(*Parsed[1]);

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			__int64 steamId2 = FCString::Atoi64(*Parsed[2]);
			AShooterPlayerController* aShooterPC2 = FindPlayerControllerFromSteamId(steamId2);
			if (aShooterPC2)
			{
				UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(aShooterPC->GetCheatManagerField());
				cheatManager->TeleportToPlayer(aShooterPC2->GetLinkedPlayerIDField());

				// Send a reply
				FString reply = "Successfully teleported player\n";
				rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
			}
		}
	}
}

void ListPlayerDinos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(1))
	{
		__int64 steamId = FCString::Atoi64(*Parsed[1]);

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			TArray<AActor*>* FoundActors = new TArray<AActor*>();
			UGameplayStatics::GetAllActorsOfClass(Ark::GetWorld(), APrimalDinoCharacter::GetPrivateStaticClass(), FoundActors);

			FString* pDinoName = new FString();
			FString reply = "";

			int playerTeam = aShooterPC->GetTargetingTeamField();

			for (AActor* actor : *FoundActors)
			{
				APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);

				int dinoTeam = dino->GetTargetingTeamField();

				if (dinoTeam == playerTeam)
				{
					dino->GetDescriptiveName(pDinoName);

					reply += *pDinoName + ", ID1=" + FString::FromInt(dino->GetDinoID1Field()) + ", ID2=" + FString::FromInt(dino->GetDinoID2Field()) + "\n";
				}
			}

			FMemory::Free(FoundActors);
			FMemory::Free(pDinoName);

			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void ListTribes(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString reply = "";

	AShooterGameMode* gameMode = Ark::GetGameMode();

	FTribeData* tribeData = new FTribeData();

	auto tribeIds = gameMode->GetTribesIdsField();
	for (auto& tribeId : tribeIds)
	{
		gameMode->GetTribeData(tribeData, tribeId);

		if (tribeData)
		{
			int tribeDataId = tribeData->GetTribeIDField();

			if (tribeDataId)
			{
				reply += tribeData->GetTribeNameField() + " - " + FString::FromInt(tribeDataId) + "\n";
			}
		}
	}

	FMemory::Free(tribeData);

	rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
}

void SpawnDino(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(3))
	{
		__int64 steamId = FCString::Atoi64(*Parsed[1]);

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			FString bpPath = Parsed[2];

			int dinoLvl = FCString::Atoi(*Parsed[3]);

			AActor* actor = aShooterPC->SpawnActor(&bpPath, 50, 0, 0, true);
			if (actor && actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
			{
				APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);
				dino->SetAbsoluteBaseLevelField(dinoLvl);
				dino->BeginPlay();

				// Send a reply
				FString reply = "Successfully spawned dino\n";
				rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
			}
		}
	}
}

void SpawnTamed(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(3))
	{
		__int64 steamId = FCString::Atoi64(*Parsed[1]);

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			FString className = Parsed[2];

			int dinoLvl = FCString::Atoi(*Parsed[3]);

			UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(aShooterPC->GetCheatManagerField());
			cheatManager->GMSummon(&className, dinoLvl);

			// Send a reply
			FString reply = "Successfully spawned dino\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void SpawnAtPos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(5))
	{
		APlayerController* aPC = Ark::GetWorld()->GetFirstPlayerController();

		AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(aPC);
		if (aShooterPC)
		{
			FString bpPath = Parsed[1];

			int dinoLvl = FCString::Atoi(*Parsed[2]);

			float x = FCString::Atof(*Parsed[3]);
			float y = FCString::Atof(*Parsed[4]);
			float z = FCString::Atof(*Parsed[5]);

			AActor* actor = aShooterPC->SpawnActor(&bpPath, 100, 0, 0, true);
			if (actor && actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
			{
				FVector pos = {x, y, z};
				FRotator rot = {0, 0, 0};

				actor->TeleportTo(&pos, &rot, true, false);

				APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);
				dino->SetAbsoluteBaseLevelField(dinoLvl);
				dino->BeginPlay();

				// Send a reply
				FString reply = "Successfully spawned dino\n";
				rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
			}
		}
	}
}

void GetTribeLog(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(1))
	{
		__int64 tribeId = FCString::Atoi64(*Parsed[1]);

		FString reply = "";

		FTribeData* tribeData = new FTribeData();
		Ark::GetGameMode()->GetTribeData(tribeData, tribeId);

		if (tribeData)
		{
			auto logs = tribeData->GetTribeLogField();
			for (FString log : logs)
			{
				reply += log + "\n";
			}
		}

		FMemory::Free(tribeData);

		rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
	}
}

void GetDinoPos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(2))
	{
		int dinoId1 = FCString::Atoi(*Parsed[1]);
		int dinoId2 = FCString::Atoi(*Parsed[2]);

		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(Ark::GetWorld(), dinoId1, dinoId2);
		if (dino)
		{
			FVector pos = dino->GetRootComponentField()->GetRelativeLocationField();

			FString reply = pos.ToString() + "\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void SetDinoPos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(5))
	{
		int dinoId1 = FCString::Atoi(*Parsed[1]);
		int dinoId2 = FCString::Atoi(*Parsed[2]);

		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(Ark::GetWorld(), dinoId1, dinoId2);
		if (dino)
		{
			float x = FCString::Atof(*Parsed[3]);
			float y = FCString::Atof(*Parsed[4]);
			float z = FCString::Atof(*Parsed[5]);

			FVector pos = {x, y, z};
			FRotator rot = {0, 0, 0};

			dino->TeleportTo(&pos, &rot, true, false);

			FString reply = "Successfully teleported dino\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void AddDinoExperience(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;

	const TCHAR* Delims[] = {TEXT(" ")};
	msg.ParseIntoArray(Parsed, Delims, 1);

	if (Parsed.IsValidIndex(3))
	{
		int dinoId1 = FCString::Atoi(*Parsed[1]);
		int dinoId2 = FCString::Atoi(*Parsed[2]);

		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(Ark::GetWorld(), dinoId1, dinoId2);
		if (dino)
		{
			float howMuch = FCString::Atof(*Parsed[3]);

			dino->GetMyCharacterStatusComponentField()->AddExperience(howMuch, false, EXPType::Generic);

			FString reply = "Successfully added experience to dino\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void Init()
{
	Ark::AddRconCommand("GiveItemNum", &GiveItemNum);
	Ark::AddRconCommand("AddExperience", &AddExperience);
	Ark::AddRconCommand("SetPlayerPos", &SetPlayerPos);
	Ark::AddRconCommand("GetPlayerPos ", &GetPlayerPos);
	Ark::AddRconCommand("TeleportToPlayer", &TeleportToPlayer);
	Ark::AddRconCommand("ListPlayerDinos", &ListPlayerDinos);
	Ark::AddRconCommand("ListTribes", &ListTribes);
	Ark::AddRconCommand("SpawnDino", &SpawnDino);
	Ark::AddRconCommand("SpawnTamed", &SpawnTamed);
	Ark::AddRconCommand("SpawnAtPos", &SpawnAtPos);
	Ark::AddRconCommand("GetTribeLog", &GetTribeLog);
	Ark::AddRconCommand("GetDinoPos", &GetDinoPos);
	Ark::AddRconCommand("SetDinoPos", &SetDinoPos);
	Ark::AddRconCommand("AddDinoExperience", &AddDinoExperience);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Init();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#define LOCTEXT_NAMESPACE "FRconPluginModule"

void FRconPluginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FRconPluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRconPluginModule, RconPlugin)
