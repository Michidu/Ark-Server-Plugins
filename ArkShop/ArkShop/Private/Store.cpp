#include <Store.h>

#include <Points.h>
#include <ArkPermissions.h>

#include "ArkShop.h"
#include "DBHelper.h"
#include "ShopLog.h"
#include "Discord.h"

namespace ArkShop::Store
{
	bool HasBuff(AShooterPlayerController* player_controller)
	{
		auto buffs = player_controller->GetPlayerCharacter()->BuffsField();

		for (const auto& buff : buffs)
		{
			const FString bpBuff = ArkApi::GetApiUtils().GetBlueprint(buff);
			const FString bpMindControl = "Blueprint'/Game/Genesis2/Dinos/BrainSlug/Buff_BrainSlugPostProccess.Buff_BrainSlugPostProccess'";
			if (bpMindControl == bpBuff) {
				return true;
			}
		}
		return false;
	}

	/**
	 * \brief Buy an item from shop
	 */
	bool BuyItem(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& item_entry, uint64 steam_id,
		int amount)
	{
		bool success = false;

		if (amount <= 0)
		{
			amount = 1;
		}

		const unsigned price = item_entry["Price"];
		const int final_price = price * amount;
		if (final_price <= 0)
		{
			return false;
		}

		const int points = Points::GetPoints(steam_id);

		if (points >= final_price && Points::SpendPoints(final_price, steam_id))
		{
			auto items_map = item_entry["Items"];
			for (const auto& item : items_map)
			{
				const float quality = item["Quality"];
				const bool force_blueprint = item["ForceBlueprint"];
				const int default_amount = item["Amount"];
				std::string blueprint = item["Blueprint"];

				FString fblueprint(blueprint.c_str());

				for (int i = 0; i < amount; ++i)
				{
					TArray<UPrimalItem*> out_items;
					player_controller->GiveItem(&out_items, &fblueprint, default_amount, quality, force_blueprint,
						false, 0);
				}
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
	* \brief Buy an unlockengram from shop
	*/
	bool UnlockEngram(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& item_entry,
		uint64 steam_id)
	{
		bool success = false;
		const int price = item_entry["Price"];

		const int points = Points::GetPoints(steam_id);

		if (points >= price && Points::SpendPoints(price, steam_id))
		{
			auto items_map = item_entry["Items"];
			for (const auto& item : items_map)
			{
				const std::string blueprint = item["Blueprint"];
				FString fblueprint(blueprint);

				auto* cheat_manager = static_cast<UShooterCheatManager*>(player_controller->CheatManagerField());
				cheat_manager->UnlockEngram(&fblueprint);
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
	* \brief Buy an Command from shop
	*/
	bool BuyCommand(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& item_entry,
		uint64 steam_id)
	{
		bool success = false;
		const int price = item_entry["Price"];

		const int points = Points::GetPoints(steam_id);

		if (points >= price && Points::SpendPoints(price, steam_id))
		{
			auto items_map = item_entry["Items"];
			for (const auto& item : items_map)
			{
				const std::string command = item["Command"];

				const bool exec_as_admin = item.value("ExecuteAsAdmin", false);

				FString fcommand = fmt::format(
					command, fmt::arg("steamid", steam_id), 
					fmt::arg("playerid", ArkApi::GetApiUtils().GetPlayerID(player_controller)),
					fmt::arg("tribeid", ArkApi::GetApiUtils().GetTribeID(player_controller))
				).c_str();

				const bool was_admin = player_controller->bIsAdmin()();

				if (exec_as_admin)
					player_controller->bIsAdmin() = true;

				FString result;
				player_controller->ConsoleCommand(&result, &fcommand, true);

				if (exec_as_admin)
					player_controller->bIsAdmin() = was_admin;
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

		const int price = item_entry["Price"];
		const int level = item_entry["Level"];
		const bool neutered = item_entry.value("Neutered", false);
		std::string saddleblueprint = item_entry.value("SaddleBlueprint", "");
		std::string blueprint = item_entry["Blueprint"];

		const int points = Points::GetPoints(steam_id);

		if (points >= price && Points::SpendPoints(price, steam_id))
		{
			success = ArkShop::GiveDino(player_controller, level, neutered, blueprint, saddleblueprint);
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("NoPoints"));
			return success;
		}

		if (success)
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("BoughtDino"));
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("RefundError"));
			Points::AddPoints(price, steam_id); //refund
		}

		return success;
	}

	/**
	* \brief Buy a beacon from shop
	*/
	bool BuyBeacon(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& item_entry,
		uint64 steam_id)
	{
		bool success = false;

		const int price = item_entry["Price"];
		std::string class_name = item_entry["ClassName"];

		const int points = Points::GetPoints(steam_id);

		if (points >= price && Points::SpendPoints(price, steam_id))
		{
			FString fclass_name(class_name.c_str());

			auto* cheatManager = static_cast<UShooterCheatManager*>(player_controller->CheatManagerField());
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

	/**
	* \brief Buy experience from shop
	*/
	bool BuyExperience(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& item_entry,
		uint64 steam_id)
	{
		bool success = false;

		const int price = item_entry["Price"];
		const float amount = item_entry["Amount"];
		const bool give_to_dino = item_entry["GiveToDino"];

		if (!give_to_dino && ArkApi::IApiUtils::IsRidingDino(player_controller))
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
				*GetText("RidingDino"));
			return false;
		}

		const int points = Points::GetPoints(steam_id);

		if (points >= price && Points::SpendPoints(price, steam_id))
		{
			player_controller->AddExperience(amount, false, true);

			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
				*GetText("BoughtExp"));

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
		if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
		{
			return false;
		}

		if (amount <= 0)
		{
			amount = 1;
		}

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

			const std::string type = item_entry["Type"];

			// Check if player has permisson to buy this

			const std::string permissions = item_entry.value("Permissions", "");
			if (!permissions.empty())
			{
				const FString fpermissions(permissions);

				TArray<FString> groups;
				fpermissions.ParseIntoArray(groups, L",", true);

				bool has_permissions = false;

				for (const auto& group : groups)
				{
					if (Permissions::IsPlayerInGroup(steam_id, group))
					{
						has_permissions = true;
						break;
					}
				}

				if (!has_permissions)
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
						*GetText("NoPermissionsStore"), type);
					return false;
				}
			}

			const int min_level = item_entry.value("MinLevel", 1);
			const int max_level = item_entry.value("MaxLevel", 999);

			auto* primal_character = static_cast<APrimalCharacter*>(player_controller->CharacterField());
			UPrimalCharacterStatusComponent* char_component = primal_character->MyCharacterStatusComponentField();

			const int level = char_component->BaseCharacterLevelField() + char_component->ExtraCharacterLevelField();
			if (level < min_level || level > max_level)
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("BadLevel"), min_level, max_level);
				return false;
			}

			if (type == "item")
			{
				success = BuyItem(player_controller, item_entry, steam_id, amount);
			}
			else if (type == "dino")
			{
				success = BuyDino(player_controller, item_entry, steam_id);
			}
			else if (type == "beacon")
			{
				success = BuyBeacon(player_controller, item_entry, steam_id);
			}
			else if (type == "experience")
			{
				success = BuyExperience(player_controller, item_entry, steam_id);
			}
			else if (type == "unlockengram")
			{
				success = UnlockEngram(player_controller, item_entry, steam_id);
			}
			else if (type == "command")
			{
				success = BuyCommand(player_controller, item_entry, steam_id);
			}

			if (success)
			{
				const unsigned price = item_entry["Price"];
				const int final_price = price * amount;

				const std::wstring log = fmt::format(L"{}({}) Bought item: \"{}\" Amount: {} Total Spent Points: {}",
					*ArkApi::IApiUtils::GetSteamName(player_controller),
					steam_id,
					*item_id, amount,
					final_price);

				ShopLog::GetLog()->info(ArkApi::Tools::Utf8Encode(log));
				if (ArkShop::discord_enabled)
				{
					const std::wstring log = fmt::format(L"{}({}) Bought item: {} Amount: {} Total Spent Points: {}",
						*ArkApi::IApiUtils::GetSteamName(player_controller),
						steam_id,
						*item_id, amount,
						final_price);

					PostToDiscord(L"{{\"content\":\"```stylus\\n{}```\",\"username\":\"{}\",\"avatar_url\":null}}",
						log, ArkShop::discord_sender_name);
				}
			}
		}

