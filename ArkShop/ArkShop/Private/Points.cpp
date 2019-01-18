#include <Points.h>

#include <DBHelper.h>

#include "ArkShop.h"

namespace ArkShop::Points
{
	// Public functions

	bool AddPoints(int amount, uint64 steam_id)
	{
		if (amount <= 0)
		{
			return false;
		}

		const bool is_added = database->AddPoints(steam_id, amount);
		if (!is_added)
		{
			return false;
		}

		AShooterPlayerController* player = ArkApi::GetApiUtils().FindPlayerFromSteamId(steam_id);
		if (player != nullptr)
		{
			ArkApi::GetApiUtils().SendChatMessage(player, GetText("Sender"), *GetText("ReceivedPoints"), amount,
			                                      GetPoints(steam_id));
		}

		return true;
	}

	bool SpendPoints(int amount, uint64 steam_id)
	{
		if (amount <= 0)
		{
			return false;
		}

		const bool is_spend = database->SpendPoints(steam_id, amount);
		if (!is_spend)
		{
			return false;
		}

		//database->AddTotalSpent(steam_id, amount);

		return true;
	}

	int GetPoints(uint64 steam_id)
	{
		return database->GetPoints(steam_id);
	}

	int GetTotalSpent(uint64 steam_id)
	{
		return database->GetTotalSpent(steam_id);
	}

	bool SetPoints(uint64 steam_id, int new_amount)
	{
		const bool is_spend = database->SetPoints(steam_id, new_amount);
		return is_spend;
	}

	// Chat callbacks

	/**
	 * \brief Send points to the other player (using character name)
	 */
	void Trade(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type /*unused*/)
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
				{
					return;
				}

				if (GetPoints(sender_steam_id) < amount)
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					                                      *GetText("NoPoints"));
					return;
				}

				TArray<AShooterPlayerController*> receiver_players = ArkApi::GetApiUtils().
					FindPlayerFromCharacterName(receiver_name, ESearchCase::IgnoreCase, false);

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

	void PrintPoints(AShooterPlayerController* player_controller, FString* /*unused*/, EChatSendMode::Type /*unused*/)
	{
		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		if (DBHelper::IsPlayerExists(steam_id))
		{
			int points = GetPoints(steam_id);

			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("HavePoints"), points);
		}
	}

	// Callbacks

	bool AddPointsCbk(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

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
				return false;
			}

			return DBHelper::IsPlayerExists(steam_id) && AddPoints(amount, steam_id);
		}

		return false;
	}

	bool SetPointsCbk(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

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
				return false;
			}

			return DBHelper::IsPlayerExists(steam_id) && SetPoints(steam_id, amount);
		}

		return false;
	}

	bool ChangePointsAmountCbk(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

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
				return false;
			}

			if (DBHelper::IsPlayerExists(steam_id))
			{
				return amount >= 0
					       ? AddPoints(amount, steam_id)
					       : SpendPoints(std::abs(amount), steam_id);
			}
		}

		return false;
	}

	int GetPlayerPointsCbk(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

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
				return -1;
			}

			if (DBHelper::IsPlayerExists(steam_id))
			{
				return GetPoints(steam_id);
			}
		}

		return -1;
	}

	// Console commands

	void AddPointsCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = AddPointsCbk(*cmd);
		if (result)
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully added points");
		}
		else
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't add points");
		}
	}

	void SetPointsCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = SetPointsCbk(*cmd);
		if (result)
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully set points");
		}
		else
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't set points");
		}
	}

	void ChangePointsAmountCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const bool result = ChangePointsAmountCbk(*cmd);
		if (result)
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully set points");
		}
		else
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't set points");
		}
	}

	/**
	 * \brief Reset points for all players
	 */
	void ResetPointsCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		if (parsed.IsValidIndex(1))
		{
			if (parsed[1].ToString() == "confirm")
			{
				database->DeleteAllPoints();

				ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green,
				                                        "Successfully reset points");
			}
		}
		else
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Yellow,
			                                        "You are going to reset points for ALL players\nType 'ResetPoints confirm' in console if you want to continue");
		}
	}

	void GetPlayerPointsCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		const int points = GetPlayerPointsCbk(*cmd);
		if (points != -1)
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Player has {} points",
			                                        points);
		}
		else
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't get points amount");
		}
	}

	// Rcon callbacks

	void AddPointsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
	{
		FString reply;

		const bool result = AddPointsCbk(rcon_packet->Body);
		if (result)
		{
			reply = "Successfully added points\n";
		}
		else
		{
			reply = "Couldn't add points\n";
		}

		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
	}

	void SetPointsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
	{
		FString reply;

		const bool result = SetPointsCbk(rcon_packet->Body);
		if (result)
		{
			reply = "Successfully set points\n";
		}
		else
		{
			reply = "Couldn't set points\n";
		}

		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
	}

	void ChangePointsAmountRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
	{
		FString reply;

		const bool result = ChangePointsAmountCbk(rcon_packet->Body);
		if (result)
		{
			reply = "Successfully set points\n";
		}
		else
		{
			reply = "Couldn't set points\n";
		}

		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
	}

	void GetPlayerPointsRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
	{
		FString reply;

		const int points = GetPlayerPointsCbk(rcon_packet->Body);
		if (points != -1)
		{
			reply = FString::Format("Player has {} points\n", points);
		}
		else
		{
			reply = "Couldn't get points amount\n";
		}

		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
	}

	void Init()
	{
		auto& commands = ArkApi::GetCommands();

		commands.AddChatCommand(GetText("PointsCmd"), &PrintPoints);
		commands.AddChatCommand(GetText("TradeCmd"), &Trade);

		commands.AddConsoleCommand("AddPoints", &AddPointsCmd);
		commands.AddConsoleCommand("SetPoints", &SetPointsCmd);
		commands.AddConsoleCommand("ChangePoints", &ChangePointsAmountCmd);
		commands.AddConsoleCommand("GetPlayerPoints", &GetPlayerPointsCmd);
		commands.AddConsoleCommand("ResetPoints", &ResetPointsCmd);

		commands.AddRconCommand("AddPoints", &AddPointsRcon);
		commands.AddRconCommand("SetPoints", &SetPointsRcon);
		commands.AddRconCommand("ChangePoints", &ChangePointsAmountRcon);
		commands.AddRconCommand("GetPlayerPoints", &GetPlayerPointsRcon);
	}

	void Unload()
	{
		auto& commands = ArkApi::GetCommands();

		commands.RemoveChatCommand(GetText("PointsCmd"));
		commands.RemoveChatCommand(GetText("TradeCmd"));

		commands.RemoveConsoleCommand("AddPoints");
		commands.RemoveConsoleCommand("SetPoints");
		commands.RemoveConsoleCommand("ChangePoints");
		commands.RemoveConsoleCommand("GetPlayerPoints");
		commands.RemoveConsoleCommand("ResetPoints");

		commands.RemoveRconCommand("AddPoints");
		commands.RemoveRconCommand("SetPoints");
		commands.RemoveRconCommand("ChangePoints");
		commands.RemoveRconCommand("GetPlayerPoints");
	}
} // namespace Points // namespace ArkShop
