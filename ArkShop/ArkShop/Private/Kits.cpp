#include <Kits.h>

#include <DBHelper.h>
#include <ArkPermissions.h>
#include <Points.h>

#include "ArkShop.h"
#include "ShopLog.h"
#include "ArkShopUIHelper.h"

namespace ArkShop::Kits
{
	DECLARE_HOOK(AShooterCharacter_AuthPostSpawnInit, void, AShooterCharacter*);

	/**
	 * \brief Returns kits info of specific player
	 */
	nlohmann::basic_json<> GetPlayerKitsConfig(uint64 steam_id)
	{
		const std::string kits_config = database->GetPlayerKits(steam_id);

		nlohmann::json conf = nlohmann::json::object();

		try
		{
			if (kits_config.length() >= 2 && (kits_config.substr(0, 1) == "{" || kits_config.substr(kits_config.length() - 1, 1) == "}"))
				conf = nlohmann::json::parse(kits_config);
			else
				conf = nlohmann::json::parse("{}");
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Couldn't parse config: {}", __FILE__, __FUNCTION__, exception.what());
		}

		return conf;
	}

	/**
	 * \brief Saves kits info of specific player
	 */
	bool SaveConfig(const std::string& dump, uint64 steam_id)
	{
		return database->UpdatePlayerKits(steam_id, dump);
	}

	/**
	 * \brief Checks if kit exists in server config
	 */
	bool IsKitExists(const FString& kit_name)
	{
		auto kits_list = config["Kits"];

		std::string kit_name_str = kit_name.ToString();

		const auto kit_entry_iter = kits_list.find(kit_name_str);

		return kit_entry_iter != kits_list.end();
	}

	/**
	 * \brief Adds or reduces kits of the specific player
	 */
	bool ChangeKitAmount(const FString& kit_name, int amount, uint64 steam_id)
	{
		if (amount == 0)
		{
			// We got nothing to change
			return true;
		}

		std::string kit_name_str = kit_name.ToString();

		int new_amount;

		// Kits json config
		auto player_kit_json = GetPlayerKitsConfig(steam_id);

		auto kit_json_iter = player_kit_json.find(kit_name_str);
		if (kit_json_iter == player_kit_json.end()) // If kit doesn't exists in player's config
		{
			auto kits_list = config["Kits"];

			auto kit_entry_iter = kits_list.find(kit_name_str);
			if (kit_entry_iter == kits_list.end())
			{
				return false;
			}

			auto kit_entry = kit_entry_iter.value();

			const int default_amount = kit_entry.value("DefaultAmount", 0);

			new_amount = default_amount + amount;
		}
		else
		{
			auto kit_json_entry = kit_json_iter.value();

			const int current_amount = kit_json_entry.value("Amount", 0);

			new_amount = current_amount + amount;
		}

		player_kit_json[kit_name_str]["Amount"] = new_amount >= 0 ? new_amount : 0;

		bool returnValue = SaveConfig(player_kit_json.dump(), steam_id);

		if (returnValue && ArkApi::Tools::IsPluginLoaded("ArkShopUI"))
		{
			FString kitData(database->GetPlayerKits(steam_id));
			ArkShopUI::PlayerKits(steam_id, kitData);
		}

		return returnValue;
	}

