#include <API/ARK/Ark.h>

#pragma comment(lib, "ArkApi.lib")

void RemoveOwnershipRadiusCmd(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		int radius;

		try
		{
			radius = std::stoi(*parsed[1]);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		UWorld* world = ArkApi::GetApiUtils().GetWorld();

		TArray<AActor*> new_actors;

		TArray<AActor*> actors_ignore;
		TArray<TEnumAsByte<enum EObjectTypeQuery>> types;

		UKismetSystemLibrary::SphereOverlapActors_NEW(world, ArkApi::IApiUtils::GetPosition(player_controller),
		                                              static_cast<float>(radius), &types, nullptr, &actors_ignore,
		                                              &new_actors);

		for (const auto& actor : new_actors)
		{
			if (actor->IsA(APrimalStructure::GetPrivateStaticClass()))
			{
				APrimalStructure* structure = static_cast<APrimalStructure*>(actor);

				structure->OwningPlayerIDField() = 0;
				structure->OwningPlayerNameField() = "";

				FString new_name = "";
				structure->NetUpdateOriginalOwnerNameAndID(0, &new_name);
			}
		}
	}
}

void RemoveOwnershipCmd(APlayerController* player_controller, FString* cmd, bool)
{
	AShooterCharacter* character = static_cast<AShooterCharacter*>(player_controller->CharacterField());

	AActor* actor = character->GetAimedActor(ECC_GameTraceChannel1, nullptr, 0.0, 0.0, nullptr, nullptr, false, false);
	if (actor && actor->IsA(APrimalStructure::GetPrivateStaticClass()))
	{
		APrimalStructure* structure = static_cast<APrimalStructure*>(actor);

		TArray<APrimalStructure*> out_linked_structures;
		structure->GetAllLinkedStructures(&out_linked_structures);

		for (APrimalStructure* linked_structure : out_linked_structures)
		{
			linked_structure->OwningPlayerIDField() = 0;
			linked_structure->OwningPlayerNameField() = "";

			FString new_name = "";
			linked_structure->NetUpdateOriginalOwnerNameAndID(0, &new_name);
		}
	}
}

void Load()
{
	Log::Get().Init("RemoveOwnership");

	ArkApi::GetCommands().AddConsoleCommand("RemoveOwnershipRadius", &RemoveOwnershipRadiusCmd);
	ArkApi::GetCommands().AddConsoleCommand("RemoveOwnership", &RemoveOwnershipCmd);
}

void Unload()
{
	ArkApi::GetCommands().RemoveConsoleCommand("RemoveOwnershipRadius");
	ArkApi::GetCommands().RemoveConsoleCommand("RemoveOwnership");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		Unload();
		break;
	}
	return TRUE;
}
