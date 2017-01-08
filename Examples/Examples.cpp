void PrintAllActorsNames() // Warning: there are more than 30 000 of them
{
	for (AActor* actor : Ark::GetWorld()->GetPersistentLevel()->GetActors())
	{
		FString* name = new FString();
		actor->GetHumanReadableName(name);

		if (!name->IsEmpty())
		{
			std::cout << TCHAR_TO_ANSI(**name) << "\n";
		}
	}
}

void PrintOnlinePlayersIds()
{
	for (auto playerController : Ark::GetWorld()->GetPlayerControllerList())
	{
		AShooterPlayerController* aShooterPlayerController = static_cast<AShooterPlayerController*>(playerController.Get(true));

		std::cout << aShooterPlayerController->GetLinkedPlayerID() << "\n";
	}
}

void PrintAllPlayersIds()
{
	for (auto& data : Ark::GetGameMode()->GetPlayersIds())
	{
		std::cout << data.Key << "\n";
	}	
}

void PrintNearTamedDinosNames(AShooterPlayerController* playerController, float distance)
{
	TArray<AWeakObjectPtr<APrimalDinoCharacter>, FDefaultAllocator>* dinos = new TArray<AWeakObjectPtr<APrimalDinoCharacter>, FDefaultAllocator>();
	playerController->GetTamedDinosNearBy(*dinos, distance);

	for (auto dino : *dinos)
	{
		FString* name = new FString();
		dino->GetDinoDescriptiveName(name);

		std::cout << TCHAR_TO_ANSI(**name) << "\n";
	}
}

void PrintAllInventoryItemsNames(AShooterPlayerController* playerController)
{
	ACharacter* aCharacter = playerController->GetCharacter();

	APrimalCharacter* aPrimalCharacter = static_cast<APrimalCharacter*>(aCharacter);

	for (UPrimalItem* item : aPrimalCharacter->GetMyInventoryComponent()->GetInventoryItems())
	{
		std::cout << TCHAR_TO_ANSI(*item->GetDescriptiveNameBase()) << ", is blueprint - " << item->CanCraft() << "\n";
	}
}