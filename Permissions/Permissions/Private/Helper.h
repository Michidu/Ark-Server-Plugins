#pragma once

namespace Permissions
{
	inline std::string GetDbPath()
	{
#ifdef PERMISSIONS_ARK
		return ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/Permissions/ArkDB.db";
#else
		return ArkApi::Tools::GetCurrentDir() + "/AtlasApi/Plugins/Permissions/AtlasDB.db";
#endif
	}

	inline std::string GetConfigPath()
	{
#ifdef PERMISSIONS_ARK
		return ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/Permissions/config.json";
#else
		return ArkApi::Tools::GetCurrentDir() + "/AtlasApi/Plugins/Permissions/config.json";
#endif
	}

	inline void SendRconReply(RCONClientConnection* rcon_connection, int packet_id, const FString& msg)
	{
		FString reply = msg + "\n";
		rcon_connection->SendMessageW(packet_id, 0, &reply);
	}
}
