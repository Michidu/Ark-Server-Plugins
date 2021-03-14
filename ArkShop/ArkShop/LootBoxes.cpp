#include "../LootBoxes.h"

#include <DBHelper.h>
#include <API/ARK/ArkPermissions.h>
#include <Points.h>
#include "Private/ShopLog.h"
#include "Private/ArkShop.h"
#include "Private/Database/MysqlDB.h"
#include "Private/Database/SqlLiteDB.h"
#include "RandomRewards.h"
#include <random>
#include <iostream>


namespace ArkShop::LootBoxes
{
	FString getConfigMessage(const std::string& key) {
		return FString(ArkApi::Tools::ConvertToAnsiStr(ArkApi::Tools::Utf8Decode((config["MessagesCajas"].value(key, "Unable to find key \"" + key + "\" in config file!")))));
	}

	nlohmann::basic_json<> GetPlayerCajasConfig(uint64 steam_id)
	{
		const std::string Cajas_config = database->GetPlayerCajas(steam_id);

		nlohmann::json conf = nlohmann::json::object();

		try
		{
			conf = nlohmann::json::parse(Cajas_config);
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Couldn't parse config: {}", __FILE__, __FUNCTION__, exception.what());
		}

		return conf;
	}

	int GetCajasAmount(uint64 steam_id, const FString& kit_name)
	{
		std::string kit_name_str = kit_name.ToString();

		auto player_kit_json = GetPlayerCajasConfig(steam_id);

		auto kit_json_iter = player_kit_json.find(kit_name_str);
		if (kit_json_iter != player_kit_json.end())
		{
			auto kit_json_entry = kit_json_iter.value();

			return kit_json_entry.value("Amount", 0);
		}

		// Return default amount if player didn't use this kit yet

		auto kits_list = config["Cajas"];

		const auto kit_entry_iter = kits_list.find(kit_name_str);
		if (kit_entry_iter != kits_list.end())
		{
			return kit_entry_iter.value().value("DefaultAmount", 0);
		}

		return 0;
	}
	
