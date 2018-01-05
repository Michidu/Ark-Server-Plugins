#include <API/ARK/Ark.h>

#pragma comment(lib, "ArkApi.lib")

void SendRconReply(RCONClientConnection* rcon_connection, int packet_id, const FString& msg)
{
	FString reply = msg + "\n";
	rcon_connection->SendMessageW(packet_id, 0, &reply);
}

void GiveItemNum(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(5))
	{
		unsigned __int64 steam_id;
		int item_id;
		int quantity;
		float quality;
		bool force_bp;

		try
		{
			steam_id = std::stoull(*parsed[1]);
			item_id = std::stoi(*parsed[2]);
			quantity = std::stoi(*parsed[3]);
			quality = std::stof(*parsed[4]);
			force_bp = std::stoi(*parsed[5]) != 0;
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		AShooterPlayerController* shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (!shooter_pc)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find player from the given steam id");
			return;
		}

		const bool result = shooter_pc->GiveItemNum(item_id, quantity, quality, force_bp);

		SendRconReply(rcon_connection, rcon_packet->Id, result ? "Successfully gave items" : "Request has failed");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void GiveItem(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(5))
	{
		unsigned __int64 steam_id;
		FString blueprint;
		int quantity;
		float quality;
		bool force_bp;

		try
		{
			steam_id = std::stoull(*parsed[1]);
			blueprint = parsed[2];
			quantity = std::stoi(*parsed[3]);
			quality = std::stof(*parsed[4]);
			force_bp = std::stoi(*parsed[5]) != 0;
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		AShooterPlayerController* shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (!shooter_pc)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find player from the given steam id");
			return;
		}

		const bool result = shooter_pc->GiveItem(&blueprint, quantity, quality, force_bp);

		SendRconReply(rcon_connection, rcon_packet->Id, result ? "Successfully gave items" : "Request has failed");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void GiveItemToAll(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(4))
	{
		FString blueprint;
		int quantity;
		float quality;
		bool force_bp;

		try
		{
			blueprint = parsed[1];
			quantity = std::stoi(*parsed[2]);
			quality = std::stof(*parsed[3]);
			force_bp = std::stoi(*parsed[4]) != 0;
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		const auto& player_controllers = ArkApi::GetApiUtils().GetWorld()->PlayerControllerListField()();
		for (TWeakObjectPtr<APlayerController> player_controller : player_controllers)
		{
			AShooterPlayerController* shooter_pc = static_cast<AShooterPlayerController*>(player_controller.Get());

			shooter_pc->GiveItem(&blueprint, quantity, quality, force_bp);
		}

		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully gave items");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void AddExperience(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(4))
	{
		unsigned __int64 steam_id;
		float how_much;
		bool from_tribe_share;
		bool prevent_sharing;

		try
		{
			steam_id = std::stoull(*parsed[1]);
			how_much = std::stof(*parsed[2]);
			from_tribe_share = std::stoi(*parsed[3]) != 0;
			prevent_sharing = std::stoi(*parsed[4]) != 0;
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		AShooterPlayerController* shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (!shooter_pc)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find player from the given steam id");
			return;
		}

		shooter_pc->AddExperience(how_much, from_tribe_share, prevent_sharing);

		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully added experience");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void SetPlayerPos(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(4))
	{
		unsigned __int64 steam_id;
		float x;
		float y;
		float z;

		try
		{
			steam_id = std::stoull(*parsed[1]);
			x = std::stof(*parsed[2]);
			y = std::stof(*parsed[3]);
			z = std::stof(*parsed[4]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		AShooterPlayerController* shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (!shooter_pc)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find player from the given steam id");
			return;
		}

		shooter_pc->SetPlayerPos(x, y, z);

		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully teleported player");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void GetPlayerPos(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		unsigned __int64 steam_id;

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		AShooterPlayerController* shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (!shooter_pc)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find player from the given steam id");
			return;
		}

		FVector pos = shooter_pc->DefaultActorLocationField()();

		SendRconReply(rcon_connection, rcon_packet->Id, pos.ToString());
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void TeleportAllPlayers(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(3))
	{
		float x;
		float y;
		float z;

		try
		{
			x = std::stof(*parsed[1]);
			y = std::stof(*parsed[2]);
			z = std::stof(*parsed[3]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		const auto& player_controllers = ArkApi::GetApiUtils().GetWorld()->PlayerControllerListField()();
		for (TWeakObjectPtr<APlayerController> player_controller : player_controllers)
		{
			AShooterPlayerController* shooter_pc = static_cast<AShooterPlayerController*>(player_controller.Get());

			shooter_pc->SetPlayerPos(x, y, z);
		}

		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully teleported players");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void KillPlayer(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		unsigned __int64 steam_id;

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		AShooterPlayerController* shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (!shooter_pc)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find player from the given steam id");
			return;
		}

		shooter_pc->GetPlayerCharacter()->Suicide();

		// Send a reply
		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully killed player");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void TeleportToPlayer(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		unsigned __int64 steam_id;
		unsigned __int64 steam_id2;

		try
		{
			steam_id = std::stoull(*parsed[1]);
			steam_id2 = std::stoull(*parsed[2]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		AShooterPlayerController* shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (!shooter_pc)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find player from the given steam id");
			return;
		}

		AShooterPlayerController* shooter_pc2 = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id2);
		if (!shooter_pc2)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find player from the given steam id");
			return;
		}

		UShooterCheatManager* cheat_manager = static_cast<UShooterCheatManager*>(shooter_pc->CheatManagerField()());
		cheat_manager->TeleportToPlayer(shooter_pc2->LinkedPlayerIDField()());

		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully teleported player");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void ListPlayerDinos(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		unsigned __int64 steam_id;

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		AShooterPlayerController* shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (!shooter_pc)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find player from the given steam id");
			return;
		}

		TArray<AActor*> found_actors;
		UGameplayStatics::GetAllActorsOfClass(reinterpret_cast<UObject*>(ArkApi::GetApiUtils().GetWorld()),
		                                      APrimalDinoCharacter::GetPrivateStaticClass(), &found_actors);

		FString dino_name;
		FString class_name;

		const int player_team = shooter_pc->TargetingTeamField()();

		FString reply = "";

		for (AActor* actor : found_actors)
		{
			APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);

			const int dino_team = dino->TargetingTeamField()();
			if (dino_team == player_team)
			{
				dino->GetDescriptiveName(&dino_name);
				dino->DinoNameTagField()().ToString(&class_name);

				reply += dino_name + " (" + class_name + ")" + ", ID1=" + FString::FromInt(dino->DinoID1Field()()) + ", ID2="
					+ FString::FromInt(dino->DinoID2Field()()) + "\n";
			}

			Sleep(0);
		}


		SendRconReply(rcon_connection, rcon_packet->Id, reply);
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void GetTribeIdOfPlayer(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		unsigned __int64 steam_id;

		try
		{
			steam_id = std::stoull(*parsed[1]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		AShooterGameMode* game_mode = ArkApi::GetApiUtils().GetShooterGameMode();

		const unsigned int player_id = game_mode->GetPlayerIDForSteamID(steam_id);

		const int tribe_id = game_mode->GetTribeIDOfPlayerID(player_id);

		SendRconReply(rcon_connection, rcon_packet->Id, "Tribe ID - " + FString::FromInt(tribe_id));
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void SpawnDino(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(7))
	{
		unsigned __int64 steam_id;
		int dino_lvl;
		bool force_tame;
		float x;
		float y;
		float z;

		try
		{
			steam_id = std::stoull(*parsed[1]);
			dino_lvl = std::stoi(*parsed[3]);
			force_tame = std::stoi(*parsed[4]) != 0;
			x = std::stof(*parsed[5]);
			y = std::stof(*parsed[6]);
			z = std::stof(*parsed[7]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		AShooterPlayerController* shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (!shooter_pc)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find player from the given steam id");
			return;
		}

		const FString blueprint = parsed[2];
		FVector location{x, y, z};

		ArkApi::GetApiUtils().SpawnDino(shooter_pc, blueprint, &location, dino_lvl, force_tame);

		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully spawned dino");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void GetTribeLog(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		int tribe_id;

		try
		{
			tribe_id = std::stoi(*parsed[1]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		FTribeData tribe_data;
		if (!ArkApi::GetApiUtils().GetShooterGameMode()->GetOrLoadTribeData(tribe_id, &tribe_data))
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Failed to load tribe data");
			return;
		}

		FString reply = "";

		TArray<FString> logs = tribe_data.TribeLog;
		for (const FString& log : logs)
		{
			reply += log + "\n";
		}

		SendRconReply(rcon_connection, rcon_packet->Id, reply);
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void GetDinoPos(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		int dino_id1;
		int dino_id2;

		try
		{
			dino_id1 = std::stoi(*parsed[1]);
			dino_id2 = std::stoi(*parsed[2]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(ArkApi::GetApiUtils().GetWorld(), dino_id1,
		                                                                  dino_id2);
		if (!dino)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find dino");
			return;
		}

		FVector pos = dino->RootComponentField()()->RelativeLocationField()();

		FString reply = pos.ToString();
		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void SetDinoPos(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(5))
	{
		int dino_id1;
		int dino_id2;
		float x;
		float y;
		float z;

		try
		{
			dino_id1 = std::stoi(*parsed[1]);
			dino_id2 = std::stoi(*parsed[2]);
			x = std::stof(*parsed[3]);
			y = std::stof(*parsed[4]);
			z = std::stof(*parsed[5]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(ArkApi::GetApiUtils().GetWorld(), dino_id1,
		                                                                  dino_id2);
		if (!dino)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find dino");
			return;
		}

		FVector pos{x, y, z};
		FRotator rot{0, 0, 0};

		dino->TeleportTo(&pos, &rot, true, false);

		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully teleported dino");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void AddDinoExperience(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(3))
	{
		int dino_id1;
		int dino_id2;
		float how_much;

		try
		{
			dino_id1 = std::stoi(*parsed[1]);
			dino_id2 = std::stoi(*parsed[2]);
			how_much = std::stof(*parsed[3]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(ArkApi::GetApiUtils().GetWorld(), dino_id1,
		                                                                  dino_id2);
		if (!dino)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find dino");
			return;
		}

		dino->MyCharacterStatusComponentField()()->AddExperience(how_much, false, EXPType::XP_GENERIC);

		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully added experience to dino");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void KillDino(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		int dino_id1;
		int dino_id2;

		try
		{
			dino_id1 = std::stoi(*parsed[1]);
			dino_id2 = std::stoi(*parsed[2]);
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(ArkApi::GetApiUtils().GetWorld(), dino_id1,
		                                                                  dino_id2);
		if (!dino)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find dino");
			return;
		}

		dino->Suicide();

		SendRconReply(rcon_connection, rcon_packet->Id, "Killed dino");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void ClientChat(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L"'", true);

	if (parsed.IsValidIndex(2))
	{
		ArkApi::GetApiUtils().SendChatMessageToAll(parsed[2], *parsed[1]);

		SendRconReply(rcon_connection, rcon_packet->Id, "Sent message");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void UnlockEngram(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString msg = rcon_packet->Body;

	TArray<FString> parsed;
	msg.ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		unsigned __int64 steam_id;
		FString blueprint;

		try
		{
			steam_id = std::stoull(*parsed[1]);
			blueprint = parsed[2];
		}
		catch (const std::exception&)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Request has failed");
			return;
		}

		AShooterPlayerController* shooter_pc = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (!shooter_pc)
		{
			SendRconReply(rcon_connection, rcon_packet->Id, "Can't find player from the given steam id");
			return;
		}

		UShooterCheatManager* cheat_manager = static_cast<UShooterCheatManager*>(shooter_pc->CheatManagerField()());
		cheat_manager->UnlockEngram(&blueprint);

		SendRconReply(rcon_connection, rcon_packet->Id, "Successfully unlocked engram");
	}
	else
	{
		SendRconReply(rcon_connection, rcon_packet->Id, "Not enough arguments");
	}
}

void Load()
{
	auto& commands = ArkApi::GetCommands();

	commands.AddRconCommand("GiveItemNum", &GiveItemNum);
	commands.AddRconCommand("GiveItem", &GiveItem);
	commands.AddRconCommand("GiveItemToAll", &GiveItemToAll);
	commands.AddRconCommand("AddExperience", &AddExperience);
	commands.AddRconCommand("SetPlayerPos", &SetPlayerPos);
	commands.AddRconCommand("GetPlayerPos", &GetPlayerPos);
	commands.AddRconCommand("TeleportAllPlayers", &TeleportAllPlayers);
	commands.AddRconCommand("KillPlayer", &KillPlayer);
	commands.AddRconCommand("TeleportToPlayer", &TeleportToPlayer);
	commands.AddRconCommand("ListPlayerDinos", &ListPlayerDinos);
	commands.AddRconCommand("GetTribeIdOfPlayer", &GetTribeIdOfPlayer);
	commands.AddRconCommand("SpawnDino", &SpawnDino);
	commands.AddRconCommand("GetTribeLog", &GetTribeLog);
	commands.AddRconCommand("GetDinoPos", &GetDinoPos);
	commands.AddRconCommand("SetDinoPos", &SetDinoPos);
	commands.AddRconCommand("AddDinoExperience", &AddDinoExperience);
	commands.AddRconCommand("KillDino", &KillDino);
	commands.AddRconCommand("ClientChat", &ClientChat);
	commands.AddRconCommand("UnlockEngram", &UnlockEngram);
}

void Unload()
{
	auto& commands = ArkApi::GetCommands();

	commands.RemoveRconCommand("GiveItemNum");
	commands.RemoveRconCommand("GiveItem");
	commands.RemoveRconCommand("GiveItemToAll");
	commands.RemoveRconCommand("AddExperience");
	commands.RemoveRconCommand("SetPlayerPos");
	commands.RemoveRconCommand("GetPlayerPos");
	commands.RemoveRconCommand("TeleportAllPlayers");
	commands.RemoveRconCommand("KillPlayer");
	commands.RemoveRconCommand("TeleportToPlayer");
	commands.RemoveRconCommand("ListPlayerDinos");
	commands.RemoveRconCommand("GetTribeIdOfPlayer");
	commands.RemoveRconCommand("SpawnDino");
	commands.RemoveRconCommand("GetTribeLog");
	commands.RemoveRconCommand("GetDinoPos");
	commands.RemoveRconCommand("SetDinoPos");
	commands.RemoveRconCommand("AddDinoExperience");
	commands.RemoveRconCommand("KillDino");
	commands.RemoveRconCommand("ClientChat");
	commands.RemoveRconCommand("UnlockEngram");
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
