#pragma once

#ifdef RCON_ARK
#include <API/Ark/Ark.h>
#else
#include <API/Atlas/Atlas.h>
#endif

inline void SendRconReply(RCONClientConnection* rcon_connection, int packet_id, const FString& msg)
{
	FString reply = msg + "\n";
	rcon_connection->SendMessageW(packet_id, 0, &reply);
}

inline bool GiveItem(AShooterPlayerController* shooter_pc, FString* blueprint_path, int quantity_override,
                     float quality_override, bool force_blueprint)
{
#ifdef RCON_ARK
	TArray<UPrimalItem*> out_items;
	return shooter_pc->GiveItem(&out_items, blueprint_path, quantity_override, quality_override, force_blueprint,
	                            false, 0);
#else
	return shooter_pc->GiveItem(blueprint_path, quantity_override, quality_override, force_blueprint);
#endif
}

inline APrimalDinoCharacter* FindDinoWithID(unsigned int dino_id1, unsigned int dino_id2)
{
#ifdef RCON_ARK
	return APrimalDinoCharacter::FindDinoWithID(ArkApi::GetApiUtils().GetWorld(), dino_id1, dino_id2);
#else
	APrimalDinoCharacter* found_dino = nullptr;

	TArray<AActor*> found_actors;
	UGameplayStatics::GetAllActorsOfClass(reinterpret_cast<UObject*>(ArkApi::GetApiUtils().GetWorld()),
	                                      APrimalDinoCharacter::StaticClass(), &found_actors);

	for (AActor* actor : found_actors)
	{
		APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);
		if (!dino)
			continue;

		if (dino->DinoID1Field() == dino_id1 && dino->DinoID2Field() == dino_id2)
		{
			found_dino = dino;
			break;
		}
	}

	return found_dino;
#endif
}