	/**
	 * \brief Lists all available kits using notification
	 */
	void ListCajas(AShooterPlayerController* player_controller)
	{
		const float display_time = config["General"].value("ShopDisplayTime", 15.0f);
		const float text_size = config["General"].value("ShopTextSize", 1.3f);

		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		FString kits_str = "";

		auto kits_map = config["Cajas"];
		for (auto iter = kits_map.begin(); iter != kits_map.end(); ++iter)
		{
			const std::string kit_name_str = iter.key();
			const FString kit_name(kit_name_str.c_str());

			auto iter_value = iter.value();
			const int price = iter_value.value("Price", -1);

			if (const int amount = GetCajasAmount(steam_id, kit_name);
				(amount > 0 || price != -1) && CanUseCajas(steam_id, kit_name))
			{
				const std::wstring description = ArkApi::Tools::Utf8Decode(
					iter_value.value("Description", "No description"));

				std::wstring price_str = price != -1 ? fmt::format(*getConfigMessage("CajasListPrice"), price) : L"";

				kits_str += FString::Format(*getConfigMessage("CajasListFormat"), *kit_name, description, amount, price_str);
			}
		}

		if (kits_str.IsEmpty())
		{
			kits_str = getConfigMessage("NoBox");
		}

		const FString kits_list_str = FString::Format(TEXT("{}\n{}\n{}"), *getConfigMessage("AvailableCajas"), *kits_str,
			*getConfigMessage("CajasUsage"));

		ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::Green, text_size, display_time, nullptr,
			*kits_list_str);
	}


	bool CanUseCajas(uint64 steamId, FString lootboxname)
	{

		auto kits_list = config["Cajas"];

		const auto kit_entry_iter = kits_list.find(lootboxname.ToString());
		if (kit_entry_iter == kits_list.end())
		{
			return false;
		}

		auto kit_entry = kit_entry_iter.value();

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
			if (Permissions::IsPlayerInGroup(steamId, group))
			{
				return true;
			}
		}

		return false;
	}

	
	void RedeemCajas(AShooterPlayerController* player_controller, const FString& kit_name, bool should_log,
		bool from_spawn)
	{
		if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
		{
			return;
		}

		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		if (DBHelper::IsPlayerExists(steam_id))
		{
			if (!CanUseCajas(steam_id, kit_name))
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, *getConfigMessage("Sender"),
					*getConfigMessage("NoPermissionsKit"));
				return;
			}

			std::string kit_name_str = kit_name.ToString();

			auto kits_list = config["Cajas"];

			auto kit_entry_iter = kits_list.find(kit_name_str);
			if (kit_entry_iter == kits_list.end())
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, *getConfigMessage("Sender"),
					*getConfigMessage("WrongId"));
				return;
			}

			const auto kit_entry = kit_entry_iter.value();

			if (const int kit_amount = GetCajasAmount(steam_id, kit_name);
				kit_amount > 0 && ChangeCajasAmount(kit_name, -1, steam_id))
			{
					
				Random::generateAndGiveRewards(player_controller, kit_name);
				
				ArkApi::GetApiUtils().SendChatMessage(player_controller, *getConfigMessage("Sender"),
					*getConfigMessage("CajasLeft"), kit_amount - 1, *kit_name);

				// Log
				if (should_log)
				{
					const std::wstring log = fmt::format(TEXT("{}({}) used Cajas \"{}\""),
						*ArkApi::IApiUtils::GetSteamName(player_controller), steam_id,
						*kit_name);

					ShopLog::GetLog()->info(ArkApi::Tools::Utf8Encode(log));
				}
			}
			else if (should_log)
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, *getConfigMessage("Sender"),
					*getConfigMessage("NoCajasLeft"), *kit_name);
			}
		}
	}



	bool SaveConfig(const std::string& dump, uint64 steam_id)
	{
		return database->UpdatePlayerCajas(steam_id, dump);
	}

	bool ChangeCajasAmount(const FString& kit_name, int amount, uint64 steam_id)
	{
		if (amount == 0)
		{
			// We got nothing to change
			return true;
		}

		std::string kit_name_str = kit_name.ToString();

		int new_amount;

		// Kits json config
		auto player_kit_json = GetPlayerCajasConfig(steam_id);

		auto kit_json_iter = player_kit_json.find(kit_name_str);
		if (kit_json_iter == player_kit_json.end()) // If kit doesn't exists in player's config
		{
			auto kits_list = config["Cajas"];

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

		return SaveConfig(player_kit_json.dump(), steam_id);
	}


	void Kit(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type /*unused*/)
	{
		if (!IsStoreEnabled(player_controller))
		{
			return;
		}

		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			RedeemCajas(player_controller, parsed[1], true, false);
		}
		else
		{
			ListCajas(player_controller);
		}
	}

	void BuyCajas(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type /*unused*/)
	{
		if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
		{
			return;
		}

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
				{
					return;
				}

				auto kits_list = config["Cajas"];

				auto kit_entry_iter = kits_list.find(kit_name_str);
				if (kit_entry_iter == kits_list.end())
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, *getConfigMessage("Sender"),
						*getConfigMessage("WrongId"));
					return;
				}

				auto kit_entry = kit_entry_iter.value();

				const int price = kit_entry.value("Price", 0);
				if (price == 0)
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, *getConfigMessage("Sender"),
						*getConfigMessage("CantBuyCajas"));
					return;
				}

				const int final_price = price * amount;
				if (final_price <= 0)
				{
					return;
				}

				if (Points::GetPoints(steam_id) >= final_price && Points::SpendPoints(final_price, steam_id))
				{
					ChangeCajasAmount(kit_name, amount, steam_id);

					ArkApi::GetApiUtils().SendChatMessage(player_controller, getConfigMessage("Sender"),
						*getConfigMessage("BoughtCajas"), *kit_name);

					// Log
					const std::wstring log = fmt::format(TEXT("{}({}) bought Cajas \"{}\". Amount - {}"),
						*ArkApi::IApiUtils::GetSteamName(player_controller), steam_id,
						*kit_name,
						amount);

					ShopLog::GetLog()->info(ArkApi::Tools::Utf8Encode(log));
				}
				else
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, *getConfigMessage("Sender"),
						*getConfigMessage("NoPoints"));
				}
			}
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, *getConfigMessage("Sender"),
				*getConfigMessage("BuyCajasUsage"));
		}
	}

	bool ChangeCajasAmountCbk(const FString& cmd)
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
				return ChangeCajasAmount(kit_name, amount, steam_id);
			}
		}

		return false;
	}

	// Console callbacks

	void ChangeCajasAmountCmd(APlayerController* controller, FString* cmd, bool /*unused*/)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(controller);

		const bool result = ChangeCajasAmountCbk(*cmd);
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
	void ChangeCajasAmountRcon(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld* /*unused*/)
	{
		FString reply;

		const bool result = ChangeCajasAmountCbk(rcon_packet->Body);
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

	void Init()
	{
		auto& commands = ArkApi::GetCommands();

		commands.AddChatCommand(getConfigMessage("BoxListCmd"), &Kit);
		commands.AddChatCommand(getConfigMessage("BuyCajasCmd"), &BuyCajas);
		commands.AddRconCommand("ChangeCajasAmount", &ChangeCajasAmountRcon);
		commands.AddConsoleCommand("ChangeCajasAmount", &ChangeCajasAmountCmd);

	}

	void Unload()
	{
		auto& commands = ArkApi::GetCommands();

		commands.RemoveChatCommand(getConfigMessage("BoxListCmd"));
		commands.RemoveChatCommand(getConfigMessage("BuyKitCmd"));
		commands.RemoveRconCommand("ChangeCajasAmount");

		commands.RemoveConsoleCommand("ChangeCajasAmount");

		
	}
} // namespace LootBoxes // namespace ArkShop
