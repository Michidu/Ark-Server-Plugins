#include "Store.h"
#include "Points.h"
#include "DBHelper.h"
#include "Tools.h"

namespace Store
{
	namespace
	{
		bool BuyItem(AShooterPlayerController* playerController, const TArray<FString>& Parsed, const nlohmann::basic_json<>& itemEntry, __int64 steamId);
		bool BuyDino(AShooterPlayerController* playerController, const nlohmann::basic_json<>& itemEntry, __int64 steamId);
		bool BuyBeacon(AShooterPlayerController* playerController, const nlohmann::basic_json<>& itemEntry, __int64 steamId);

		void Buy(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
		{
			__int64 steamId = Tools::GetSteamId(playerController);

			if (DBHelper::IsPlayerEntryExists(steamId))
			{
				TArray<FString> Parsed;
				message->ParseIntoArray(&Parsed, L" ", true);

				if (Parsed.IsValidIndex(1))
				{
					std::string strItemId = Parsed[1].ToString();

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
					Tools::SendDirectMessage(playerController, TEXT("Usage: /buy id amount"));
				}
			}
		}

		bool BuyItem(AShooterPlayerController* playerController, const TArray<FString>& Parsed, const nlohmann::basic_json<>& itemEntry, __int64 steamId)
		{
			bool success = false;

			if (Parsed.IsValidIndex(2))
			{
				std::string strItemId = Parsed[1].ToString();

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

				int points = Points::GetPoints(steamId);

				if (points >= finalPrice)
				{
					if (Points::SpendPoints(finalPrice, steamId))
					{
						auto itemsMap = itemEntry["items"];
						for (auto iter = itemsMap.begin(); iter != itemsMap.end(); ++iter)
						{
							auto item = iter.value();

							float quality = item["quality"];
							bool forceBlueprint = item["forceBlueprint"];
							int defaultAmount = item["amount"];
							std::string blueprint = item["blueprint"];

							std::wstring buffer = Tools::ConvertToWideStr(blueprint);

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

			int points = Points::GetPoints(steamId);

			if (points >= price)
			{
				Points::SpendPoints(price, steamId);

				std::wstring buffer = Tools::ConvertToWideStr(className);

				FString dinoClass(buffer);

				level = static_cast<int>(ceil(level / 1.5f));

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

			int points = Points::GetPoints(steamId);

			if (points >= price)
			{
				Points::SpendPoints(price, steamId);

				std::wstring buffer = Tools::ConvertToWideStr(className);

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

		void ShowItems(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
		{
			TArray<FString> Parsed;
			message->ParseIntoArray(&Parsed, L" ", true);

			int page = 0;

			try
			{
				if (Parsed.IsValidIndex(1))
				{
					page = std::stoi(*Parsed[1]) - 1;
				}
				else
				{
					Tools::SendDirectMessage(playerController, TEXT("Usage: /shop page"));
				}
			}
			catch (const std::exception&)
			{
				return;
			}

			if (page < 0)
				return;

			__int64 steamId = Tools::GetSteamId(playerController);

			if (DBHelper::IsPlayerEntryExists(steamId))
			{
				auto itemsList = json["ShopItems"];

				int itemsPerPage = json["General"].value("ItemsPerPage", 20);
				float displayTime = json["General"].value("ShopDisplayTime", 15.0f);

				int startIndex = page * itemsPerPage;
				if (startIndex >= itemsList.size())
					return;

				auto start = itemsList.begin();
				advance(start, startIndex);

				for (auto iter = start; iter != itemsList.end(); ++iter)
				{
					size_t i = distance(itemsList.begin(), iter);
					if (i == startIndex + itemsPerPage)
						break;

					auto item = iter.value();

					int price = item["price"];
					std::string type = item["type"];
					std::string description = item.value("description", "No description");

					std::stringstream ss;

					ss << i + 1 << ") " << description;

					if (type == "dino")
					{
						int level = item["level"];

						ss << ", Level: " << level;
					}

					ss << ", Id: " << iter.key() << ", Price: " << price << "\n";

					Tools::SendNotification(playerController, TEXT("%hs"), {1,0.549f,0,1}, 0.8f, displayTime, nullptr, ss.str().c_str());
				}
			}
		}
	}

	void Init()
	{
		Ark::AddChatCommand(L"/buy", &Buy);
		Ark::AddChatCommand(L"/shop", &ShowItems);
	}
}
