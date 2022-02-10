#include "Teleport.h"
#include "Helper.h"
#include "HomeSystem.h"

namespace Teleport
{
	void SetTpCooldown(uint64 steam_id, int64 cooldown)
	{
		auto& db = ArkHome::GetDB();

		try
		{
			SQLite::Statement query(db, "UPDATE Players SET TeleportCooldown = ? WHERE SteamId = ?;");
			query.bind(1, cooldown);
			query.bind(2, static_cast<int64>(steam_id));
			query.exec();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	int64 GetTpCooldown(uint64 steam_id)
	{
		auto& db = ArkHome::GetDB();

		int64 cooldown = 0;

		try
		{
			SQLite::Statement query(db, "SELECT TeleportCooldown FROM Players WHERE SteamId = ?;");
			query.bind(1, static_cast<int64>(steam_id));
			query.executeStep();

			cooldown = query.getColumn(0).getInt64();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return cooldown;
	}

	void DoTp(AShooterPlayerController* player_controller, const FVector& pos)
	{
		if (!player_controller || ArkApi::IApiUtils::IsPlayerDead(player_controller))
			return;

		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		const auto player_iter = find(HomeSystem::teleporting_players.begin(), HomeSystem::teleporting_players.end(),
		                              steam_id);
		if (player_iter == HomeSystem::teleporting_players.end())
			return;

		HomeSystem::teleporting_players.erase(
			remove(HomeSystem::teleporting_players.begin(), HomeSystem::teleporting_players.end(), *player_iter),
			HomeSystem::teleporting_players.end());

		player_controller->SetPlayerPos(pos[0], pos[1], pos[2]);

		Helper::DisableInput(player_controller, false);

		const int tp_cooldown = ArkHome::config["General"].value("TeleportCooldown", 1);
		const auto cooldown = std::chrono::system_clock::to_time_t(
			std::chrono::system_clock::now() + std::chrono::minutes(tp_cooldown));

		SetTpCooldown(steam_id, cooldown);

		ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
		                                      *ArkHome::GetText("HomeTeleported"));
	}

	void TpCmd(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
	{
		if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
			return;

		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			std::string name = parsed[1].ToString();

			const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

			if (!Helper::IsPlayerExists(steam_id))
				HomeSystem::AddPlayer(steam_id);

			const auto teleport_poses = ArkHome::config["Teleport"];

			const auto tp_json_iter = teleport_poses.find(name);
			if (tp_json_iter == teleport_poses.end())
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("TpNameNotExists"));
				return;
			}

			const auto cooldown = std::chrono::system_clock::from_time_t(GetTpCooldown(steam_id));
			if (std::chrono::system_clock::now() < cooldown)
			{
				auto time_left = std::chrono::duration_cast<std::chrono::seconds>(
					cooldown - std::chrono::system_clock::now()).count();

				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("HomeCooldown"), time_left);
				return;
			}

			const int enemy_min_distance = ArkHome::config["General"].value("EnemyStructureMinDistance", 10000);
			if (HomeSystem::IsEnemyStructureNear(player_controller, enemy_min_distance))
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("CantTpNearEnemy"));
				return;
			}

			auto pos = teleport_poses[name];

			const int delay = ArkHome::config["General"].value("TeleportDelay", 15);

			Helper::Timer(delay * 1000, true, steam_id, &DoTp, player_controller, FVector(pos[0], pos[1], pos[2]));

			HomeSystem::teleporting_players.push_back(steam_id);

			Helper::DisableInput(player_controller, true);

			ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
			                                      *ArkHome::GetText("HomeTeleportStart"), delay);
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
			                                      *ArkHome::GetText("TpUsage"));
		}
	}

	void Init()
	{
		auto& commands = ArkApi::GetCommands();

		commands.AddChatCommand(ArkHome::GetText("TpCmd"), &TpCmd);
	}
}
