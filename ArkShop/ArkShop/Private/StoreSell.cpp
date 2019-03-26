#include "StoreSell.h"

#include <Points.h>

#include "ArkShop.h"
#include "DBHelper.h"
#include "ShopLog.h"

namespace ArkShop::StoreSell
{
	FString GetItemBlueprint(UPrimalItem* item)
	{
		if (item != nullptr)
		{
			FString path_name;
			item->ClassField()->GetDefaultObject(true)->GetFullName(&path_name, nullptr);

			if (int find_index = 0; path_name.FindChar(' ', find_index))
			{
				path_name = "Blueprint'" + path_name.Mid(find_index + 1,
				                                         path_name.Len() - (find_index + (path_name.EndsWith(
					                                                                          "_C", ESearchCase::
					                                                                          CaseSensitive)
					                                                                          ? 3
					                                                                          : 1))) + "'";
				return path_name.Replace(L"Default__", L"", ESearchCase::CaseSensitive);
			}
		}

		return FString("");
	}

	bool SellItem(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& item_entry,
	              uint64 steam_id,
	              int amount)
	{
		bool success = false;

		const int price = item_entry.value("Price", 1) * amount;
		if (price <= 0)
		{
			return false;
		}

		const FString blueprint = FString(item_entry.value("Blueprint", "").c_str());
		const int needed_amount = item_entry.value("Amount", 1) * amount;
		if (needed_amount <= 0)
		{
			return false;
		}

		UPrimalInventoryComponent* inventory = player_controller->GetPlayerCharacter()->MyInventoryComponentField();
		if (inventory == nullptr)
		{
			return false;
		}

		int item_count = 0;

		// Count items

		TArray<UPrimalItem*> items_for_removal;

		TArray<UPrimalItem*> items = inventory->InventoryItemsField();
		for (UPrimalItem* item : items)
		{
			if (item->ClassField() != nullptr)
			{
				const FString item_bp = GetItemBlueprint(item);

				if (item_bp == blueprint)
				{
					items_for_removal.Add(item);

					item_count += item->GetItemQuantity();
					if (item_count >= needed_amount)
					{
						break;
					}
				}
			}
		}

		if (item_count >= needed_amount)
		{
			item_count = 0;

			// Remove items
			for (UPrimalItem* item : items_for_removal)
			{
				item_count += item->GetItemQuantity();

				if (item_count > needed_amount)
				{
					item->SetQuantity(item_count - needed_amount, true);
					inventory->NotifyClientsItemStatus(item, false, false, true, false, false, nullptr, nullptr, false,
					                                   false, true);
				}
				else
				{
					inventory->RemoveItem(&item->ItemIDField(), false, false, true, true);
				}
			}

			if (!Points::AddPoints(price, steam_id))
			{
				ShopLog::GetLog()->error("Unexpected error when selling {} for {}", blueprint.ToString(), steam_id);
				return false;
			}

			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("SoldItems"));

			success = true;
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("NotEnoughItems"),
			                                      item_count,
			                                      needed_amount);
		}

		return success;
	}

	bool Sell(AShooterPlayerController* player_controller, const FString& item_id, int amount)
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
			auto items_list = config.value("SellItems", nlohmann::json::object());

			auto item_entry_iter = items_list.find(item_id.ToString());
			if (item_entry_iter == items_list.end())
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
				                                      *GetText("WrongId"));
				return false;
			}

			auto item_entry = item_entry_iter.value();

			const std::string type = item_entry["Type"];

			if (type == "item")
			{
				success = SellItem(player_controller, item_entry, steam_id, amount);
			}

			if (success)
			{
				const std::wstring log = fmt::format(TEXT("{}({}) sold item \"{}\". Amount - {}"),
				                                     *ArkApi::IApiUtils::GetSteamName(player_controller), steam_id,
				                                     *item_id,
				                                     amount);

				ShopLog::GetLog()->info(ArkApi::Tools::Utf8Encode(log));
			}
		}

		return success;
	}

	// Chat callbacks

	void ChatSell(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type /*unused*/)
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

			Sell(player_controller, parsed[1], amount);
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
			                                      *GetText("SellUsage"));
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

		if (page < 0)
		{
			return;
		}

		auto items_list = config.value("SellItems", nlohmann::json::object());

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
			const std::string description = item.value("Description", "No description");

			store_str += FString::Format(*GetText("StoreListItem"), i + 1, description,
			                             ArkApi::Tools::Utf8Decode(iter.key()),
			                             price);
		}

		ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::White, text_size, display_time, nullptr,
		                                       *store_str);
	}

	// Console callbacks

	void ListInvItemsCmd(APlayerController* controller, FString* /*cmd*/, bool /*unused*/)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(controller);
		AShooterCharacter* character = shooter_controller->GetPlayerCharacter();
		if (!character)
		{
			return;
		}

		UPrimalInventoryComponent* inventory = character->MyInventoryComponentField();
		if (!inventory)
		{
			return;
		}

		TArray<UPrimalItem*> items = inventory->InventoryItemsField();
		for (UPrimalItem* item : items)
		{
			if (item->ClassField() != nullptr)
			{
				const FString item_bp = GetItemBlueprint(item);
				Log::GetLog()->info(item_bp.ToString());
			}
		}
	}

	void Init()
	{
		ArkApi::GetCommands().AddChatCommand(GetText("SellCmd"), &ChatSell);
		ArkApi::GetCommands().AddChatCommand(GetText("ShopSellCmd"), &ShowItems);

		ArkApi::GetCommands().AddConsoleCommand(L"ListInvItems", &ListInvItemsCmd);
	}

	void Unload()
	{
		ArkApi::GetCommands().RemoveChatCommand(GetText("SellCmd"));
		ArkApi::GetCommands().RemoveChatCommand(GetText("ShopSellCmd"));

		ArkApi::GetCommands().RemoveConsoleCommand(L"ListInvItems");
	}
} // namespace StoreSell // namespace ArkShop
