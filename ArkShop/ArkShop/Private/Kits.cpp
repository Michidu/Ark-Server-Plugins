#include <Kits.h>

#include <Permissions.h>
#include <Points.h>
#include <DBHelper.h>

#include "ArkShop.h"
#include "ShopLog.h"

namespace ArkShop::Kits
{
	/**
	 * \brief Returns kits info of specific player
	 */
	nlohmann::basic_json<> GetPlayerKitsConfig(uint64 steam_id)
	{
		auto& db = GetDB();

		std::string kits_config = "{}";

		try
		{
			db << "SELECT Kits FROM Players WHERE SteamId = ?;" << steam_id >> kits_config;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return nlohmann::json::parse(kits_config);
	}

	/**
	 * \brief Saves kits info of specific player
	 */
	bool SaveConfig(const std::string& dump, uint64 steam_id)
	{
		auto& db = GetDB();

		try
		{
			db << "UPDATE Players SET Kits = ? WHERE SteamId = ?;" << dump << steam_id;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
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
		if (amount == 0) // We got nothing to change
			return true;

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
				return false;

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

		return SaveConfig(player_kit_json.dump(), steam_id);
	}

	/**
	* \brief Checks if player has permissions to use this kit
	*/
	bool CanUseKit(uint64 steam_id, const FString& kit_name)
	{
		auto kits_list = config["Kits"];

		std::string kit_name_str = kit_name.ToString();

		const auto kit_entry_iter = kits_list.find(kit_name_str);
		if (kit_entry_iter == kits_list.end())
			return false;

		const std::string permissions = kit_entry_iter.value().value("Permissions", "");
		if (permissions.empty())
			return true;

		const FString fpermissions(permissions.c_str());

		TArray<FString> groups;
		fpermissions.ParseIntoArray(groups, L",", true);

		for (const auto& group : groups)
		{
			if (Permissions::IsPlayerInGroup(steam_id, group))
				return true;
		}

		return false;
	}

	/**
	 * \brief Returns amount of kits player has
	 */
	int GetKitAmount(uint64 steam_id, const FString& kit_name)
	{
		auto kits_list = config["Kits"];

		std::string kit_name_str = kit_name.ToString();

		auto player_kit_json = GetPlayerKitsConfig(steam_id);

		auto kit_json_iter = player_kit_json.find(kit_name_str);
		if (kit_json_iter != player_kit_json.end())
		{
			auto kit_json_entry = kit_json_iter.value();

			return kit_json_entry.value("Amount", 0);
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
			const int amount = item["Amount"];
			const float quality = item["Quality"];
			const bool force_blueprint = item["ForceBlueprint"];
			std::string blueprint = item["Blueprint"];

			FString fblueprint(blueprint.c_str());

			player_controller->GiveItem(&fblueprint, amount, quality, force_blueprint);
		}

		// Give dinos
		auto dinos_map = kit_entry.value("Dinos", nlohmann::json::array());
		for (const auto& dino : dinos_map)
		{
			const int level = dino["Level"];
			std::string blueprint = dino["Blueprint"];

			const FString fblueprint(blueprint.c_str());

			ArkApi::GetApiUtils().SpawnDino(player_controller, fblueprint, nullptr, level, true);
		}
	}

	/**
	 * \brief Redeem the kit for the specific player
	 */
	void RedeemKit(AShooterPlayerController* player_controller, const FString& kit_name)
	{
		if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
			return;

		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		if (DBHelper::IsPlayerExists(steam_id))
		{
			if (!CanUseKit(steam_id, kit_name))
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
				                                      *GetText("NoPermissionsKit"));
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

			if (const int kit_amount = GetKitAmount(steam_id, kit_name);
				kit_amount > 0 && ChangeKitAmount(kit_name, -1, steam_id))
			{
				GiveKitFromJson(player_controller, kit_entry_iter.value());

				ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
				                                      *GetText("KitsLeft"), kit_amount - 1, *kit_name);

				// Log
				const std::wstring log = fmt::format(TEXT("{}({}) used kit \"{}\""),
				                                     *ArkApi::IApiUtils::GetSteamName(player_controller), steam_id, *kit_name);

				ShopLog::GetLog()->info(ArkApi::Tools::ConvertToAnsiStr(log));
			}
			else
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
				(amount > 0 || price != -1) && CanUseKit(steam_id, kit_name))
			{
				const std::string description = iter_value.value("Description", "No description");

				std::string price_str = price != -1 ? fmt::format("Price: {}", price) : "";

				kits_str += FString::Format("\"{}\" - {}. {} left. {}\n", kit_name_str, description, amount, price_str);
			}
		}

		if (kits_str.IsEmpty())
			kits_str = GetText("NoKits");

		const FString kits_list_str = FString::Format(TEXT("{}\n{}\n{}"), *GetText("AvailableKits"), *kits_str,
		                                              *GetText("KitUsage"));

		ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::White, text_size, display_time, nullptr,
		                                       *kits_list_str);
	}

	// Chat callbacks

	void Kit(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
	{
		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			RedeemKit(player_controller, parsed[1]);
		}
		else
		{
			ListKits(player_controller);
		}
	}

	void BuyKit(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
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
					return;

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
					return;

				if (Points::GetPoints(steam_id) >= final_price && Points::SpendPoints(final_price, steam_id))
				{
					ChangeKitAmount(kit_name, amount, steam_id);

					ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					                                      *GetText("BoughtKit"), *kit_name);

					// Log
					const std::wstring log = fmt::format(TEXT("{}({}) bought kit \"{}\". Amount - {}"),
					                                     *ArkApi::IApiUtils::GetSteamName(player_controller), steam_id, *kit_name,
					                                     amount);

					ShopLog::GetLog()->info(ArkApi::Tools::ConvertToAnsiStr(log));
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

	// Console callbacks

	void ChangeKitAmountCmd(APlayerController* controller, FString* cmd, bool)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

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
				return;
			}

			if (DBHelper::IsPlayerExists(steam_id))
			{
				const auto shooter_controller = static_cast<AShooterPlayerController*>(controller);

				const bool result = ChangeKitAmount(kit_name, amount, steam_id);
				if (result)
					ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully changed kit amount");
				else
					ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Red, "Couldn't change kit amount");
			}
		}
	}

	void ResetKitsCmd(APlayerController* player_controller, FString* cmd, bool)
	{
		TArray<FString> parsed;
		cmd->ParseIntoArray(parsed, L" ", true);

		AShooterPlayerController* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);

		if (parsed.IsValidIndex(1))
		{
			if (parsed[1].ToString() == "confirm")
			{
				auto& db = GetDB();

				db << "UPDATE Players SET Kits = \"{}\";";

				ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Green, "Successfully reset kits");
			}
		}
		else
		{
			ArkApi::GetApiUtils().SendServerMessage(shooter_controller, FColorList::Yellow,
			                                        "You are going to reset kits for ALL players\nType 'ResetKits confirm' in console if you want to continue");
		}
	}

	void Init()
	{
		auto& commands = ArkApi::GetCommands();

		commands.AddChatCommand(GetText("KitCmd"), &Kit);
		commands.AddChatCommand(GetText("BuyKitCmd"), &BuyKit);

		commands.AddConsoleCommand("ChangeKitAmount", &ChangeKitAmountCmd);
		commands.AddConsoleCommand("ResetKits", &ResetKitsCmd);
	}
}