		return success;
	}

	// Chat callbacks

	void ChatBuy(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type /*unused*/)
	{
		if (!IsStoreEnabled(player_controller))
		{
			return;
		}

		if (HasBuff(player_controller))
		{
			return;
		}

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
				catch (const std::exception&)
				{
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

	void ShowItems(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type /*unused*/)
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
		{
			return;
		}

		auto items_list = config["ShopItems"];

		const int items_per_page = config["General"].value("ItemsPerPage", 20);
		const float display_time = config["General"].value("ShopDisplayTime", 15.0f);
		const float text_size = config["General"].value("ShopTextSize", 1.3f);

		const unsigned start_index = page * items_per_page;
		if (start_index >= items_list.size())
		{
			return;
		}

		auto start = items_list.begin();
		advance(start, start_index);

		FString store_str = "";

		for (auto iter = start; iter != items_list.end(); ++iter)
		{
			const size_t i = distance(items_list.begin(), iter);
			if (i == start_index + items_per_page)
			{
				break;
			}

			auto item = iter.value();

			const int price = item["Price"];
			const std::string type = item["Type"];
			const std::wstring description = ArkApi::Tools::Utf8Decode(item.value("Description", "No description"));

			if (type == "dino")
			{
				const int level = item["Level"];

				store_str += FString::Format(*GetText("StoreListDino"), i + 1, description, level,
					ArkApi::Tools::Utf8Decode(iter.key()), price);
			}
			else
			{
				store_str += FString::Format(*GetText("StoreListItem"), i + 1, description,
					ArkApi::Tools::Utf8Decode(iter.key()),
					price);
			}
		}

		store_str = FString::Format(*GetText("StoreListFormat"), *store_str);

		ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::Green, text_size, display_time, nullptr,
			*store_str);

		FString shopmessage = GetText("ShopMessage");
		if (shopmessage != ArkApi::Tools::Utf8Decode("No message").c_str())
		{
			shopmessage = FString::Format(*shopmessage, page + 1,
				items_list.size() % items_per_page == 0
				? items_list.size() / items_per_page
				: items_list.size() / items_per_page + 1);
			ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::Green, text_size, display_time,
				nullptr,
				*shopmessage);
		}
	}

	bool findCaseInsensitive(std::wstring data, std::wstring toSearch, size_t pos = 0)
	{
		std::transform(data.begin(), data.end(), data.begin(), ::tolower);
		std::transform(toSearch.begin(), toSearch.end(), toSearch.begin(), ::tolower);

		if (data.find(toSearch, pos) != std::wstring::npos)
			return true;
		else
			return false;
	}

	void FindItems(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type /*unused*/)
	{
		std::wstring searchTerm;
		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			searchTerm = ArkApi::Tools::Utf8Decode(parsed[1].ToString());
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("ShopFindUsage"));
			return;
		}

		auto items_list = config["ShopItems"];

		const int items_per_page = config["General"].value("ItemsPerPage", 20);
		const float display_time = config["General"].value("ShopDisplayTime", 15.0f);
		const float text_size = config["General"].value("ShopTextSize", 1.3f);

		FString store_str = "";

		for (auto iter = items_list.begin(); iter != items_list.end(); ++iter)
		{
			bool found = false;

			const size_t i = distance(items_list.begin(), iter);

			auto item = iter.value();
			std::wstring key = ArkApi::Tools::Utf8Decode(iter.key());
			if (findCaseInsensitive(key, searchTerm))
				found = true;

			const int price = item["Price"];
			const std::string type = item["Type"];
			const std::wstring description = ArkApi::Tools::Utf8Decode(item.value("Description", "No description"));

			if (findCaseInsensitive(description, searchTerm))
				found = true;

			if (found)
			{
				if (type == "dino")
				{
					const int level = item["Level"];

					store_str += FString::Format(*GetText("StoreListDino"), i + 1, description, level,
						ArkApi::Tools::Utf8Decode(iter.key()), price);
				}
				else
				{
					store_str += FString::Format(*GetText("StoreListItem"), i + 1, description,
						ArkApi::Tools::Utf8Decode(iter.key()),
						price);
				}
			}
		}

		if (store_str.IsEmpty())
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("ShopFindNotFound"));
		}
		else
		{
			store_str = FString::Format(*GetText("StoreListFormat"), *store_str);

			ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::Green, text_size, display_time, nullptr,
				*store_str);
		}
	}

	bool IsStoreEnabled(AShooterPlayerController* player_controller)
	{
		return ArkShop::IsStoreEnabled(player_controller);
	}

	void ToogleStore(bool enabled, const FString& reason)
	{
		ArkShop::ToogleStore(enabled, reason);
	}

	void Init()
	{
		auto& commands = ArkApi::GetCommands();

		commands.AddChatCommand(GetText("BuyCmd"), &ChatBuy);
		commands.AddChatCommand(GetText("ShopCmd"), &ShowItems);
		commands.AddChatCommand(GetText("ShopFindCmd"), &FindItems);
	}

	void Unload()
	{
		auto& commands = ArkApi::GetCommands();

		commands.RemoveChatCommand(GetText("BuyCmd"));
		commands.RemoveChatCommand(GetText("ShopCmd"));
		commands.RemoveChatCommand(GetText("ShopFindCmd"));
	}
} // namespace Store // namespace ArkShop