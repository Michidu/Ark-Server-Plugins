#include "Points.h"
#include "DBHelper.h"
#include "Tools.h"

namespace Points
{
	namespace
	{
		void PrintPoints(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode);
		void Trade(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode);
		void AddPointsCmd(APlayerController* playerController, FString* cmd, bool shouldLog);
		void GetPlayerPoints(APlayerController* playerController, FString* cmd, bool shouldLog);
		void AddPointsRcon(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld);
		void GetPlayerPointsRcon(RCONClientConnection* rconClientConnection, RCONPacket* rconPacket, UWorld* uWorld);

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

		void GetPlayerPoints(APlayerController* playerController, FString* cmd, bool shouldLog)
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
					std::cout << "Error in GetPlayerPoints() - " << e.what() << std::endl;
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
	}

	void Init()
	{
		Ark::AddChatCommand(L"/points", &PrintPoints);
		Ark::AddChatCommand(L"/trade", &Trade);

		Ark::AddConsoleCommand(L"AddPoints", &AddPointsCmd);
		Ark::AddConsoleCommand(L"GetPlayerPoints", &GetPlayerPoints);

		Ark::AddRconCommand(L"AddPoints", &AddPointsRcon);
		Ark::AddRconCommand(L"GetPlayerPoints", &GetPlayerPointsRcon);
	}

	bool AddPoints(int amount, __int64 steamId)
	{
		auto db = GetDB();

		try
		{
			db << "UPDATE Players SET Points = Points + ? WHERE SteamId = ?;" << amount << steamId;
		}
		catch (sqlite::sqlite_exception& e)
		{
			std::cout << "AddPoints() Unexpected DB error " << e.what() << std::endl;
			return false;
		}

		AShooterPlayerController* player = Tools::FindPlayerFromSteamId(steamId);
		if (player)
		{
			Tools::SendChatMessage(player, L"", TEXT("<RichColor Color=\"1, 1, 0, 1\">You have received %d points! (total: %d)</>"), amount, GetPoints(steamId));
		}

		return true;
	}

	bool SpendPoints(int amount, __int64 steamId)
	{
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
}
