#include "Points.h"

#include "ArkShop.h"
#include "DBHelper.h"

namespace ArkShop::Points
{
	// Public functions

	bool AddPoints(int amount, uint64 steam_id)
	{
		if (amount <= 0)
			return false;

		auto& db = GetDB();

		try
		{
			db << "UPDATE Players SET Points = Points + ? WHERE SteamId = ?;" << amount << steam_id;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		AShooterPlayerController* player = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (player)
		{
			ArkApi::GetApiUtils().SendChatMessage(player, GetText("Sender"),
			                                      *GetText("ReceivedPoints"),
			                                      amount,
			                                      GetPoints(steam_id));
		}

		return true;
	}

	bool SpendPoints(int amount, uint64 steam_id)
	{
		if (amount <= 0)
			return false;

		auto& db = GetDB();

		try
		{
			db << "UPDATE Players SET Points = Points - ? WHERE SteamId = ?;" << amount << steam_id;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
	}

	int GetPoints(uint64 steam_id)
	{
		auto& db = GetDB();

		int points = 0;

		try
		{
			db << "SELECT Points FROM Players WHERE SteamId = ?;" << steam_id >> points;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return points;
	}

	bool SetPoints(uint64 steam_id, int new_amount)
	{
		auto& db = GetDB();

		try
		{
			db << "UPDATE Players SET Points = ? WHERE SteamId = ?;" << new_amount << steam_id;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
	}

	// Chat callbacks

	/**
	 * \brief Send points to the other player (using character name)
	 */
	void Trade(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
	{
		const uint64 sender_steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		if (DBHelper::IsPlayerExists(sender_steam_id))
		{
			TArray<FString> parsed;
			message->ParseIntoArray(parsed, L"'", true);

			if (parsed.IsValidIndex(2))
			{
				const FString receiver_name = parsed[1];

				int amount;

				try
				{
					amount = std::stoi(*parsed[2]);
				}
				catch (const std::exception& exception)
				{
					Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
					return;
				}

				if (amount <= 0)
					return;

				if (GetPoints(sender_steam_id) < amount)
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					                                      *GetText("NoPoints"));
					return;
				}

				TArray<AShooterPlayerController*> receiver_players = ArkApi::GetApiUtils().
					FindPlayerFromCharacterName(receiver_name);

				if (receiver_players.Num() > 1)
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					                                      *GetText("FoundMorePlayers"));
					return;
				}

				if (receiver_players.Num() < 1)
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					                                      *GetText("NoPlayer"));
					return;
				}

				AShooterPlayerController* receiver_player = receiver_players[0];

				const uint64 receiver_steam_id = ArkApi::IApiUtils::GetSteamIdFromController(receiver_player);
				if (receiver_steam_id == sender_steam_id)
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					                                      *GetText("CantGivePoints"));
					return;
				}

				if (DBHelper::IsPlayerExists(receiver_steam_id) && SpendPoints(amount, sender_steam_id) && AddPoints(
					amount, receiver_steam_id))
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					                                      *GetText("SentPoints"), amount, *receiver_name);

					const FString sender_name = ArkApi::IApiUtils::GetCharacterName(player_controller);

					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					                                      *GetText("GotPoints"), amount, *sender_name);
				}
			}
			else
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
				                                      *GetText("TradeUsage"));
			}
		}
	}

	void PrintPoints(AShooterPlayerController* player_controller, FString*, EChatSendMode::Type)
	{
		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		if (DBHelper::IsPlayerExists(steam_id))
		{
			int points = GetPoints(steam_id);

			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("HavePoints"), points);
		}
	}

	// Console callbacks

	void AddPointsCmd(APlayerController*, FString* cmd, bool)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(2))
		{
			uint64 steam_id;
			int amount;

			try
			{
				steam_id = std::stoull(*parsed[1]);
				amount = std::stoi(*parsed[2]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return;
			}

			if (DBHelper::IsPlayerExists(steam_id))
			{
				AddPoints(amount, steam_id);
			}
		}
	}

	void SetPointsCmd(APlayerController*, FString* cmd, bool)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(2))
		{
			uint64 steam_id;
			int amount;

			try
			{
				steam_id = std::stoull(*parsed[1]);
				amount = std::stoi(*parsed[2]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return;
			}

			if (DBHelper::IsPlayerExists(steam_id))
			{
				SetPoints(steam_id, amount);
			}
		}
	}

	/**
	 * \brief Reset points for all players
	 */
	void ResetPointsCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		if (parsed.IsValidIndex(1))
		{
			if (parsed[1].ToString() == "confirm")
			{
				auto& db = GetDB();

				db << "UPDATE Players SET Points = 0;";

				ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully reset points");
			}
		}
		else
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Yellow,
			                                        "You are going to reset points for ALL players\nType 'ResetPoints confirm' in console if you want to continue");
		}
	}

	void GetPlayerPointsCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			uint64 steam_id;

			try
			{
				steam_id = std::stoull(*parsed[1]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return;
			}

			if (DBHelper::IsPlayerExists(steam_id))
			{
				int points = GetPoints(steam_id);

				AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

				ArkApi::GetApiUtils().SendChatMessage(shooter_controller, GetText("Sender"), "Player has {} points", points);
			}
		}
	}

	// Rcon callbacks

	void AddPointsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		FString msg = rcon_packet->Body;

		TArray<FString> parsed;
		msg.ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(2))
		{
			uint64 steam_id;
			int amount;

			try
			{
				steam_id = std::stoull(*parsed[1]);
				amount = std::stoi(*parsed[2]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return;
			}

			if (DBHelper::IsPlayerExists(steam_id))
			{
				AddPoints(amount, steam_id);

				FString reply("Successfully added points\n");
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
			}
		}
	}

	void SetPointsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		FString msg = rcon_packet->Body;

		TArray<FString> parsed;
		msg.ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(2))
		{
			uint64 steam_id;
			int amount;

			try
			{
				steam_id = std::stoull(*parsed[1]);
				amount = std::stoi(*parsed[2]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return;
			}

			if (DBHelper::IsPlayerExists(steam_id))
			{
				SetPoints(steam_id, amount);

				FString reply("Successfully set points\n");
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
			}
		}
	}

	void GetPlayerPointsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
	{
		FString msg = rcon_packet->Body;

		TArray<FString> parsed;
		msg.ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			uint64 steam_id;

			try
			{
				steam_id = std::stoull(*parsed[1]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return;
			}

			if (DBHelper::IsPlayerExists(steam_id))
			{
				const int points = GetPoints(steam_id);

				FString reply = FString::Format("Player has {} points\n", points);
				rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
			}
		}
	}

	void Init()
	{
		auto& commands = ArkApi::GetCommands();

		commands.AddChatCommand(GetText("PointsCmd"), &PrintPoints);
		commands.AddChatCommand(GetText("TradeCmd"), &Trade);

		commands.AddConsoleCommand("AddPoints", &AddPointsCmd);
		commands.AddConsoleCommand("SetPoints", &SetPointsCmd);
		commands.AddConsoleCommand("GetPlayerPoints", &GetPlayerPointsCmd);
		commands.AddConsoleCommand("ResetPoints", &ResetPointsCmd);

		commands.AddRconCommand("AddPoints", &AddPointsRcon);
		commands.AddRconCommand("SetPoints", &SetPointsRcon);
		commands.AddRconCommand("GetPlayerPoints", &GetPlayerPointsRcon);
	}
}
