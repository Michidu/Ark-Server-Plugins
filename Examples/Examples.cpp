void PrintOnlinePlayersIds()
{
	for (auto playerController : Ark::GetWorld()->GetPlayerControllerListField())
	{
		AShooterPlayerController* aShooterPlayerController = static_cast<AShooterPlayerController*>(playerController.Get(true));

		std::cout << aShooterPlayerController->GetLinkedPlayerIDField() << "\n";
	}
}

void PrintAllPlayersIds()
{
	for (auto& data : Ark::GetGameMode()->GetPlayersIdsField())
	{
		std::cout << "ARK ID - " << data.Key << "Steam ID - " << data.Value << "\n";
	}
}

void PrintNearTamedDinosNames(AShooterPlayerController* playerController, float distance)
{
	TArray<AWeakObjectPtr<APrimalDinoCharacter>, FDefaultAllocator>* dinos = new TArray<AWeakObjectPtr<APrimalDinoCharacter>, FDefaultAllocator>();
	playerController->GetTamedDinosNearBy(dinos, distance);

	FString* dinoName = new FString();

	for (auto dino : *dinos)
	{
		dino->GetDinoDescriptiveName(dinoName);

		std::cout << TCHAR_TO_ANSI(**dinoName) << "\n";
	}

	FMemory::Free(dinoName);
	FMemory::Free(dinos);
}

void PrintAllInventoryItemsNames(AShooterPlayerController* playerController)
{
	ACharacter* aCharacter = playerController->GetCharacterField();

	APrimalCharacter* aPrimalCharacter = static_cast<APrimalCharacter*>(aCharacter);

	FString* itemName = new FString();

	auto inventoryItems = aPrimalCharacter->GetMyInventoryComponentField()->GetInventoryItemsField();
	for (UPrimalItem* item : inventoryItems)
	{
		item->GetItemName(itemName, true, true);

		std::cout << TCHAR_TO_ANSI(**itemName) << ", craftable - " << item->CanCraft() << "\n";
	}

	FMemory::Free(itemName);
}

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

void PrintAllDinos()
{
	TArray<AActor*>* FoundActors = new TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(Ark::GetWorld(), APrimalDinoCharacter::GetPrivateStaticClass(), FoundActors);
	
	for (AActor* actor : *FoundActors)
	{
		APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);
		
		std::cout << TCHAR_TO_ANSI(*dino->GetDescriptiveNameField()) << "\n";
	}
	
	FMemory::Free(FoundActors);
}