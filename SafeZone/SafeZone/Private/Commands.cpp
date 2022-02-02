#include "Commands.h"

#include "SafeZones.h"
#include "SafeZoneManager.h"

#include "SzTools.h"

#include <fstream>

namespace SafeZones::Commands
{
	nlohmann::basic_json<> FindZoneConfigByName(const std::string& name)
	{
		nlohmann::basic_json<> safezone_config;

		auto safe_zones = config.value("SafeZones", nlohmann::json::array());
		for (const auto& safe_zone : safe_zones)
		{
			if (safe_zone.value("Name", "") == name)
			{
				safezone_config = safe_zone;
				break;
			}
		}

		return safezone_config;
	}

	void SZGiveItems(APlayerController*, FString* cmd, bool)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			parsed.RemoveAt(0);
			FString name = FString::Join(parsed, L" ");

			nlohmann::basic_json<> safezone_config = FindZoneConfigByName(name.ToString());
			if (safezone_config.empty())
				return;

			const auto& items_entry = safezone_config.value("ItemsConfig", nlohmann::json::object());

			const auto safe_zone = SafeZoneManager::Get().FindZoneByName(name);
			if (!safe_zone)
				return;

			for (AActor* actor : safe_zone->GetActorsInsideZone())
			{
				if (!actor->IsA(AShooterCharacter::GetPrivateStaticClass())
					&& !actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
					continue;

				AShooterPlayerController* player = nullptr;
				
				if (actor->IsA(AShooterCharacter::GetPrivateStaticClass()))
				{
					player = ArkApi::GetApiUtils().FindControllerFromCharacter(
						static_cast<AShooterCharacter*>(actor));
				}
				else
				{
					AShooterCharacter* rider = static_cast<APrimalDinoCharacter*>(actor)->RiderField().Get();
					if (rider)
					{
						player = ArkApi::GetApiUtils().FindControllerFromCharacter(rider);
					}
				}

				if (player)
				{
					// Give items
					auto items_map = items_entry.value("Items", nlohmann::json::array());
					for (const auto& item : items_map)
					{
						const int amount = item["Amount"];
						const float quality = item["Quality"];
						const bool force_blueprint = item["ForceBlueprint"];
						std::string blueprint = item["Blueprint"];

						TArray<UPrimalItem*> out;
						FString fblueprint(blueprint.c_str());

						player->GiveItem(&out, &fblueprint, amount, quality, force_blueprint, false, quality);
					}

					// Give dinos
					auto dinos_map = items_entry.value("Dinos", nlohmann::json::array());
					for (const auto& dino : dinos_map)
					{
						const int level = dino["Level"];
						std::string blueprint = dino["Blueprint"];
						const bool neuter = dino.value("Neutered", false);
						const std::string saddle = dino.value("SaddleBlueprint", "");

						const FString fblueprint(blueprint.c_str());

						if (!safe_zone->cryopod_dinos)
							ArkApi::GetApiUtils().SpawnDino(player, fblueprint, nullptr, level, true, neuter);
						else
							SafeZones::Tools::GiveDino(player, level, neuter, blueprint, saddle);
					}
				}
			}
		}
	}
	void SZGiveItems_RCON(RCONClientConnection* c, RCONPacket* p, UWorld*)
	{
		TArray<FString> parsed;
		p->Body.ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			parsed.RemoveAt(0);
			FString name = FString::Join(parsed, L" ");

			nlohmann::basic_json<> safezone_config = FindZoneConfigByName(name.ToString());
			if (safezone_config.empty())
				return;

			const auto& items_entry = safezone_config.value("ItemsConfig", nlohmann::json::object());

			const auto safe_zone = SafeZoneManager::Get().FindZoneByName(name);
			if (!safe_zone)
				return;

			for (AActor* actor : safe_zone->GetActorsInsideZone())
			{
				if (!actor->IsA(AShooterCharacter::GetPrivateStaticClass())
					&& !actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
					continue;

				AShooterPlayerController* player = nullptr;
				
				if (actor->IsA(AShooterCharacter::GetPrivateStaticClass()))
				{
					player = ArkApi::GetApiUtils().FindControllerFromCharacter(
						static_cast<AShooterCharacter*>(actor));
				}
				else
				{
					AShooterCharacter* rider = static_cast<APrimalDinoCharacter*>(actor)->RiderField().Get();
					if (rider)
					{
						player = ArkApi::GetApiUtils().FindControllerFromCharacter(rider);
					}
				}

				if (player)
				{
					// Give items
					auto items_map = items_entry.value("Items", nlohmann::json::array());
					for (const auto& item : items_map)
					{
						const int amount = item["Amount"];
						const float quality = item["Quality"];
						const bool force_blueprint = item["ForceBlueprint"];
						std::string blueprint = item["Blueprint"];

						TArray<UPrimalItem*> out;
						FString fblueprint(blueprint.c_str());

						player->GiveItem(&out, &fblueprint, amount, quality, force_blueprint, false, quality);
					}

					// Give dinos
					auto dinos_map = items_entry.value("Dinos", nlohmann::json::array());
					for (const auto& dino : dinos_map)
					{
						const int level = dino["Level"];
						std::string blueprint = dino["Blueprint"];
						const bool neuter = dino.value("Neutered", false);
						const std::string saddle = dino.value("SaddleBlueprint", "");

						const FString fblueprint(blueprint.c_str());

						if (!safe_zone->cryopod_dinos)
							ArkApi::GetApiUtils().SpawnDino(player, fblueprint, nullptr, level, true, neuter);
						else
							SafeZones::Tools::GiveDino(player, level, neuter, blueprint, saddle);
					}
				}
			}
		}
	}

	void SZEnterSettings(APlayerController*, FString* cmd, bool)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(3))
		{
			const FString name = parsed[1];

			const auto safe_zone = SafeZoneManager::Get().FindZoneByName(name);
			if (!safe_zone)
				return;

			bool prevent_entering;
			bool prevent_leaving;

			try
			{
				prevent_entering = std::stoi(*parsed[2]) != 0;
				prevent_leaving = std::stoi(*parsed[3]) != 0;
			}
			catch (const std::exception&)
			{
				return;
			}

			safe_zone->prevent_entering = prevent_entering;
			safe_zone->prevent_leaving = prevent_leaving;
		}
	}

	void SZReloadConfig(APlayerController* player_controller, FString* cmd, bool)
	{
		AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		try
		{
			ReadConfig();
		}
		catch (const std::exception& error)
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, {1, 0, 0}, "Failed to reload config");

			Log::GetLog()->error(error.what());
			return;
		}

		auto& default_safe_zones = SafeZoneManager::Get().GetDefaultSafeZones();
		for (const auto& name : default_safe_zones)
		{
			SafeZoneManager::Get().RemoveSafeZone(name);
		}

		SafeZoneManager::Get().ClearAllTriggerSpheres();
		SafeZoneManager::Get().ReadSafeZones();

		ArkApi::GetApiUtils().SendServerMessage(shooter_controller, {0, 1, 0}, "Reloaded config");
	}
	void SZReloadConfig_RCON(RCONClientConnection* c, RCONPacket* p, UWorld*)
	{
		FString rep = L"Failed to reload config";

		try
		{
			ReadConfig();
		}
		catch (const std::exception& error)
		{
			c->SendMessageW(p->Id, 0, &rep);

			Log::GetLog()->error(error.what());
			return;
		}

		auto& default_safe_zones = SafeZoneManager::Get().GetDefaultSafeZones();
		for (const auto& name : default_safe_zones)
		{
			SafeZoneManager::Get().RemoveSafeZone(name);
		}

		SafeZoneManager::Get().ClearAllTriggerSpheres();
		SafeZoneManager::Get().ReadSafeZones();

		rep = L"Reloaded config";
		c->SendMessageW(p->Id, 0, &rep);
	}

	nlohmann::ordered_json default_config_zone;
	void SZCreateZone(APlayerController* PC, FString* cmd, bool)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			try
			{
				parsed.RemoveAt(0);
				const FString name = FString::Join(parsed, L" ");

				auto new_zone = default_config_zone;
				new_zone["Name"] = name.ToString();

				const FVector pos = ArkApi::GetApiUtils().GetPosition(PC);
				std::vector<float> pos_vec{ pos.X, pos.Y, pos.Z };

				new_zone["Position"] = pos_vec;
				FString map_name;
				ArkApi::GetApiUtils().GetShooterGameMode()->GetMapName(&map_name);

				new_zone["ForMap"] = map_name.ToString();

				const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/SafeZone/config.json";
				std::ofstream file{ config_path };

				config["SafeZones"].push_back(new_zone);

				file << config.dump(1, '	');

				file.close();

				ReadConfig();
			}
			catch (const std::exception& error)
			{
				ArkApi::GetApiUtils().SendServerMessage((AShooterPlayerController*)PC, { 1, 0, 0 }, "Failed to create zone");

				Log::GetLog()->error(error.what());
				return;
			}
		}
	}

	void SZSpawnPoints_Console(APlayerController* PC, FString* cmd, bool)
	{
		if (PC)
		{
			TArray<FString>* spawnPoints = UPrimalGameData::BPGetGameData()->GetPlayerSpawnRegions(ArkApi::GetApiUtils().GetWorld());

			for (auto& zone : *spawnPoints)
				ArkApi::GetApiUtils().SendChatMessage((AShooterPlayerController*)PC, "SafeZones", *zone);
		}
	}
	void SZSpawnPoints_Rcon(RCONClientConnection* c, RCONPacket* p, UWorld*)
	{
		if (c
			&& p)
		{
			TArray<FString>* spawnPoints = UPrimalGameData::BPGetGameData()->GetPlayerSpawnRegions(ArkApi::GetApiUtils().GetWorld());

			for (auto& zone : *spawnPoints)
				c->SendMessageW(p->Id, 0, &zone);
		}
	}

	void Init()
	{
		// Default json for creating zones with command
		auto& new_zone = default_config_zone;
		new_zone["Name"] = "";
		new_zone["Position"] = std::vector<float>{};
		new_zone["ForMap"] = "";
		new_zone["Radius"] = 1800;
		new_zone["PreventPVP"] = false;
		new_zone["PreventStructureDamage"] = false;
		new_zone["PreventFriendlyFire"] = false;
		new_zone["PreventWildDinoDamage"] = false;
		new_zone["PreventBuilding"] = false;
		new_zone["KillWildDinos"] = false;
		new_zone["OnlyKillAggressiveDinos"] = false;
		new_zone["PreventLeaving"] = false;
		new_zone["PreventEntering"] = false;
		new_zone["EnableEvents"] = false;
		new_zone["ScreenNotifications"] = false;
		new_zone["ChatNotifications"] = false;
		new_zone["GiveDinosInCryopod"] = false;
		new_zone["ShowBubble"] = false;
		new_zone["BubbleColor"] = std::vector<float>{ 1, 0, 0 };
		new_zone["SuccessNotificationColor"] = std::vector<float>{
				0,
				1,
				0,
				1
		};
		new_zone["FailNotificationColor"] = std::vector<float>{
			  1,
			  0,
			  0,
			  1
		};
		new_zone["Messages"] = {
			"You have entered {0}",
			"You have left {0}",
			"You can't build here",
			"You are not allowed to enter",
			"You are not allowed to leave",
		};

		auto& commands = ArkApi::GetCommands();

		commands.AddConsoleCommand("SZGiveItems", &SZGiveItems);
		commands.AddRconCommand("SZGiveItems", &SZGiveItems_RCON);
		commands.AddConsoleCommand("SZEnterSettings", &SZEnterSettings);
		commands.AddConsoleCommand("SZReloadConfig", &SZReloadConfig);
		commands.AddRconCommand("SZReloadConfig", &SZReloadConfig_RCON);
		commands.AddConsoleCommand("SZCreateZone", &SZCreateZone);
		commands.AddConsoleCommand("SZShowSpawnPoints", &SZSpawnPoints_Console);
		commands.AddRconCommand("SZShowSpawnPoints", &SZSpawnPoints_Rcon);
	}

	void Clean()
	{
		auto& commands = ArkApi::GetCommands();

		commands.RemoveConsoleCommand("SZGiveItems");
		commands.RemoveRconCommand("SZGiveItems");
		commands.RemoveConsoleCommand("SZEnterSettings");
		commands.RemoveConsoleCommand("SZReloadConfig");
		commands.RemoveRconCommand("SZReloadConfig");
		commands.RemoveConsoleCommand("SZCreateZone");
		commands.RemoveRconCommand("SZShowSpawnPoints");
		commands.RemoveConsoleCommand("SZShowSpawnPoints");
	}
}
