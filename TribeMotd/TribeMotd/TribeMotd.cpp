#include "TribeMotd.h"

#include <API/ARK/Ark.h>
#include <Tools.h>

#include <fstream>

#include "json.hpp"
#include "DBHelper.h"

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
	FTribeData* tribe = static_cast<AShooterPlayerState*>(player->PlayerStateField()())->
		MyTribeDataField()();

	if (!tribe)
		return;

	const int tribe_id = tribe->TribeIDField()();
	if (tribe_id == 0 || !DBHelper::IsEntryExists(tribe_id))
		return;

	std::string message;

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

	const FString tribe_name = tribe->TribeNameField()();

	std::wstring wmsg = ArkApi::Tools::Utf8Decode(message);

	const std::string type = config["MessageType"];

	if (type == "ServerChat")
	{
		auto config_color = config["Color"];
		const FLinearColor color{config_color[0], config_color[1], config_color[2], config_color[3]};

		ArkApi::GetApiUtils().SendServerMessage(player, color, L"{}: {}", *tribe_name, wmsg.c_str());
	}
	else if (type == "Notification")
	{
		const float display_scale = config["DisplayScale"];
		const float display_time = config["DisplayTime"];

		auto config_color = config["Color"];
		const FLinearColor color{config_color[0], config_color[1], config_color[2], config_color[3]};

		ArkApi::GetApiUtils().SendNotification(player, color, display_scale, display_time,
		                                       nullptr, L"{}: {}", *tribe_name, wmsg.c_str());
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

			FTribeData* tribe = static_cast<AShooterPlayerState*>(player_controller->PlayerStateField()())->
				MyTribeDataField()();

			if (!tribe)
				return;

			try
			{
				auto& db = GetDB();
				db << "REPLACE INTO TribeMotd (TribeId, Message) VALUES (?, ?);" << tribe->TribeIDField()() << ArkApi::Tools::
					Utf8Encode(**message);
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
	db << "CREATE UNIQUE INDEX IF NOT EXISTS IdxTribeId ON TribeMotd (TribeId);";

	ArkApi::GetHooks().SetHook("AShooterPlayerController.ServerReadMessageOFTheDay_Implementation",
	                           &Hook_AShooterPlayerController_ServerReadMessageOFTheDay_Impl,
	                           &AShooterPlayerController_ServerReadMessageOFTheDay_Impl_original);

	ArkApi::GetCommands().AddChatCommand("/SetMotd", &SetMotd);
}

void Unload()
{
	ArkApi::GetHooks().DisableHook("AShooterPlayerController.ServerReadMessageOFTheDay_Implementation",
	                               &Hook_AShooterPlayerController_ServerReadMessageOFTheDay_Impl);

	ArkApi::GetCommands().RemoveChatCommand("/SetMotd");
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