	/**
	* \brief Checks if player has permissions to use this kit
	*/
	bool CanUseKit(AShooterPlayerController* player_controller, uint64 steam_id, const FString& kit_name)
	{
		if (player_controller == nullptr || ArkApi::IApiUtils::IsPlayerDead(player_controller))
		{
			return false;
		}

		auto kits_list = config["Kits"];

		std::string kit_name_str = kit_name.ToString();

		const auto kit_entry_iter = kits_list.find(kit_name_str);
		if (kit_entry_iter == kits_list.end())
		{
			return false;
		}

		auto kit_entry = kit_entry_iter.value();

		const int min_level = kit_entry.value("MinLevel", 1);
		const int max_level = kit_entry.value("MaxLevel", 999);

		auto* primal_character = static_cast<APrimalCharacter*>(player_controller->CharacterField());
		UPrimalCharacterStatusComponent* char_component = primal_character->MyCharacterStatusComponentField();
		if (char_component == nullptr)
		{
			return false;
		}

		const int level = char_component->BaseCharacterLevelField() + char_component->ExtraCharacterLevelField();
		if (level < min_level || level > max_level)
		{
			return false;
		}

		const std::string permissions = kit_entry.value("Permissions", "");
		if (permissions.empty())
		{
			return true;
		}

		const FString fpermissions(permissions);

		TArray<FString> groups;
		fpermissions.ParseIntoArray(groups, L",", true);

		for (const auto& group : groups)
		{
			if (Permissions::IsPlayerInGroup(steam_id, group))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * \brief Returns amount of kits player has
	 */
	int GetKitAmount(uint64 steam_id, const FString& kit_name)
	{
		std::string kit_name_str = kit_name.ToString();

		auto player_kit_json = GetPlayerKitsConfig(steam_id);

		auto kit_json_iter = player_kit_json.find(kit_name_str);
		if (kit_json_iter != player_kit_json.end())
		{
			auto kit_json_entry = kit_json_iter.value();

			return kit_json_entry.value("Amount", 0);
		}

		// Return default amount if player didn't use this kit yet

		auto kits_list = config["Kits"];

		const auto kit_entry_iter = kits_list.find(kit_name_str);
		if (kit_entry_iter != kits_list.end())
		{
			return kit_entry_iter.value().value("DefaultAmount", 0);
		}

		return 0;
	}

	/**
	 * \brief Redeem the kit from the given config entry
	 */
	void GiveKitFromJson(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& kit_entry)
	{
		// Give items
		auto items_map = kit_entry.value("Items", nlohmann::json::array());
		for (const auto& item : items_map)
		{
			const float quality = item.value("Quality", 0);
			const bool force_blueprint = item.value("ForceBlueprint", false);
			const int amount = item.value("Amount", 1);
			std::string blueprint = item.value("Blueprint", "");
			int armor = item.value("Armor", 0);
			int durability = item.value("Durability", 0);
			int damage = item.value("Damage", 0);

			FString fblueprint(blueprint.c_str());

			TArray<UPrimalItem*> out_items;
			player_controller->GiveItem(&out_items, &fblueprint, amount, quality, force_blueprint, false, 0);
			ApplyItemStats(out_items, armor, durability, damage);
		}

		// Give dinos
		auto dinos_map = kit_entry.value("Dinos", nlohmann::json::array());
		for (const auto& dino : dinos_map)
		{
			const int level = dino.value("Level", 1);
			const bool neutered = dino.value("Neutered", false);
			std::string gender = dino.value("Gender", "random");
			std::string saddleblueprint = dino.value("SaddleBlueprint", "");
			std::string blueprint = dino.value("Blueprint", "");
			bool preventCryo = dino.value("PreventCryo", false);
			const int stryderhead = dino.value("StryderHead", -1);
			const int stryderchest = dino.value("StryderChest", -1);
			nlohmann::json resourceoverrides = dino.value("GachaResources", nlohmann::json());

			bool success = ArkShop::GiveDino(player_controller, level, neutered, gender, blueprint, saddleblueprint, preventCryo, stryderhead, stryderchest, resourceoverrides);
		}

		// Give commands
		uint64 steam_id = ArkApi::GetApiUtils().GetSteamIdFromController(player_controller);
		auto commands_map = kit_entry.value("Commands", nlohmann::json::array());
		for (const auto& command_entry : commands_map)
		{
			const std::string command = command_entry.value("Command", "");

			const bool exec_as_admin = command_entry.value("ExecuteAsAdmin", false);

			FString fcommand = fmt::format(
				command, fmt::arg("steamid", steam_id),
				fmt::arg("playerid", ArkApi::GetApiUtils().GetPlayerID(player_controller)),
				fmt::arg("tribeid", ArkApi::GetApiUtils().GetTribeID(player_controller))
			).c_str();

			const bool was_admin = player_controller->bIsAdmin()();

			if (!was_admin && exec_as_admin)
				player_controller->bIsAdmin() = true;

			FString result;
			((APlayerController*)player_controller)->ConsoleCommand(&result, &fcommand, false);

			if (!was_admin && exec_as_admin)
				player_controller->bIsAdmin() = false;
		}
	}

	/**
	 * \brief Redeem the kit for the specific player
	 */
	void RedeemKit(AShooterPlayerController* player_controller, const FString& kit_name, bool should_log, bool from_spawn)
	{
		if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
		{
			return;
		}

		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		if (DBHelper::IsPlayerExists(steam_id))
		{
			if (!CanUseKit(player_controller, steam_id, kit_name))
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("NoPermissionsKit"));
				return;
			}

			if (player_controller->GetPlayerInventoryComponent() && player_controller->GetPlayerInventoryComponent()->IsAtMaxInventoryItems())
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("InventoryIsFull"));
				return;
			}

