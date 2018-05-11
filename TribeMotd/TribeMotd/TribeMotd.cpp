#include "TribeMotd.h"

#include <API/ARK/Ark.h>
#include <Tools.h>

#include <fstream>

#include "DBHelper.h"
#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(AShooterPlayerController_ServerReadMessageOFTheDay_Impl, void, AShooterPlayerController*);

nlohmann::json config;

sqlite::database& GetDB()
{
	static sqlite::database db(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/TribeMotd/ArkDB.db");
	return db;
}

FString GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Messages"].value(str, "No message")).c_str());
}

void TribeMotd(AShooterPlayerController* player)
{
	FString sender;
	std::string message;

	const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player);

	if (DBHelper::IsPlayerEntryExists(steam_id))
	{
		try
		{
			auto& db = GetDB();
			db << "SELECT Message FROM PlayerMotd WHERE SteamId = ?;" << steam_id >> message;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		sender = "Admin";
	}
	else
	{
		FTribeData* tribe = static_cast<AShooterPlayerState*>(player->PlayerStateField())->
			MyTribeDataField();

		if (!tribe)
			return;

		const int tribe_id = tribe->TribeIDField();
		if (tribe_id == 0 || !DBHelper::IsEntryExists(tribe_id))
			return;

		try
		{
			auto& db = GetDB();
			db << "SELECT Message FROM TribeMotd WHERE TribeId = ?;" << tribe_id >> message;
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		sender = tribe->TribeNameField();
	}

	std::wstring wmsg = ArkApi::Tools::Utf8Decode(message);

	const std::string type = config["MessageType"];

	if (type == "ServerChat")
	{
		auto config_color = config["Color"];
		const FLinearColor color{config_color[0], config_color[1], config_color[2], config_color[3]};

		ArkApi::GetApiUtils().SendServerMessage(player, color, L"{}: {}", *sender, wmsg.c_str());
	}
	else if (type == "Notification")
	{
		const float display_scale = config["DisplayScale"];
		const float display_time = config["DisplayTime"];

		auto config_color = config["Color"];
		const FLinearColor color{config_color[0], config_color[1], config_color[2], config_color[3]};

		ArkApi::GetApiUtils().SendNotification(player, color, display_scale, display_time,
		                                       nullptr, L"{}: {}", *sender, wmsg.c_str());
	}
}

void Hook_AShooterPlayerController_ServerReadMessageOFTheDay_Impl(AShooterPlayerController* _this)
{
	AShooterPlayerController_ServerReadMessageOFTheDay_Impl_original(_this);

	TribeMotd(_this);
}

void SetMotd(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
{
	if (player_controller->IsTribeAdmin())
	{
		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			if (!message->RemoveFromStart("/SetMotd "))
				return;

			FTribeData* tribe = static_cast<AShooterPlayerState*>(player_controller->PlayerStateField())->
				MyTribeDataField();

			if (!tribe)
				return;

			try
			{
				auto& db = GetDB();

				const int tribe_id = tribe->TribeIDField();
				if (DBHelper::IsEntryExists(tribe_id))
				{
					db << "UPDATE TribeMotd SET Message = ? WHERE TribeId = ?;" << ArkApi::Tools::Utf8Encode(**message) << tribe_id;
				}
				else
				{
					db << "INSERT INTO TribeMotd (TribeId, Message) VALUES (?, ?);" << tribe_id
						<< ArkApi::Tools::Utf8Encode(**message);
				}
			}
			catch (const sqlite::sqlite_exception& exception)
			{
				Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
				return;
			}

			ArkApi::GetApiUtils().SendChatMessage(player_controller, *GetText("Sender"), *GetText("ChangedMotd"));
		}
	}
	else
	{
		ArkApi::GetApiUtils().SendChatMessage(player_controller, *GetText("Sender"), *GetText("NotAdmin"));
	}
}

void AdminSetTribeMotd(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		if (!cmd->RemoveFromStart(L"SetTribeMotd " + parsed[1]))
			return;

		try
		{
			const int tribe_id = std::stoi(*parsed[1]);

			auto& db = GetDB();

			if (DBHelper::IsEntryExists(tribe_id))
			{
				db << "UPDATE TribeMotd SET Message = ? WHERE TribeId = ?;" << ArkApi::Tools::Utf8Encode(**cmd) << tribe_id;
			}
			else
			{
				db << "INSERT INTO TribeMotd (TribeId, Message) VALUES (?, ?);" << tribe_id
					<< ArkApi::Tools::Utf8Encode(**cmd);
			}
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		ArkApi::GetApiUtils().SendChatMessage(static_cast<AShooterPlayerController*>(player_controller), *GetText("Sender"),
		                                      *GetText("ChangedMotd"));
	}
}

void AdminSetPlayerMotd(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(2))
	{
		if (!cmd->RemoveFromStart(L"SetPlayerMotd " + parsed[1]))
			return;

		try
		{
			const uint64 steam_id = std::stoull(*parsed[1]);

			auto& db = GetDB();

			if (DBHelper::IsPlayerEntryExists(steam_id))
			{
				db << "UPDATE PlayerMotd SET Message = ? WHERE SteamId = ?;" << ArkApi::Tools::Utf8Encode(**cmd) << steam_id;
			}
			else
			{
				db << "INSERT INTO PlayerMotd (SteamId, Message) VALUES (?, ?);" << steam_id
					<< ArkApi::Tools::Utf8Encode(**cmd);
			}
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		ArkApi::GetApiUtils().SendChatMessage(static_cast<AShooterPlayerController*>(player_controller), *GetText("Sender"),
		                                      *GetText("ChangedMotd"));
	}
}

void AdminRemoveTribeMotd(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		try
		{
			const int tribe_id = std::stoi(*parsed[1]);
			if (DBHelper::IsEntryExists(tribe_id))
			{
				auto& db = GetDB();
				db << "DELETE FROM TribeMotd WHERE TribeId = ?;" << tribe_id;
			}
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		ArkApi::GetApiUtils().SendChatMessage(static_cast<AShooterPlayerController*>(player_controller), *GetText("Sender"),
		                                      *GetText("RemovedMotd"));
	}
}

void AdminRemovePlayerMotd(APlayerController* player_controller, FString* cmd, bool)
{
	TArray<FString> parsed;
	cmd->ParseIntoArray(parsed, L" ", true);

	if (parsed.IsValidIndex(1))
	{
		try
		{
			const uint64 steam_id = std::stoull(*parsed[1]);
			if (DBHelper::IsPlayerEntryExists(steam_id))
			{
				auto& db = GetDB();
				db << "DELETE FROM PlayerMotd WHERE SteamId = ?;" << steam_id;
			}
		}
		catch (const sqlite::sqlite_exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected error {}", __FILE__, __FUNCTION__, exception.what());
			return;
		}

		ArkApi::GetApiUtils().SendChatMessage(static_cast<AShooterPlayerController*>(player_controller), *GetText("Sender"),
		                                      *GetText("RemovedMotd"));
	}
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/TribeMotd/config.json";
	std::ifstream file{config_path};
	if (!file.is_open())
		throw std::runtime_error("Can't open config.json");

	file >> config;

	file.close();
}

void Load()
{
	Log::Get().Init("TribeMotd");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	auto& db = GetDB();

	db << "create table if not exists TribeMotd ("
		"Id integer primary key autoincrement not null,"
		"TribeId integer not null,"
		"Message text"
		");";
	db << "create table if not exists PlayerMotd ("
		"Id integer primary key autoincrement not null,"
		"SteamId integer not null,"
		"Message text"
		");";
	db << "CREATE UNIQUE INDEX IF NOT EXISTS IdxTribeId ON TribeMotd (TribeId);";

	ArkApi::GetHooks().SetHook("AShooterPlayerController.ServerReadMessageOFTheDay_Implementation",
	                           &Hook_AShooterPlayerController_ServerReadMessageOFTheDay_Impl,
	                           &AShooterPlayerController_ServerReadMessageOFTheDay_Impl_original);

	ArkApi::GetCommands().AddChatCommand("/SetMotd", &SetMotd);
	ArkApi::GetCommands().AddConsoleCommand("SetTribeMotd", &AdminSetTribeMotd);
	ArkApi::GetCommands().AddConsoleCommand("SetPlayerMotd", &AdminSetPlayerMotd);
	ArkApi::GetCommands().AddConsoleCommand("RemoveTribeMotd", &AdminRemoveTribeMotd);
	ArkApi::GetCommands().AddConsoleCommand("RemovePlayerMotd", &AdminRemovePlayerMotd);
}

void Unload()
{
	ArkApi::GetHooks().DisableHook("AShooterPlayerController.ServerReadMessageOFTheDay_Implementation",
	                               &Hook_AShooterPlayerController_ServerReadMessageOFTheDay_Impl);

	ArkApi::GetCommands().RemoveChatCommand("/SetMotd");
	ArkApi::GetCommands().RemoveConsoleCommand("SetTribeMotd");
	ArkApi::GetCommands().RemoveConsoleCommand("SetPlayerMotd");
	ArkApi::GetCommands().RemoveConsoleCommand("RemoveTribeMotd");
	ArkApi::GetCommands().RemoveConsoleCommand("RemovePlayerMotd");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		Unload();
		break;
	}
	return TRUE;
}
