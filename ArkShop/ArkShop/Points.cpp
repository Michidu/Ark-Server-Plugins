#include "Points.h"
#include "DBHelper.h"
#include "Tools.h"

namespace Points
{
	namespace
	{
		void PrintPoints(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode);
		void Buy(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode);
		bool BuyItem(AShooterPlayerController* playerController, const TArray<FString>& Parsed, const nlohmann::basic_json<>& itemEntry, __int64 steamId);
		bool BuyDino(AShooterPlayerController* playerController, const nlohmann::basic_json<>& itemEntry, __int64 steamId);
		bool BuyBeacon(AShooterPlayerController* playerController, const nlohmann::basic_json<>& itemEntry, __int64 steamId);
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

		void Buy(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
		{
			__int64 steamId = Tools::GetSteamId(playerController);

			if (DBHelper::IsPlayerEntryExists(steamId))
			{
				TArray<FString> Parsed;
				message->ParseIntoArray(&Parsed, L" ", true);

				if (Parsed.IsValidIndex(1))
				{
					std::string strItemId = Parsed[1].c_str();

					auto itemsList = json["ShopItems"];

					auto itemEntryIter = itemsList.find(strItemId);
					if (itemEntryIter == itemsList.end())
					{
						Tools::SendDirectMessage(playerController, TEXT("Wrong id"));
						return;
					}

					auto itemEntry = itemEntryIter.value();

					std::string type = itemEntry["type"];

					bool success = false;

					if (type == "item")
						success = BuyItem(playerController, Parsed, itemEntry, steamId);
					else if (type == "dino")
						success = BuyDino(playerController, itemEntry, steamId);
					else if (type == "beacon")
						success = BuyBeacon(playerController, itemEntry, steamId);

					Tools::Log(std::to_string(steamId) + " bought id - " + strItemId + ". Success: " + std::to_string(success));
				}
				else
				{
					Tools::SendDirectMessage(playerController, TEXT("Usage: /buy id <amount>"));
				}
			}
		}

		bool BuyItem(AShooterPlayerController* playerController, const TArray<FString>& Parsed, const nlohmann::basic_json<>& itemEntry, __int64 steamId)
		{
			bool success = false;

			if (Parsed.IsValidIndex(2))
			{
				std::string strItemId = Parsed[1].c_str();

				int amount;

				try
				{
					amount = std::stoi(*Parsed[2]);
				}
				catch (const std::exception& e)
				{
					std::cout << "Error in BuyItem() - " << e.what() << std::endl;
					return false;
				}

				if (amount <= 0)
					return false;

				int price = itemEntry["price"];

				int finalPrice = price * amount;

				int points = GetPoints(steamId);

				if (points >= finalPrice)
				{
					if (SpendPoints(finalPrice, steamId))
					{
						auto itemsMap = itemEntry["items"];
						for (auto iter = itemsMap.begin(); iter != itemsMap.end(); ++iter)
						{
							auto item = iter.value();

							float quality = item["quality"];
							bool forceBlueprint = item["forceBlueprint"];
							int defaultAmount = item["amount"];
							std::string blueprint = item["blueprint"];

							wchar_t buffer[512];
							swprintf_s(buffer, L"%hs", blueprint.c_str());

							FString bpPath(buffer);

							int finalAmount = defaultAmount * amount;

							playerController->GiveItem(&bpPath, finalAmount, quality, forceBlueprint);
						}

						Tools::SendDirectMessage(playerController, TEXT("You have successfully bought item"));

						success = true;
					}
				}
				else
				{
					Tools::SendDirectMessage(playerController, TEXT("You don't have enough points"));
				}
			}
			else
			{
				Tools::SendDirectMessage(playerController, TEXT("Please, specify amount"));
			}

			return success;
		}

		bool BuyDino(AShooterPlayerController* playerController, const nlohmann::basic_json<>& itemEntry, __int64 steamId)
		{
			bool success = false;

			int price = itemEntry["price"];
			int level = itemEntry["level"];
			std::string className = itemEntry["className"];

			int points = GetPoints(steamId);

			if (points >= price)
			{
				SpendPoints(price, steamId);

				wchar_t buffer[256];
				swprintf_s(buffer, L"%hs", className.c_str());

				FString dinoClass(buffer);

				level = static_cast<int>(std::ceil(level / 1.5f));

				UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(playerController->GetCheatManagerField());
				cheatManager->GMSummon(&dinoClass, level);

				Tools::SendDirectMessage(playerController, TEXT("You have successfully bought dino"));

				success = true;
			}
			else
			{
				Tools::SendDirectMessage(playerController, TEXT("You don't have enough points"));
			}

			return success;
		}

		bool BuyBeacon(AShooterPlayerController* playerController, const nlohmann::basic_json<>& itemEntry, __int64 steamId)
		{
			bool success = false;

			int price = itemEntry["price"];
			std::string className = itemEntry["className"];

			int points = GetPoints(steamId);

			if (points >= price)
			{
				SpendPoints(price, steamId);

				wchar_t buffer[256];
				swprintf_s(buffer, L"%hs", className.c_str());

				FString beaconClass(buffer);

				UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(playerController->GetCheatManagerField());
				cheatManager->Summon(&beaconClass);

				Tools::SendDirectMessage(playerController, TEXT("You have successfully bought beacon"));

				success = true;
			}
			else
			{
				Tools::SendDirectMessage(playerController, TEXT("You don't have enough points"));
			}

			return success;
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
					std::string receiverName = Parsed[1].c_str();

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
		Ark::AddChatCommand(L"/buy", &Buy);
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
