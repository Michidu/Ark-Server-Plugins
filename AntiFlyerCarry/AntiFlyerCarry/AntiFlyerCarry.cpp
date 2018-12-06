#include "json.hpp"

#include <API/ARK/Ark.h>
#include <API/UE/Math/ColorList.h>

#include <fstream>

#pragma comment(lib, "ArkApi.lib")

nlohmann::json config;

DECLARE_HOOK(APrimalDinoCharacter_CanCarryCharacter, bool, APrimalDinoCharacter*, APrimalCharacter*);

bool Hook_APrimalDinoCharacter_CanCarryCharacter(APrimalDinoCharacter* _this, APrimalCharacter* can_carry_pawn)
{
	const int this_team = _this->TargetingTeamField();

	if (can_carry_pawn && this_team >= 50000)
	{
		const int carry_team = can_carry_pawn->TargetingTeamField();

		if (can_carry_pawn->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
		{
			const bool can_carry_wild_dino = config["CanCarryWildDino"];
			const bool can_carry_tamed_dino = config["CanCarryTamedDino"];

			if (!can_carry_tamed_dino && (carry_team < 50000 || carry_team >= 50000 && this_team != carry_team))
			{
				const bool can_carry_alliance_dino = config["CanCarryAllianceDino"];
				if (can_carry_alliance_dino)
				{
					FTribeData tribe;
					ArkApi::GetApiUtils().GetShooterGameMode()->GetTribeData(&tribe, carry_team);
					if (!tribe.IsTribeAlliedWith(this_team)) return false;
				} else return false;
			}
		}
		else
		{
			const bool can_carry_players = config["CanCarryPlayers"];
			if (!can_carry_players && this_team != carry_team)
			{
				const bool can_carry_alliance_players = config["CanCarryAlliancePlayers"];
				if (can_carry_alliance_players && can_carry_pawn->PlayerStateField() && can_carry_pawn->PlayerStateField()->IsA(AShooterPlayerState::StaticClass()))
				{
					AShooterPlayerState* Asps = static_cast<AShooterPlayerState*>(can_carry_pawn->PlayerStateField());
					if (!Asps->MyTribeDataField()->IsTribeAlliedWith(this_team)) return false;
				} else return false;
			}
		}
	}

	return APrimalDinoCharacter_CanCarryCharacter_original(_this, can_carry_pawn);
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AntiFlyerCarry/config.json";
	std::ifstream file{config_path};
	if (!file.is_open())
		throw std::runtime_error("Can't open config.json");

	file >> config;

	file.close();
}

void ReloadConfig(APlayerController* player_controller, FString*, bool)
{
	AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Failed to reload config");

		Log::GetLog()->error(error.what());
		return;
	}

	ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Reloaded config");
}

void ReloadConfigRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString reply;

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());

		reply = error.what();
		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
		return;
	}

	reply = "Reloaded config";
	rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
}

void Load()
{
	Log::Get().Init("AntiFlyerCarry");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	ArkApi::GetCommands().AddConsoleCommand("AntiCarry.Reload", &ReloadConfig);
	ArkApi::GetCommands().AddRconCommand("AntiCarry.Reload", &ReloadConfigRcon);

	ArkApi::GetHooks().SetHook("APrimalDinoCharacter.CanCarryCharacter", &Hook_APrimalDinoCharacter_CanCarryCharacter,
	                           &APrimalDinoCharacter_CanCarryCharacter_original);
}

void Unload()
{
	ArkApi::GetCommands().RemoveConsoleCommand("AntiCarry.Reload");
	ArkApi::GetCommands().RemoveRconCommand("AntiCarry.Reload");

	ArkApi::GetHooks().DisableHook("APrimalDinoCharacter.CanCarryCharacter", &Hook_APrimalDinoCharacter_CanCarryCharacter);
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
