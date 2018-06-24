#include "Stats.h"

namespace Stats
{
	bool IsPlayerExists(uint64 steam_id)
	{
		int count = 0;

		try
		{
			auto& db = GetDB();

			db << "SELECT count(1) FROM Players WHERE SteamId = ?;" << steam_id >> count;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return count != 0;
	}

	bool AddPlayerKill(uint64 steam_id)
	{
		try
		{
			auto& db = GetDB();

			if (IsPlayerExists(steam_id))
			{
				db << "UPDATE Players SET PlayerKills = PlayerKills + 1 WHERE SteamId = ?;" << steam_id;
			}
			else
			{
				FString name = "";

				AShooterPlayerController* controller = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
				if (controller && controller->GetPlayerCharacter())
					name = controller->GetPlayerCharacter()->PlayerNameField();

				db << "INSERT INTO Players (SteamId, Name, PlayerKills) VALUES (?, ?, 1);" << steam_id
					<< ArkApi::Tools::Utf8Encode(*name);
			}
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
	}

	bool AddPlayerDeath(uint64 steam_id)
	{
		try
		{
			auto& db = GetDB();

			if (IsPlayerExists(steam_id))
			{
				db << "UPDATE Players SET PlayerDeaths = PlayerDeaths + 1 WHERE SteamId = ?;" << steam_id;
			}
			else
			{
				FString name = "";

				AShooterPlayerController* controller = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
				if (controller && controller->GetPlayerCharacter())
					name = controller->GetPlayerCharacter()->PlayerNameField();

				db << "INSERT INTO Players (SteamId, Name, PlayerDeaths) VALUES (?, ?, 1);" << steam_id
					<< ArkApi::Tools::Utf8Encode(*name);
			}
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
	}

	bool AddWildDinoKill(uint64 steam_id)
	{
		try
		{
			auto& db = GetDB();

			if (IsPlayerExists(steam_id))
			{
				db << "UPDATE Players SET WildDinoKills = WildDinoKills + 1 WHERE SteamId = ?;" << steam_id;
			}
			else
			{
				FString name = "";

				AShooterPlayerController* controller = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
				if (controller && controller->GetPlayerCharacter())
					name = controller->GetPlayerCharacter()->PlayerNameField();

				db << "INSERT INTO Players (SteamId, Name, WildDinoKills) VALUES (?, ?, 1);" << steam_id
					<< ArkApi::Tools::Utf8Encode(*name);
			}
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
	}

	bool AddTamedDinoKill(uint64 steam_id)
	{
		try
		{
			auto& db = GetDB();

			if (IsPlayerExists(steam_id))
			{
				db << "UPDATE Players SET TamedDinoKills = TamedDinoKills + 1 WHERE SteamId = ?;" << steam_id;
			}
			else
			{
				FString name = "";

				AShooterPlayerController* controller = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
				if (controller && controller->GetPlayerCharacter())
					name = controller->GetPlayerCharacter()->PlayerNameField();

				db << "INSERT INTO Players (SteamId, Name, TamedDinoKills) VALUES (?, ?, 1);" << steam_id
					<< ArkApi::Tools::Utf8Encode(*name);
			}
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
	}

	void ShowMyStats(AShooterPlayerController* player_controller, FString*, EChatSendMode::Type)
	{
		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		try
		{
			auto& db = GetDB();

			int player_kills = 0;
			int player_deaths = 0;
			int wild_dino_kills = 0;
			int tamed_dino_kills = 0;

			if (IsPlayerExists(steam_id))
			{
				db << "SELECT PlayerKills, PlayerDeaths, WildDinoKills, TamedDinoKills FROM Players WHERE SteamId = ?;" << steam_id
					>> std::tie(player_kills, player_deaths, wild_dino_kills, tamed_dino_kills);
			}

			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("MyStats"), player_kills,
			                                      player_deaths, wild_dino_kills, tamed_dino_kills);
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	void ShowLeaderboardDinos(AShooterPlayerController* player_controller)
	{
		const float display_time = config["Stats"].value("DisplayTime", 15.0f);
		const float text_size = config["Stats"].value("TextSize", 1.3f);

		FString str = "";

		try
		{
			int i = 1;

			auto& db = GetDB();

			auto res = db <<
				"SELECT Name, TamedDinoKills, WildDinoKills FROM Players ORDER BY TamedDinoKills DESC, WildDinoKills DESC LIMIT 10;";
			res >> [&str, &i](const std::string& name, int tamed_dino_kills, int wild_dino_kills)
			{
				str += FString::Format(*GetText("StatsFormat"), i++, ArkApi::Tools::Utf8Decode(name).c_str(), tamed_dino_kills,
				                       wild_dino_kills);
			};
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		const FString list_str = FString::Format(*GetText("StatsFormatDino"), *str);

		ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::White, text_size, display_time, nullptr,
		                                       *list_str);
	}

	void ShowLeaderboardPlayers(AShooterPlayerController* player_controller)
	{
		const float display_time = config["Stats"].value("DisplayTime", 15.0f);
		const float text_size = config["Stats"].value("TextSize", 1.3f);

		FString str = "";

		try
		{
			int i = 1;

			auto& db = GetDB();

			auto res = db <<
				"SELECT Name, PlayerKills, PlayerDeaths FROM Players ORDER BY PlayerKills DESC, PlayerDeaths ASC LIMIT 10;";
			res >> [&str, &i](const std::string& name, int player_kills, int player_deaths)
			{
				str += FString::Format(*GetText("StatsFormat"), i++, ArkApi::Tools::Utf8Decode(name).c_str(), player_kills,
				                       player_deaths);
			};
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		const FString list_str = FString::Format(*GetText("StatsFormatPlayer"), *str);

		ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::White, text_size, display_time, nullptr,
		                                       *list_str);
	}

	void ShowLeaderboard(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
	{
		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			const FString type = parsed[1];

			if (type == L"dino")
				ShowLeaderboardDinos(player_controller);
			else if (type == L"player")
				ShowLeaderboardPlayers(player_controller);
			else
				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("StatsUsage"));
		}

		ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("StatsUsage"));
	}

	void Init()
	{
		auto& commands = ArkApi::GetCommands();

		commands.AddChatCommand(GetText("MyStatsCmd"), &ShowMyStats);
		commands.AddChatCommand(GetText("ShowLeaderboardCmd"), &ShowLeaderboard);
	}
}
