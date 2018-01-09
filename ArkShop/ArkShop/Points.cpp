#include "Points.h"
#include "DBHelper.h"
#include "Tools.h"

namespace Points
{
	namespace
	{
		void PrintPoints(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
		{
			__int64 steamId = Tools::GetSteamId(playerController);

			if (DBHelper::IsPlayerEntryExists(steamId))
			{
				int points = GetPoints(steamId);

				Tools::SendDirectMessage(playerController, TEXT("You have %d points"), points);
			}
		}

		void Trade(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
		{
			__int64 senderSteamId = Tools::GetSteamId(playerController);

			if (DBHelper::IsPlayerEntryExists(senderSteamId))
			{
				TArray<FString> Parsed;
				message->ParseIntoArray(&Parsed, L"'", true);

				if (Parsed.IsValidIndex(2))
				{
					std::string receiverName = Parsed[1].ToString();

					int amount;

					try
					{
						amount = std::stoi(*Parsed[2]);
					}
					catch (const std::exception& e)
					{
						std::cout << "Error in Trade() - " << e.what() << std::endl;
						return;
					}

					if (amount <= 0)
						return;

					if (GetPoints(senderSteamId) < amount)
					{
						Tools::SendDirectMessage(playerController, TEXT("You don't have enough points"));
						return;
					}

					AShooterPlayerController* receiverPlayer = Tools::FindPlayerFromName(receiverName);
					if (receiverPlayer)
					{
						__int64 receiverSteamId = Tools::GetSteamId(receiverPlayer);
						if (receiverSteamId == senderSteamId)
						{
							Tools::SendDirectMessage(playerController, TEXT("You can't give points to yourself"));
							return;
						}

						if (DBHelper::IsPlayerEntryExists(receiverSteamId))
						{
							if (SpendPoints(amount, senderSteamId) && AddPoints(amount, receiverSteamId))
							{
								Tools::SendDirectMessage(playerController, TEXT("You have successfully gave %d points to %hs"), amount, receiverName.c_str());

								FString senderName = playerController->GetPlayerStateField()->GetPlayerNameField();
								Tools::SendDirectMessage(receiverPlayer, TEXT("You have received %d points from %s"), amount, *senderName);
							}
						}
					}
				}
				else
				{
					Tools::SendDirectMessage(playerController, TEXT("Usage: /trade 'Steam Name' amount"));
				}
			}
		}

		void AddPointsCmd(APlayerController* playerController, FString* cmd, bool shouldLog)
		{
			TArray<FString> Parsed;
			cmd->ParseIntoArray(&Parsed, L" ", true);

			if (Parsed.IsValidIndex(2))
			{
				__int64 steamId;
				int amount;

				try
				{
					steamId = std::stoll(*Parsed[1]);
					amount = std::stoi(*Parsed[2]);
				}
				catch (const std::exception& e)
				{
					std::cout << "Error in AddPointsCmd() - " << e.what() << std::endl;
					return;
				}

				if (DBHelper::IsPlayerEntryExists(steamId))
				{
					AddPoints(amount, steamId);
				}
			}
		}

		void SetPointsCmd(APlayerController* playerController, FString* cmd, bool shouldLog)
		{
			TArray<FString> Parsed;
			cmd->ParseIntoArray(&Parsed, L" ", true);

			if (Parsed.IsValidIndex(2))
			{
				__int64 steamId;
				int amount;

				try
				{
					steamId = std::stoll(*Parsed[1]);
					amount = std::stoi(*Parsed[2]);
				}
				catch (const std::exception& e)
				{
					std::cout << "Error in SetPointsCmd() - " << e.what() << std::endl;
					return;
				}

				if (DBHelper::IsPlayerEntryExists(steamId))
				{
					SetPoints(steamId, amount);
				}
			}
		}

		void ResetPointsCmd(APlayerController* playerController, FString* cmd, bool shouldLog)
		{
			TArray<FString> Parsed;
			cmd->ParseIntoArray(&Parsed, L" ", true);

			AShooterPlayerController* aShooterController = static_cast<AShooterPlayerController*>(playerController);

			if (Parsed.IsValidIndex(1))
			{
				if (Parsed[1].ToString() == "confirm")
				{
					auto db = GetDB();

					db << "UPDATE Players SET Points = 0;";

					Tools::SendDirectMessage(aShooterController, TEXT("Successfully reset points"));
				}
			}
			else
			{
				Tools::SendDirectMessage(aShooterController, TEXT("You are going to reset points for ALL players\nType 'ResetPoints confirm' if you want to continue"));
			}
		}

		void GetPlayerPointsCmd(APlayerController* playerController, FString* cmd, bool shouldLog)
		{
			TArray<FString> Parsed;
			cmd->ParseIntoArray(&Parsed, L" ", true);

			if (Parsed.IsValidIndex(1))
			{
				__int64 steamId;

				try
				{
					steamId = std::stoll(*Parsed[1]);
				}
				catch (const std::exception& e)
				{
					std::cout << "Error in GetPlayerPointsCmd() - " << e.what() << std::endl;
					return;
				}

				if (DBHelper::IsPlayerEntryExists(steamId))
				{
					int points = GetPoints(steamId);

					AShooterPlayerController* aShooterController = static_cast<AShooterPlayerController*>(playerController);

					Tools::SendDirectMessage(aShooterController, TEXT("Player has %d points"), points);
				}
			}
		}

		void SetTimedRewardOverrideForPlayerCmd(APlayerController* playerController, FString* cmd, bool shouldLog)
		{
			AShooterPlayerController* aShooterController = static_cast<AShooterPlayerController*>(playerController);

			TArray<FString> Parsed;
			cmd->ParseIntoArray(&Parsed, L" ", true);

			if (Parsed.IsValidIndex(2))
			{
				__int64 steamId;
				std::unique_ptr<int> amountOverride;

				try
				{
					steamId = std::stoll(*Parsed[1]);

					if (Parsed[2].Compare(L"null", ESearchCase::IgnoreCase) != 0)
					{
						auto tmp = std::stoi(*Parsed[2]);
						if (tmp > 0) amountOverride = std::make_unique<int>(tmp);
					}
				}
				catch (const std::exception& e)
				{
					std::cout << "Error in SetTimedRewardOverrideForPlayerCmd() - " << e.what() << std::endl;
					return;
				}

				if (DBHelper::IsPlayerEntryExists(steamId))
				{
					if (SetTimedRewardOverride(steamId, std::move(amountOverride)))
						Tools::SendDirectMessage(aShooterController, L"Successfully set timed reward override for player!");
					else
						Tools::SendDirectMessage(aShooterController, L"Failed to set timed reward override for player...");
				}
			}
		}

		// Rcon

		void AddPointsRcon(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
		{
			FString msg = rconPacket->Body;

			TArray<FString> Parsed;
			msg.ParseIntoArray(&Parsed, L" ", true);

			if (Parsed.IsValidIndex(2))
			{
				__int64 steamId;
				int amount;

				try
				{
					steamId = std::stoll(*Parsed[1]);
					amount = std::stoi(*Parsed[2]);
				}
				catch (const std::exception& e)
				{
					std::cout << "Error in AddPointsRcon() - " << e.what() << std::endl;
					return;
				}

				if (DBHelper::IsPlayerEntryExists(steamId))
				{
					AddPoints(amount, steamId);

					FString reply = L"Successfully added points\n";
					rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
				}
			}
		}

		void SetPointsRcon(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
		{
			FString msg = rconPacket->Body;

			TArray<FString> Parsed;
			msg.ParseIntoArray(&Parsed, L" ", true);

			if (Parsed.IsValidIndex(2))
			{
				__int64 steamId;
				int amount;

				try
				{
					steamId = std::stoll(*Parsed[1]);
					amount = std::stoi(*Parsed[2]);
				}
				catch (const std::exception& e)
				{
					std::cout << "Error in SetPointsRcon() - " << e.what() << std::endl;
					return;
				}

				if (DBHelper::IsPlayerEntryExists(steamId))
				{
					SetPoints(steamId, amount);

					FString reply(L"Successfully set points\n");
					rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
				}
			}
		}

		void GetPlayerPointsRcon(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
		{
			FString msg = rconPacket->Body;

			TArray<FString> Parsed;
			msg.ParseIntoArray(&Parsed, L" ", true);

			if (Parsed.IsValidIndex(1))
			{
				__int64 steamId;

				try
				{
					steamId = std::stoll(*Parsed[1]);
				}
				catch (const std::exception& e)
				{
					std::cout << "Error in GetPlayerPointsRcon() - " << e.what() << std::endl;
					return;
				}

				if (DBHelper::IsPlayerEntryExists(steamId))
				{
					int points = GetPoints(steamId);

					wchar_t buffer[256];
					swprintf_s(buffer, L"Player has %d points\n", points);

					FString reply(buffer);
					rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
				}
			}
		}

		void SetTimedRewardOverrideForPlayerRcon(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld)
		{
			FString msg = rconPacket->Body;

			TArray<FString> Parsed;
			msg.ParseIntoArray(&Parsed, L" ", true);

			if (Parsed.IsValidIndex(2))
			{
				__int64 steamId;
				std::unique_ptr<int> amountOverride;

				try
				{
					steamId = std::stoll(*Parsed[1]);
					if (Parsed[2].Compare(L"null", ESearchCase::IgnoreCase) != 0)
					{
						auto tmp = std::stoi(*Parsed[2]);
						if (tmp > 0) amountOverride = std::make_unique<int>(tmp);
					}
				}
				catch (const std::exception& e)
				{
					std::cout << "Error in SetTimedRewardOverrideForPlayerRcon() - " << e.what() << std::endl;
					return;
				}

				if (DBHelper::IsPlayerEntryExists(steamId))
				{
					wchar_t* message = nullptr;
					if (SetTimedRewardOverride(steamId, std::move(amountOverride)))
						message = L"Successfully set timed reward override for player\n";
					else
						message = L"Failed to set timed reward override for player...\n";
					FString reply(message);
					rconClientConnection->SendMessageW(rconPacket->Id, 0, &reply);
				}
			}
		}
	}

	void Init()
	{
		Ark::AddChatCommand(L"/points", &PrintPoints);
		Ark::AddChatCommand(L"/trade", &Trade);

		Ark::AddConsoleCommand(L"AddPoints", &AddPointsCmd);
		Ark::AddConsoleCommand(L"SetPoints", &SetPointsCmd);
		Ark::AddConsoleCommand(L"GetPlayerPoints", &GetPlayerPointsCmd);
		Ark::AddConsoleCommand(L"ResetPoints", &ResetPointsCmd);

		Ark::AddRconCommand(L"AddPoints", &AddPointsRcon);
		Ark::AddRconCommand(L"SetPoints", &SetPointsRcon);
		Ark::AddRconCommand(L"GetPlayerPoints", &GetPlayerPointsRcon);

		Ark::AddConsoleCommand(L"SetPointsOverrideForPlayer", &SetTimedRewardOverrideForPlayerCmd);
		Ark::AddRconCommand(L"SetPointsOverrideForPlayer", &SetTimedRewardOverrideForPlayerRcon);
	}

	bool AddPoints(int amount, __int64 steamId, bool isTimedReward)
	{
		auto db = GetDB();

		try
		{
			if (isTimedReward)
			{
				db << "UPDATE Players SET Points = Points + COALESCE(TimedRewardAmountOverride, ?) WHERE SteamId = ?;" << amount << steamId;
			}
			else db << "UPDATE Players SET Points = Points + ? WHERE SteamId = ?;" << amount << steamId;
		}
		catch (sqlite::sqlite_exception& e)
		{
			std::cout << "AddPoints() Unexpected DB error " << e.what() << std::endl;
			return false;
		}

		AShooterPlayerController* player = Tools::FindPlayerFromSteamId(steamId);
		if (player)
		{
			auto d = GetPointsAndTimedRewardAmountOverride(steamId);
			int totalPoints = std::get<0>(d);
			if (isTimedReward) amount = std::get<1>(d) != nullptr ? *std::get<1>(d) : amount;

			Tools::SendChatMessage(player, L"", TEXT("<RichColor Color=\"1, 1, 0, 1\">You have received %d points! (total: %d)</>"), amount, totalPoints);
		}

		return true;
	}

	bool SpendPoints(int amount, __int64 steamId)
	{
		if (amount <= 0)
			return false;

		auto db = GetDB();

		try
		{
			db << "UPDATE Players SET Points = Points - ? WHERE SteamId = ?;" << amount << steamId;
		}
		catch (sqlite::sqlite_exception& e)
		{
			std::cout << "SpendPoints() Unexpected DB error " << e.what() << std::endl;
			return false;
		}

		return true;
	}

	int GetPoints(__int64 steamId)
	{
		auto db = GetDB();

		int points = 0;

		try
		{
			db << "SELECT Points FROM Players WHERE SteamId = ?;" << steamId >> points;
		}
		catch (sqlite::sqlite_exception& e)
		{
			std::cout << "GetPoints() Unexpected DB error " << e.what() << std::endl;
		}

		return points;
	}

	std::tuple<int, std::unique_ptr<int>> GetPointsAndTimedRewardAmountOverride(__int64 steamId)
	{
		auto db = GetDB();

		int points = 0;
		std::unique_ptr<int> timedRewardAmountOverride;

		try
		{
			db << "SELECT Points, TimedRewardAmountOverride FROM Players WHERE SteamId = ? LIMIT 1;" << steamId >> [&](int _points, std::unique_ptr<int> _timedRewardDelayOverride)
			{
				points = _points;
				timedRewardAmountOverride = move(_timedRewardDelayOverride);
			};
		}
		catch (sqlite::sqlite_exception& e)
		{
			std::cout << "GetPointsAndTimedRewardAmountOverride() Unexpected DB error " << e.what() << std::endl;
		}

		return make_tuple(points, move(timedRewardAmountOverride));
	}

	bool SetPoints(__int64 steamId, int newAmount)
	{
		auto db = GetDB();

		try
		{
			db << "UPDATE Players SET Points = ? WHERE SteamId = ?;" << newAmount << steamId;
		}
		catch (sqlite::sqlite_exception& e)
		{
			std::cout << "SetPoints() Unexpected DB error " << e.what() << std::endl;
			return false;
		}

		return true;
	}

	bool SetTimedRewardOverride(__int64 steamId, std::unique_ptr<int> newValue)
	{
		auto db = GetDB();

		try
		{
			db << "UPDATE Players SET TimedRewardAmountOverride = ? WHERE SteamId = ?;" << newValue << steamId;
		}
		catch (sqlite::sqlite_exception& e)
		{
			std::cout << "SetTimedRewardOverride() Unexpected DB error " << e.what() << std::endl;
			return false;
		}

		return true;
	}
}
