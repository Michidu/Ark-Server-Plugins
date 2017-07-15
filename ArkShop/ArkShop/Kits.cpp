#include "Kits.h"
#include "DBHelper.h"
#include "Tools.h"
#include "Points.h"

namespace Kits
{
	namespace
	{
		nlohmann::basic_json<> GetPlayerKitsConfig(__int64 steamId);
		void GiveKit(AShooterPlayerController* playerController, const nlohmann::basic_json<>& kitEntry);
		bool SaveConfig(const std::string& dump, __int64 steamId);
		void ListKits(AShooterPlayerController* playerController);

		void Kit(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
		{
			TArray<FString> Parsed;
			message->ParseIntoArray(&Parsed, L" ", true);

			if (Parsed.IsValidIndex(1))
			{
				__int64 steamId = Tools::GetSteamId(playerController);

				if (DBHelper::IsPlayerEntryExists(steamId))
				{
					std::string kitName = Parsed[1].ToString();

					auto kitsList = json["Kits"];

					auto kitEntryIter = kitsList.find(kitName);
					if (kitEntryIter == kitsList.end())
					{
						Tools::SendDirectMessage(playerController, TEXT("Wrong kit name"));
						return;
					}

					auto kitEntry = kitEntryIter.value();

					// Kits json config
					nlohmann::json kitJson = GetPlayerKitsConfig(steamId);

					int amount = 0;

					auto kitJsonIter = kitJson.find(kitName);
					if (kitJsonIter == kitJson.end())
					{
						int defaultAmount = kitEntry["DefaultAmount"];
						if (defaultAmount > 0)
						{
							amount = defaultAmount - 1;

							kitJson[kitName]["Amount"] = amount;

							if (SaveConfig(kitJson.dump(), steamId))
							{
								GiveKit(playerController, kitEntry);
							}
						}
					}
					else
					{
						auto kitJsonEntry = kitJsonIter.value();

						amount = kitJsonEntry.value("Amount", 0);
						if (amount > 0)
						{
							kitJson[kitName]["Amount"] = --amount;

							if (SaveConfig(kitJson.dump(), steamId))
							{
								GiveKit(playerController, kitEntry);
							}
						}
					}

					Tools::SendDirectMessage(playerController, TEXT("You have %d %hs kits left"), amount, kitName.c_str());
				}
			}
			else
			{
				ListKits(playerController);
			}
		}

		nlohmann::basic_json<> GetPlayerKitsConfig(__int64 steamId)
		{
			auto db = GetDB();

			std::string kitsConfig = "{}";
			db << "SELECT Kits FROM Players WHERE SteamId = ?;" << steamId >> kitsConfig;

			std::stringstream ss;
			ss << kitsConfig;

			nlohmann::basic_json<> parsed = nlohmann::json::parse(ss);

			return parsed;
		}

		void GiveKit(AShooterPlayerController* playerController, const nlohmann::basic_json<>& kitEntry)
		{
			// Give items
			auto itemsMap = kitEntry.value("Items", nlohmann::json::array());
			for (auto iter = itemsMap.begin(); iter != itemsMap.end(); ++iter)
			{
				auto item = iter.value();

				int amount = item["amount"];
				float quality = item["quality"];
				bool forceBlueprint = item["forceBlueprint"];
				std::string blueprint = item["blueprint"];

				std::wstring buffer = Tools::ConvertToWideStr(blueprint);

				FString bpPath(buffer);

				playerController->GiveItem(&bpPath, amount, quality, forceBlueprint);
			}

			// Give dinos
			UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(playerController->GetCheatManagerField());

			auto dinosMap = kitEntry.value("Dinos", nlohmann::json::array());
			for (auto iter = dinosMap.begin(); iter != dinosMap.end(); ++iter)
			{
				auto dino = iter.value();

				int level = dino["level"];
				std::string className = dino["className"];

				std::wstring buffer = Tools::ConvertToWideStr(className);

				FString dinoClass(buffer);

				level = static_cast<int>(ceil(level / 1.5f));

				cheatManager->GMSummon(&dinoClass, level);
			}
		}

		void BuyKit(AShooterPlayerController* playerController, FString* message, EChatSendMode::Type mode)
		{
			TArray<FString> Parsed;
			message->ParseIntoArray(&Parsed, L" ", true);

			if (Parsed.IsValidIndex(2))
			{
				__int64 steamId = Tools::GetSteamId(playerController);

				if (DBHelper::IsPlayerEntryExists(steamId))
				{
					std::string kitName = Parsed[1].ToString();

					int amount;

					try
					{
						amount = std::stoi(*Parsed[2]);
					}
					catch (const std::exception& e)
					{
						std::cout << "Error in BuyKit() - " << e.what() << std::endl;
						return;
					}

					if (amount <= 0)
						return;

					auto kitsList = json["Kits"];

					auto kitEntryIter = kitsList.find(kitName);
					if (kitEntryIter == kitsList.end())
					{
						Tools::SendDirectMessage(playerController, TEXT("Wrong kit name"));
						return;
					}

					auto kitEntry = kitEntryIter.value();

					int price = kitEntry.value("Price", -1);
					if (price == -1)
					{
						Tools::SendDirectMessage(playerController, TEXT("You can't buy this kit"));
						return;
					}

					int finalPrice = price * amount;

					if (Points::GetPoints(steamId) >= finalPrice)
					{
						if (Points::SpendPoints(finalPrice, steamId))
						{
							AddKit(kitName, amount, steamId);

							Tools::SendDirectMessage(playerController, TEXT("You have successfully bought %hs kit"), kitName.c_str());
						}
					}
					else
					{
						Tools::SendDirectMessage(playerController, TEXT("You don't have enough points"));
					}
				}
			}
			else
			{
				Tools::SendDirectMessage(playerController, TEXT("Usage: /BuyKit KitName amount"));
			}
		}

		void ListKits(AShooterPlayerController* playerController)
		{
			std::stringstream ss;
			ss << "Available kits: ";

			int i = 0;

			auto kitsMap = json["Kits"];
			for (auto iter = kitsMap.begin(); iter != kitsMap.end(); ++iter)
			{
				if (i++)
				{
					ss << ", ";
				}

				ss << iter.key();
			}

			ss << "\nUsage: /kit KitName";

			Tools::SendDirectMessage(playerController, L"%hs", ss.str().c_str());
		}

		void AddKits(APlayerController* playerController, FString* cmd, bool shouldLog)
		{
			TArray<FString> Parsed;
			cmd->ParseIntoArray(&Parsed, L" ", true);

			if (Parsed.IsValidIndex(3))
			{
				std::string kitName = Parsed[2].ToString();

				__int64 steamId;
				int newAmount;

				try
				{
					steamId = std::stoll(*Parsed[1]);
					newAmount = std::stoi(*Parsed[3]);
				}
				catch (const std::exception& e)
				{
					std::cout << "Error in AddKits() - " << e.what() << std::endl;
					return;
				}

				if (DBHelper::IsPlayerEntryExists(steamId))
				{
					AddKit(kitName, newAmount, steamId);
				}
			}
		}

		void ResetKitsCmd(APlayerController* playerController, FString* cmd, bool shouldLog)
		{
			TArray<FString> Parsed;
			cmd->ParseIntoArray(&Parsed, L" ", true);

			AShooterPlayerController* aShooterController = static_cast<AShooterPlayerController*>(playerController);

			if (Parsed.IsValidIndex(1))
			{
				if (Parsed[1].ToString() == "confirm")
				{
					auto db = GetDB();

					db << "UPDATE Players SET Kits = \"{}\";";

					Tools::SendDirectMessage(aShooterController, TEXT("Successfully reset kits"));
				}
			}
			else
			{
				Tools::SendDirectMessage(aShooterController, TEXT("You are going to reset kits for ALL players\nType 'ResetKits confirm' if you want to continue"));
			}
		}

		bool SaveConfig(const std::string& dump, __int64 steamId)
		{
			auto db = GetDB();

			try
			{
				db << "UPDATE Players SET Kits = ? WHERE SteamId = ?;" << dump << steamId;
			}
			catch (sqlite::sqlite_exception& e)
			{
				std::cout << "SaveConfig() Unexpected DB error " << e.what() << std::endl;
				return false;
			}

			return true;
		}
	}

	void Init()
	{
		Ark::AddChatCommand(L"/kit", &Kit);
		Ark::AddChatCommand(L"/BuyKit", &BuyKit);

		Ark::AddConsoleCommand(L"AddKits", &AddKits);
		Ark::AddConsoleCommand(L"ResetKits", &ResetKitsCmd);
	}

	void AddKit(const std::string& kitName, int newAmount, __int64 steamId)
	{
		auto kitsList = json["Kits"];

		auto kitEntryIter = kitsList.find(kitName);
		if (kitEntryIter == kitsList.end())
			return;

		// Kits json config
		nlohmann::json kitJson = GetPlayerKitsConfig(steamId);

		auto kitJsonIter = kitJson.find(kitName);
		if (kitJsonIter == kitJson.end())
		{
			kitJson[kitName]["Amount"] = newAmount;
		}
		else
		{
			auto kitJsonEntry = kitJsonIter.value();

			int amount = kitJsonEntry.value("Amount", 0);

			kitJson[kitName]["Amount"] = amount + newAmount;
		}

		SaveConfig(kitJson.dump(), steamId);
	}
}
