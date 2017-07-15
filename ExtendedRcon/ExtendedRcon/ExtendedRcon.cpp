#include <windows.h>
#include <iostream>
#include <sstream>
#include "API/Base.h"
#include "Tools.h"

#pragma comment(lib, "ArkApi.lib")

void GiveItemNum(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(5))
	{
		unsigned __int64 steamId;
		int itemId;
		int quantity;
		float quality;
		bool forceBP;

		try
		{
			steamId = std::stoull(*Parsed[1]);
			itemId = std::stoi(*Parsed[2]);
			quantity = std::stoi(*Parsed[3]);
			quality = std::stof(*Parsed[4]);
			forceBP = std::stoi(*Parsed[5]) != 0;
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			aShooterPC->GiveItemNum(itemId, quantity, quality, forceBP);

			// Send a reply
			FString reply = L"Successfully gave items\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void GiveItem(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(5))
	{
		unsigned __int64 steamId;
		FString bpPath;
		int quantity;
		float quality;
		bool forceBP;

		try
		{
			steamId = std::stoull(*Parsed[1]);
			bpPath = Parsed[2];
			quantity = std::stoi(*Parsed[3]);
			quality = std::stof(*Parsed[4]);
			forceBP = std::stoi(*Parsed[5]) != 0;
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			aShooterPC->GiveItem(&bpPath, quantity, quality, forceBP);

			// Send a reply
			FString reply = L"Successfully gave items\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void AddExperience(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(4))
	{
		unsigned __int64 steamId;
		float howMuch;
		bool fromTribeShare;
		bool bPreventSharing;

		try
		{
			steamId = std::stoull(*Parsed[1]);
			howMuch = std::stof(*Parsed[2]);
			fromTribeShare = std::stoi(*Parsed[3]) != 0;
			bPreventSharing = std::stoi(*Parsed[4]) != 0;
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			aShooterPC->AddExperience(howMuch, fromTribeShare, bPreventSharing);

			// Send a reply
			FString reply = L"Successfully added experience\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void SetPlayerPos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(4))
	{
		unsigned __int64 steamId;
		float x;
		float y;
		float z;

		try
		{
			steamId = std::stoull(*Parsed[1]);
			x = std::stof(*Parsed[2]);
			y = std::stof(*Parsed[3]);
			z = std::stof(*Parsed[4]);
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			aShooterPC->SetPlayerPos(x, y, z);

			// Send a reply
			FString reply = L"Successfully teleported player\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void GetPlayerPos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(1))
	{
		unsigned __int64 steamId;

		try
		{
			steamId = std::stoull(*Parsed[1]);
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			FVector pos = aShooterPC->GetDefaultActorLocationField();

			// Send a reply
			wchar_t buffer[256];
			swprintf_s(buffer, TEXT("%s\n"), *pos.ToString());

			FString reply(buffer);
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void KillPlayer(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(1))
	{
		unsigned __int64 steamId;

		try
		{
			steamId = std::stoull(*Parsed[1]);
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			aShooterPC->GetPlayerCharacter()->Suicide();

			// Send a reply
			FString reply = L"Successfully killed player\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void TeleportToPlayer(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(2))
	{
		unsigned __int64 steamId;
		unsigned __int64 steamId2;

		try
		{
			steamId = std::stoull(*Parsed[1]);
			steamId2 = std::stoull(*Parsed[2]);
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			AShooterPlayerController* aShooterPC2 = FindPlayerControllerFromSteamId(steamId2);
			if (aShooterPC2)
			{
				UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(aShooterPC->GetCheatManagerField());
				cheatManager->TeleportToPlayer(aShooterPC2->GetLinkedPlayerIDField());

				// Send a reply
				FString reply = L"Successfully teleported player\n";
				rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
			}
		}
	}
}

void ListPlayerDinos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(1))
	{
		unsigned __int64 steamId;

		try
		{
			steamId = std::stoull(*Parsed[1]);
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			TArray<AActor*>* FoundActors = new TArray<AActor*>();
			UGameplayStatics::GetAllActorsOfClass(Ark::GetWorld(), APrimalDinoCharacter::GetPrivateStaticClass(), FoundActors);

			FString* pDinoName = new FString();
			FString* className = new FString();

			std::stringstream ss;

			int playerTeam = aShooterPC->GetTargetingTeamField();

			for (uint32_t i = 0; i < FoundActors->Num(); i++)
			{
				AActor* actor = (*FoundActors)[i];

				APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);

				int dinoTeam = dino->GetTargetingTeamField();

				if (dinoTeam == playerTeam)
				{
					dino->GetDescriptiveName(pDinoName);
					dino->GetDinoNameTagField().ToString(className);

					ss << pDinoName->ToString() << "(" << className->ToString() << ")" ", ID1=" << dino->GetDinoID1Field() << ", ID2=" << dino->GetDinoID2Field() << "\n";
				}
			}

			wchar_t* wcstring = ConvertToWideStr(ss.str());

			FString reply(wcstring);
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);

			delete FoundActors;
			delete pDinoName;
			delete className;
			delete[] wcstring;
		}
	}
}

void GetTribeIdOfPlayer(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(1))
	{
		unsigned __int64 playerId;

		try
		{
			playerId = std::stoull(*Parsed[1]);
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterGameMode* gameMode = Ark::GetGameMode();
		int tribeId = gameMode->GetTribeIDOfPlayerID(playerId);

		wchar_t buffer[256];
		swprintf_s(buffer, TEXT("Tribe ID - %d\n"), tribeId);

		FString reply(buffer);
		rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
	}
}

void SpawnDino(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(3))
	{
		unsigned __int64 steamId;
		int dinoLvl;

		try
		{
			steamId = std::stoull(*Parsed[1]);
			dinoLvl = std::stoi(*Parsed[3]);
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			FString bpPath = Parsed[2];

			AActor* actor = aShooterPC->SpawnActor(&bpPath, 50, 0, 0, true);
			if (actor && actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
			{
				APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);
				dino->SetAbsoluteBaseLevelField(dinoLvl);
				dino->BeginPlay();

				// Send a reply
				FString reply = L"Successfully spawned dino\n";
				rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
			}
		}
	}
}

void SpawnTamed(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(3))
	{
		unsigned __int64 steamId;
		int dinoLvl;

		try
		{
			steamId = std::stoull(*Parsed[1]);
			dinoLvl = std::stoi(*Parsed[3]);
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			FString className = Parsed[2];

			UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(aShooterPC->GetCheatManagerField());
			cheatManager->GMSummon(&className, dinoLvl);

			// Send a reply
			FString reply = L"Successfully spawned dino\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void SpawnAtPos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(5))
	{
		APlayerController* aPC = Ark::GetWorld()->GetFirstPlayerController();

		AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(aPC);
		if (aShooterPC)
		{
			int dinoLvl;
			float x;
			float y;
			float z;

			try
			{
				dinoLvl = std::stoi(*Parsed[2]);
				x = std::stof(*Parsed[3]);
				y = std::stof(*Parsed[4]);
				z = std::stof(*Parsed[5]);
			}
			catch (const std::exception&)
			{
				return;
			}

			FString bpPath = Parsed[1];

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
				FString reply = L"Successfully spawned dino\n";
				rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
			}
		}
	}
}

void GetTribeLog(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(1))
	{
		__int64 tribeId;

		try
		{
			tribeId = std::stoull(*Parsed[1]);
		}
		catch (const std::exception&)
		{
			return;
		}

		FTribeData* tribeData = new FTribeData();
		Ark::GetGameMode()->GetTribeData(tribeData, tribeId);

		if (tribeData)
		{
			std::stringstream ss;

			auto logs = tribeData->GetTribeLogField();
			for (uint32_t i = 0; i < logs.Num(); i++)
			{
				auto log = logs[i];

				ss << log.ToString() << "\n";
			}

			wchar_t* wcstring = ConvertToWideStr(ss.str());

			FString reply(wcstring);
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);

			delete[] wcstring;
		}

		delete tribeData;
	}
}

void GetDinoPos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(2))
	{
		int dinoId1;
		int dinoId2;

		try
		{
			dinoId1 = std::stoi(*Parsed[1]);
			dinoId2 = std::stoi(*Parsed[2]);
		}
		catch (const std::exception&)
		{
			return;
		}

		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(Ark::GetWorld(), dinoId1, dinoId2);
		if (dino)
		{
			FVector pos = dino->GetRootComponentField()->GetRelativeLocationField();

			wchar_t buffer[256];
			swprintf_s(buffer, TEXT("%s\n"), *pos.ToString());

			FString reply(buffer);
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void SetDinoPos(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(5))
	{
		int dinoId1;
		int dinoId2;
		float x;
		float y;
		float z;

		try
		{
			dinoId1 = std::stoi(*Parsed[1]);
			dinoId2 = std::stoi(*Parsed[2]);
			x = std::stof(*Parsed[3]);
			y = std::stof(*Parsed[4]);
			z = std::stof(*Parsed[5]);
		}
		catch (const std::exception&)
		{
			return;
		}

		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(Ark::GetWorld(), dinoId1, dinoId2);
		if (dino)
		{
			FVector pos = {x, y, z};
			FRotator rot = {0, 0, 0};

			dino->TeleportTo(&pos, &rot, true, false);

			FString reply = L"Successfully teleported dino\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void AddDinoExperience(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(3))
	{
		int dinoId1;
		int dinoId2;
		float howMuch;

		try
		{
			dinoId1 = std::stoi(*Parsed[1]);
			dinoId2 = std::stoi(*Parsed[2]);
			howMuch = std::stof(*Parsed[3]);
		}
		catch (const std::exception&)
		{
			return;
		}

		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(Ark::GetWorld(), dinoId1, dinoId2);
		if (dino)
		{
			dino->GetMyCharacterStatusComponentField()->AddExperience(howMuch, false, EXPType::XP_GENERIC);

			FString reply = L"Successfully added experience to dino\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void KillDino(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(2))
	{
		int dinoId1;
		int dinoId2;

		try
		{
			dinoId1 = std::stoi(*Parsed[1]);
			dinoId2 = std::stoi(*Parsed[2]);
		}
		catch (const std::exception&)
		{
			return;
		}

		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(Ark::GetWorld(), dinoId1, dinoId2);
		if (dino)
		{
			dino->Suicide();

			FString reply = L"Killed dino\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void ClientChat(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L"'", true);

	if (Parsed.IsValidIndex(2))
	{
		auto playerControllers = Ark::GetWorld()->GetPlayerControllerListField();
		for (uint32_t i = 0; i < playerControllers.Num(); ++i)
		{
			auto playerController = playerControllers[i];

			AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(playerController.Get());

			SendChatMessage(aShooterPC, Parsed[2], *Parsed[1]);
		}
	}
}

void UnlockEngram(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
{
	FString msg = rconPacket->Body;

	TArray<FString> Parsed;
	msg.ParseIntoArray(&Parsed, L" ", true);

	if (Parsed.IsValidIndex(2))
	{
		unsigned __int64 steamId;
		FString bpPath;

		try
		{
			steamId = std::stoull(*Parsed[1]);
			bpPath = Parsed[2];
		}
		catch (const std::exception&)
		{
			return;
		}

		AShooterPlayerController* aShooterPC = FindPlayerControllerFromSteamId(steamId);
		if (aShooterPC)
		{
			UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(aShooterPC->GetCheatManagerField());
			cheatManager->UnlockEngram(&bpPath);

			// Send a reply
			FString reply = L"Successfully unlocked engram\n";
			rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
		}
	}
}

void Init()
{
	Ark::AddRconCommand(L"GiveItemNum", &GiveItemNum);
	Ark::AddRconCommand(L"GiveItem", &GiveItem);
	Ark::AddRconCommand(L"AddExperience", &AddExperience);
	Ark::AddRconCommand(L"SetPlayerPos", &SetPlayerPos);
	Ark::AddRconCommand(L"GetPlayerPos ", &GetPlayerPos);
	Ark::AddRconCommand(L"KillPlayer ", &KillPlayer);
	Ark::AddRconCommand(L"TeleportToPlayer", &TeleportToPlayer);
	Ark::AddRconCommand(L"ListPlayerDinos", &ListPlayerDinos);
	Ark::AddRconCommand(L"GetTribeIdOfPlayer", &GetTribeIdOfPlayer);
	Ark::AddRconCommand(L"SpawnDino", &SpawnDino);
	Ark::AddRconCommand(L"SpawnTamed", &SpawnTamed);
	Ark::AddRconCommand(L"SpawnAtPos", &SpawnAtPos);
	Ark::AddRconCommand(L"GetTribeLog", &GetTribeLog);
	Ark::AddRconCommand(L"GetDinoPos", &GetDinoPos);
	Ark::AddRconCommand(L"SetDinoPos", &SetDinoPos);
	Ark::AddRconCommand(L"AddDinoExperience", &AddDinoExperience);
	Ark::AddRconCommand(L"KillDino", &KillDino);
	Ark::AddRconCommand(L"ClientChat", &ClientChat);
	Ark::AddRconCommand(L"UnlockEngram", &UnlockEngram);
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
