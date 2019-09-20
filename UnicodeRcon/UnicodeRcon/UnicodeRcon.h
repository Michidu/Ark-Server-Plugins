#pragma once

#include <windows.h>

#include <API/ARK/Ark.h>

#include "Other.h"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(RCONClientConnection_SendMessageW, void, RCONClientConnection*, int, int, FString*);
DECLARE_HOOK(RCONClientConnection_ProcessRCONPacket, void, RCONClientConnection*, RCONPacket *, UWorld *);
DECLARE_HOOK(FSocketBSD_Recv, bool, FSocketBSD*, char *, int, int *, ESocketReceiveFlags::Type);
DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*,
             AShooterCharacter*, bool);

inline bool loaded_rcon_port = false;
inline FString rcon_cmd_data;
inline int last_packet_id;
inline int rconport;

inline void LoadServerRconPort()
{
	if (ArkApi::GetApiUtils().GetShooterGameMode())
	{
		rconport = static_cast<FSocketBSD*>(ArkApi::GetApiUtils()
		                                    .GetShooterGameMode()->RCONSocketField()->SocketField())->GetPortNo();

		loaded_rcon_port = true;
	}
}
