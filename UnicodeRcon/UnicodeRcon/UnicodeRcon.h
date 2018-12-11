#pragma once
#include <windows.h>
#include <API/ARK/Ark.h>
#include "Other.h"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(RCONClientConnection_SendMessageW, void, RCONClientConnection*, int, int, FString*);
DECLARE_HOOK(RCONClientConnection_ProcessRCONPacket, void, RCONClientConnection*, RCONPacket *, UWorld *);
DECLARE_HOOK(FSocketBSD_Recv, bool, FSocketBSD*, char *, int, int *, ESocketReceiveFlags::Type);
DECLARE_HOOK(AShooterGameMode_HandleNewPlayer, bool, AShooterGameMode*, AShooterPlayerController*, UPrimalPlayerData*, AShooterCharacter*, bool);

bool LoadedRconPort = false;
FString RconCMDData;
int RCONPORT;

void LoadServerRconPort()
{
	if (ArkApi::GetApiUtils().GetShooterGameMode())
	{
		FInternetAddrBSD* hsdf = static_cast<FInternetAddrBSD*>(ArkApi::GetApiUtils().GetShooterGameMode()->RCONSocketField()->ListenAddrField().Get());
		if (hsdf)
		{
			hsdf->GetPort(&RCONPORT);
			LoadedRconPort = true;
		}
	}
}
