#include "Store.h"

#include "Points.h"
#include "ArkShop.h"
#include "DBHelper.h"
#include "ShopLog.h"

namespace ArkShop::Store
{
	/**
	 * \brief Buy an item from shop
	 */
	bool BuyItem(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& item_entry, uint64 steam_id,
	             int amount)
	{
		bool success = false;

		if (amount <= 0)
			amount = 1;

		const unsigned price = item_entry["price"];
		const int final_price = price * amount;
		if (final_price <= 0)
			return false;

		const int points = Points::GetPoints(steam_id);

		if (points >= final_price && Points::SpendPoints(final_price, steam_id))
		{
			auto items_map = item_entry["items"];
			for (const auto& item : items_map)
			{
				const float quality = item["quality"];
				const bool force_blueprint = item["forceBlueprint"];
				const int default_amount = item["amount"];
				std::string blueprint = item["blueprint"];

				const int final_amount = default_amount * amount;
				if (final_amount <= 0)
					return false;

				FString fblueprint(blueprint.c_str());
				player_controller->GiveItem(&fblueprint, final_amount, quality, force_blueprint);
			}

			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
			                                      *GetText("BoughtItem"));

			success = true;
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
			                                      *GetText("NoPoints"));
		}

		return success;
	}

	/**
	* \brief Buy a dino from shop
	*/
	bool BuyDino(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& item_entry, uint64 steam_id)
	{
		bool success = false;

		const int price = item_entry["price"];
		const int level = item_entry["level"];
		std::string blueprint = item_entry["blueprint"];

		const int points = Points::GetPoints(steam_id);

		if (points >= price && Points::SpendPoints(price, steam_id))
		{
			const FString fblueprint(blueprint.c_str());

			ArkApi::GetApiUtils().SpawnDino(player_controller, fblueprint, nullptr, level, true);

			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
			                                      *GetText("BoughtDino"));

			success = true;
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
			                                      *GetText("NoPoints"));
		}

		return success;
	}

	/**
	* \brief Buy a beacon from shop
	*/
	bool BuyBeacon(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& item_entry, uint64 steam_id)
	{
		bool success = false;

		const int price = item_entry["price"];
		std::string class_name = item_entry["className"];

		const int points = Points::GetPoints(steam_id);

		if (points >= price && Points::SpendPoints(price, steam_id))
		{
			FString fclass_name(class_name.c_str());

			UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(player_controller->CheatManagerField()());
			cheatManager->Summon(&fclass_name);

			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
			                                      *GetText("BoughtBeacon"));

			success = true;
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
			                                      *GetText("NoPoints"));
		}

		return success;
	}

	bool Buy(AShooterPlayerController* player_controller, const FString& item_id, int amount)
	{
		if (amount <= 0)
			amount = 1;

		bool success = false;

		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		if (DBHelper::IsPlayerExists(steam_id))
		{
			auto items_list = config["ShopItems"];

			auto item_entry_iter = items_list.find(item_id.ToString());
			if (item_entry_iter == items_list.end())
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
				                                      *GetText("WrongId"));
				return false;
			}

			auto item_entry = item_entry_iter.value();

			const std::string type = item_entry["type"];

			if (type == "item")
				success = BuyItem(player_controller, item_entry, steam_id, amount);
			else if (type == "dino")
				success = BuyDino(player_controller, item_entry, steam_id);
			else if (type == "beacon")
				success = BuyBeacon(player_controller, item_entry, steam_id);

			if (success)
			{
				const std::wstring log = fmt::format(TEXT("{}({}) bought item \"{}\". Amount - {}"),
				                                     *ArkApi::IApiUtils::GetSteamName(player_controller), steam_id, *item_id,
				                                     amount);

				ShopLog::GetLog()->info(ArkApi::Tools::ConvertToAnsiStr(log));
			}
		}

		return success;
	}

	// Chat callbacks

	void ChatBuy(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
	{
		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			int amount = 0;

			if (parsed.IsValidIndex(2))
			{
				try
				{
					amount = std::stoi(*parsed[2]);
				}
				catch (const std::exception& exception)
				{
					Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
					return;
				}
			}

			Buy(player_controller, parsed[1], amount);
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
			                                      *GetText("BuyUsage"));
		}
	}

	void ShowItems(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type mode)
	{
		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		int page = 0;

		if (parsed.IsValidIndex(1))
		{
			try
			{
				page = std::stoi(*parsed[1]) - 1;
			}
			catch (const std::exception&)
			{
				return;
			}
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
			                                      *GetText("ShopUsage"));
		}

		if (page < 0)
			return;

		auto items_list = config["ShopItems"];

		const int items_per_page = config["General"].value("ItemsPerPage", 20);
		const float display_time = config["General"].value("ShopDisplayTime", 15.0f);
		const float text_size = config["General"].value("ShopTextSize", 1.3f);

		const unsigned start_index = page * items_per_page;
		if (start_index >= items_list.size())
			return;

		auto start = items_list.begin();
		advance(start, start_index);

		std::stringstream ss;

		for (auto iter = start; iter != items_list.end(); ++iter)
		{
			const size_t i = distance(items_list.begin(), iter);
			if (i == start_index + items_per_page)
				break;

			auto item = iter.value();

			const int price = item["price"];
			const std::string type = item["type"];
			const std::string description = item.value("description", "No description");

			ss << i + 1 << ") " << description;

			if (type == "dino")
			{
				const int level = item["level"];

				ss << ", Level: " << level;
			}

			ss << ", Id: " << iter.key() << ", Price: " << price << "\n";
		}

		ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::White, text_size, display_time, nullptr,
		                                       ss.str().c_str());
	}

	void Init()
	{
		auto& commands = ArkApi::GetCommands();

		commands.AddChatCommand(GetText("BuyCmd"), &ChatBuy);
		commands.AddChatCommand(GetText("ShopCmd"), &ShowItems);
	}
}