			std::string kit_name_str = kit_name.ToString();

			auto kits_list = config["Kits"];

			auto kit_entry_iter = kits_list.find(kit_name_str);
			if (kit_entry_iter == kits_list.end())
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("WrongId"));
				return;
			}

			const auto kit_entry = kit_entry_iter.value();

			if (!from_spawn && kit_entry.value("OnlyFromSpawn", false))
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("OnlyOnSpawnKit"));
				return;
			}

			if (const int kit_amount = GetKitAmount(steam_id, kit_name);
				kit_amount > 0 && ChangeKitAmount(kit_name, -1, steam_id))
			{
				GiveKitFromJson(player_controller, kit_entry);

				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("KitsLeft"), kit_amount - 1, *kit_name);

				// Log
				if (should_log)
				{
					const std::wstring log = fmt::format(TEXT("[{}] {}({}) Used kit '{}'"),
						*ArkShop::SetMapName(),
						*ArkApi::IApiUtils::GetSteamName(player_controller), steam_id,
						*kit_name);

					ShopLog::GetLog()->info(ArkApi::Tools::Utf8Encode(log));
					ArkShop::PostToDiscord(log);
				}
			}
			else if (should_log)
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("NoKitsLeft"), *kit_name);
			}
		}
	}

	/**
	 * \brief Lists all available kits using notification
	 */
	void ListKits(AShooterPlayerController* player_controller)
	{
		const float display_time = config["General"].value("ShopDisplayTime", 15.0f);
		const float text_size = config["General"].value("ShopTextSize", 1.3f);

		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		FString kits_str = "";

		auto kits_map = config["Kits"];
		for (auto iter = kits_map.begin(); iter != kits_map.end(); ++iter)
		{
			const std::string kit_name_str = iter.key();
			const FString kit_name(kit_name_str.c_str());

			auto iter_value = iter.value();

			const int price = iter_value.value("Price", -1);

			if (const int amount = GetKitAmount(steam_id, kit_name);
				(amount > 0 || price != -1) && CanUseKit(player_controller, steam_id, kit_name))
			{
				const std::wstring description = ArkApi::Tools::Utf8Decode(
					iter_value.value("Description", "No description"));

				std::wstring price_str = price != -1 ? fmt::format(*GetText("KitsListPrice"), price) : L"";

				kits_str += FString::Format(*GetText("KitsListFormat"), *kit_name, description, amount, price_str);
			}
		}

		if (kits_str.IsEmpty())
		{
			kits_str = GetText("NoKits");
		}

		const FString kits_list_str = FString::Format(TEXT("{}\n{}\n{}"), *GetText("AvailableKits"), *kits_str,
			*GetText("KitUsage"));

		ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::Green, text_size, display_time, nullptr,
			*kits_list_str);
	}

	void InitKitData(uint64 steam_id)
	{
		//Kits json config
		auto player_kit_json = GetPlayerKitsConfig(steam_id);

		auto kits_map = config["Kits"];
		for (auto iter = kits_map.begin(); iter != kits_map.end(); ++iter)
		{
			const std::string kit_name_str = iter.key();
			int new_amount;

			auto kit_json_iter = player_kit_json.find(kit_name_str);
			if (kit_json_iter == player_kit_json.end()) // If kit doesn't exists in player's config
			{
				auto kits_list = config["Kits"];

				auto kit_entry_iter = kits_list.find(kit_name_str);
				if (kit_entry_iter == kits_list.end())
				{
					continue;
				}

				auto kit_entry = kit_entry_iter.value();

				const int default_amount = kit_entry.value("DefaultAmount", 0);

				new_amount = default_amount;
			}
			else
			{
				auto kit_json_entry = kit_json_iter.value();

				const int current_amount = kit_json_entry.value("Amount", 0);

				new_amount = current_amount;
			}

			player_kit_json[kit_name_str]["Amount"] = new_amount >= 0 ? new_amount : 0;
		}

		bool returnValue = SaveConfig(player_kit_json.dump(), steam_id);

		if (returnValue && ArkApi::Tools::IsPluginLoaded("ArkShopUI"))
		{
			FString kitData(database->GetPlayerKits(steam_id));
			ArkShopUI::PlayerKits(steam_id, kitData);
		}
	}

	// Chat callbacks

	void Kit(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type /*unused*/)
	{
		if (!IsStoreEnabled(player_controller))
			return;

		if (ShouldPreventStoreUse(player_controller))
			return;

		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			RedeemKit(player_controller, parsed[1], true, false);
		}
		else
		{
			if (ArkApi::Tools::IsPluginLoaded("ArkShopUI"))
			{
				uint64 steam_id = ArkApi::GetApiUtils().GetSteamIdFromController(player_controller);
				if (steam_id > 0)
				{
					FString kitData(database->GetPlayerKits(steam_id));
					ArkShopUI::PlayerKits(steam_id, kitData);
				}
				return;
			}
			else
				ListKits(player_controller);
		}
	}

	void BuyKit(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type /*unused*/)
	{
		if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
			return;

		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(2))
		{
			const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

			if (DBHelper::IsPlayerExists(steam_id))
			{
				FString kit_name = parsed[1];
				std::string kit_name_str = kit_name.ToString();

				if (!CanUseKit(player_controller, steam_id, kit_name))
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
						*GetText("CantBuyKit"));
					return;
				}

				int amount;

				try
				{
					amount = std::stoi(*parsed[2]);
				}
				catch (const std::exception& exception)
				{
					Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
					return;
				}

				if (amount <= 0)
				{
					return;
				}

				auto kits_list = config["Kits"];

				auto kit_entry_iter = kits_list.find(kit_name_str);
				if (kit_entry_iter == kits_list.end())
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
						*GetText("WrongId"));
					return;
				}

				auto kit_entry = kit_entry_iter.value();

				const int price = kit_entry.value("Price", 0);
				if (price == 0)
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
						*GetText("CantBuyKit"));
					return;
				}

				const int final_price = price * amount;
				if (final_price <= 0)
				{
					return;
				}

				if (Points::GetPoints(steam_id) >= final_price && Points::SpendPoints(final_price, steam_id))
				{
					ChangeKitAmount(kit_name, amount, steam_id);

					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
						*GetText("BoughtKit"), *kit_name);

					// Log
					const std::wstring log = fmt::format(TEXT("[{}] {}({}) Bought kit: '{}' Amount: {}"),
						*ArkShop::SetMapName(),
						*ArkApi::IApiUtils::GetSteamName(player_controller), steam_id,
						*kit_name,
						amount);

					ShopLog::GetLog()->info(ArkApi::Tools::Utf8Encode(log));
					ArkShop::PostToDiscord(log);
				}
				else
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
						*GetText("NoPoints"));
				}
			}
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
				*GetText("BuyKitUsage"));
		}
	}

	bool ChangeKitAmountCbk(const FString& cmd)
	{
		TArray<FString> parsed;
		cmd.ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(3))
		{
			const FString kit_name = parsed[2];

			uint64 steam_id;
			int amount;

			try
			{
				steam_id = std::stoull(*parsed[1]);
				amount = std::stoi(*parsed[3]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
				return false;
			}

			if (DBHelper::IsPlayerExists(steam_id))
			{
				return ChangeKitAmount(kit_name, amount, steam_id);
			}
		}

		return false;
	}

	// Console callbacks

	void ChangeKitAmountCmd(APlayerController* controller, FString* cmd, bool /*unused*/)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(controller);

		const bool result = ChangeKitAmountCbk(*cmd);
		if (result)
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green,
				"Successfully changed kit amount");
		}
		else
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't change kit amount");
		}
	}

	void ResetKitsCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		if (parsed.IsValidIndex(1))
		{
			if (parsed[1].ToString() == "confirm")
			{
				database->DeleteAllKits();

				ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green,
					"Successfully reset kits");
			}
		}
		else
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Yellow,
				"You are going to reset kits for ALL players\nType 'ResetKits confirm' in console if you want to continue");
		}
	}

	// Rcon

	void ChangeKitAmountRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
	{
		FString reply;

		const bool result = ChangeKitAmountCbk(rcon_packet->Body);
		if (result)
		{
			reply = "Successfully changed kit amount";
		}
		else
		{
			reply = "Couldn't change kit amount";
		}

		rcon_connection->SendMessageW(rcon_packet->Id, 0, &reply);
	}

	// Hook

	void Hook_AShooterCharacter_AuthPostSpawnInit(AShooterCharacter* _this)
	{
		AShooterCharacter_AuthPostSpawnInit_original(_this);

		AShooterPlayerController* player = ArkApi::GetApiUtils().FindControllerFromCharacter(static_cast<AShooterCharacter*>(_this));
		if (player != nullptr)
		{
			const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player);
			if (steam_id > 0)
				database->TryAddNewPlayer(steam_id);

			const std::string default_kit = config["General"].value("DefaultKit", "");
			if (!default_kit.empty())
			{
				try
				{
					const FString fdefault_kit(default_kit);

					TArray<FString> kits;
					fdefault_kit.ParseIntoArray(kits, L",", true);

					for (const auto& kit : kits)
					{
						if (const int kit_amount = GetKitAmount(steam_id, kit);
							kit_amount > 0 && CanUseKit(player, steam_id, kit))
						{
							RedeemKit(player, kit, false, true);
							break;
						}
					}
				}
				catch (const std::exception& exception)
				{
					Log::GetLog()->error("({} {}) Unexpected error {}", __FILE__, __FUNCTION__, exception.what());
				}
			}
		}
	}

	void Init()
	{
		auto& commands = ArkApi::GetCommands();

		commands.AddChatCommand(GetText("KitCmd"), &Kit);
		commands.AddChatCommand(GetText("BuyKitCmd"), &BuyKit);

		commands.AddConsoleCommand("ChangeKitAmount", &ChangeKitAmountCmd);
		commands.AddConsoleCommand("ResetKits", &ResetKitsCmd);

		commands.AddRconCommand("ChangeKitAmount", &ChangeKitAmountRcon);

		ArkApi::GetHooks().SetHook("AShooterCharacter.AuthPostSpawnInit", &Hook_AShooterCharacter_AuthPostSpawnInit,
			&AShooterCharacter_AuthPostSpawnInit_original);
	}

	void Unload()
	{
		auto& commands = ArkApi::GetCommands();

		commands.RemoveChatCommand(GetText("KitCmd"));
		commands.RemoveChatCommand(GetText("BuyKitCmd"));

		commands.RemoveConsoleCommand("ChangeKitAmount");
		commands.RemoveConsoleCommand("ResetKits");

		commands.RemoveRconCommand("ChangeKitAmount");

		ArkApi::GetHooks().DisableHook("AShooterCharacter.AuthPostSpawnInit",
			&Hook_AShooterCharacter_AuthPostSpawnInit);
	}
} // namespace Kits // namespace ArkShop