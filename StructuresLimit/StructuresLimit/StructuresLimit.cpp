#include "json.hpp"

#include <API/ARK/Ark.h>

#include <fstream>

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(AShooterGameMode_InitGame, void, AShooterGameMode*, FString*, FString*, FString*);
//DECLARE_HOOK(APrimalStructure_Die, bool, APrimalStructure*, float, FDamageEvent*, AController*, AActor*);
DECLARE_HOOK(APrimalStructure_IsAllowedToBuild, int, APrimalStructure*, APlayerController*, FVector, FRotator,
	FPlacementData*, bool, FRotator, bool);

std::map<FString, std::unordered_map<int, int>> all_structures;
std::unordered_map<int, int> structures_count;

nlohmann::json config;

FString GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Messages"].value(str, "No message")).c_str());
}

FString GetBlueprint(UObjectBase* object)
{
	if (object != nullptr && object->ClassField() != nullptr)
	{
		FString path_name;
		object->ClassField()->GetDefaultObject(true)->GetFullName(&path_name, nullptr);

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

void UpdateStructuresCount()
{
	all_structures.clear();
	structures_count.clear();

	auto& structures_map = config["Structures"];
	for (auto iter = structures_map.begin(); iter != structures_map.end(); ++iter)
	{
		const std::string name_str = iter.key();

		all_structures[name_str.c_str()] = {};
	}

	const auto& actors = ArkApi::GetApiUtils().GetWorld()->PersistentLevelField()->GetActorsField();
	for (AActor* actor : actors)
	{
		if (actor != nullptr && actor->IsA(APrimalStructure::GetPrivateStaticClass()))
		{
			const int team_id = actor->TargetingTeamField();
			if (team_id != 0)
			{
				++structures_count[team_id];

				auto* structure = static_cast<APrimalStructure*>(actor);

				const FString path_name = GetBlueprint(structure);

				if (const auto& iter = all_structures.find(path_name);
					iter != all_structures.end())
				{
					int& count = iter->second[team_id];
					++count;
				}
			}
		}
	}
}

void Hook_AShooterGameMode_InitGame(AShooterGameMode* a_shooter_game_mode, FString* map_name, FString* options,
                                    FString* error_message)
{
	AShooterGameMode_InitGame_original(a_shooter_game_mode, map_name, options, error_message);

	UpdateStructuresCount();
}

/*bool Hook_APrimalStructure_Die(APrimalStructure* _this, float KillingDamage, FDamageEvent* DamageEvent,
                               AController* Killer, AActor* DamageCauser)
{
	const int team_id = _this->TargetingTeamField();

	int& struct_count = structures_count[team_id];
	if (struct_count > 0)
		--struct_count;

	const FString path_name = GetBlueprint(_this);

	if (const auto& iter = all_structures.find(path_name);
		iter != all_structures.end())
	{
		int& count = iter->second[team_id];
		if (count > 0)
			--count;
	}

	return APrimalStructure_Die_original(_this, KillingDamage, DamageEvent, Killer, DamageCauser);
}*/

int Hook_APrimalStructure_IsAllowedToBuild(APrimalStructure* _this, APlayerController* PC, FVector AtLocation,
                                           FRotator AtRotation, FPlacementData* OutPlacementData,
                                           bool bDontAdjustForMaxRange, FRotator PlayerViewRotation,
                                           bool bFinalPlacement)
{
	if (bFinalPlacement && PC != nullptr)
	{
		const int team_id = _this->TargetingTeamField();

		int& struct_count = structures_count[team_id];
		if (struct_count >= config["General"].value("MaxAmount", 0))
		{
			const float display_scale = config["General"]["NotificationScale"];
			const float display_time = config["General"]["NotificationDisplayTime"];

			ArkApi::GetApiUtils().SendNotification(static_cast<AShooterPlayerController*>(PC), FLinearColor(1, 0, 0),
			                                       display_scale, display_time, nullptr, *GetText("ReachLimit"));

			return 0;
		}

		const FString path_name = GetBlueprint(_this);

		if (const auto& iter = all_structures.find(path_name);
			iter != all_structures.end())
		{
			int& count = iter->second[team_id];
			if (count >= config["Structures"].value(path_name.ToString(), nlohmann::json::object())
			                                 .value("Count", 9999))
			{
				const float display_scale = config["General"]["NotificationScale"];
				const float display_time = config["General"]["NotificationDisplayTime"];

				ArkApi::GetApiUtils().SendNotification(static_cast<AShooterPlayerController*>(PC),
				                                       FLinearColor(1, 0, 0),
				                                       display_scale, display_time, nullptr, *GetText("ReachLimit"));

				return 0;
			}

			++count;
		}

		++struct_count;
	}

	return APrimalStructure_IsAllowedToBuild_original(_this, PC, AtLocation, AtRotation, OutPlacementData,
	                                                  bDontAdjustForMaxRange, PlayerViewRotation, bFinalPlacement);
}

std::chrono::system_clock::time_point next_time;

void Update()
{
	const auto now = std::chrono::system_clock::now();
	if (now >= next_time)
	{
		next_time = now + std::chrono::seconds(config["General"].value("CheckInterval", 30));

		UpdateStructuresCount();
	}
}

void ShowStructuresLimits(AShooterPlayerController* player_controller, FString* /*unused*/, bool /*unused*/)
{
	if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
	{
		return;
	}

	try
	{
		FString text;

		const int team_id = player_controller->TargetingTeamField();

		const int struct_count = structures_count[team_id];
		const int max_struct_count = config["General"]["MaxAmount"];

		for (const auto& data : all_structures)
		{
			const FString bp = data.first;

			const int count = data.second.find(team_id) == data.second.end()
				                  ? 0
				                  : data.second.at(team_id);

			const auto str_config = config["Structures"].value(bp.ToString(), nlohmann::json::object());

			const int max_count = str_config.value("Count", 9999);
			const std::string name = str_config.value("Name", "");

			text += FString::Format(*GetText("ShowLimitMsg"), ArkApi::Tools::Utf8Decode(name).c_str(), count,
			                        max_count);
		}

		text += FString::Format(*GetText("TotalLimitMsg"), struct_count, max_struct_count);

		ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
		                                      *text);
	}
	catch (const std::exception& exception)
	{
		Log::GetLog()->warn(exception.what());
	}
}

void GetBpPathCmd(APlayerController* player, FString* /*unused*/, bool /*unused*/)
{
	auto* player_controller = static_cast<AShooterPlayerController*>(player);

	if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
	{
		return;
	}

	AActor* actor = player_controller->GetPlayerCharacter()->GetAimedActor(
		ECC_GameTraceChannel2, nullptr, 0.0, 0.0, nullptr, nullptr,
		false, false);

	if (actor != nullptr && actor->IsA(APrimalStructure::GetPrivateStaticClass()))
	{
		const FString path_name = GetBlueprint(actor);

		ArkApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
		                                      *path_name);

		Log::GetLog()->info(path_name.ToString());
	}
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/StructuresLimit/config.json";
	std::ifstream file{config_path};
	if (!file.is_open())
	{
		throw std::runtime_error("Can't open config.json");
	}

	file >> config;

	file.close();
}

void Load()
{
	Log::Get().Init("StructuresLimit");

	ReadConfig();

	next_time = std::chrono::system_clock::now();

	auto& hooks = ArkApi::GetHooks();

	hooks.SetHook("AShooterGameMode.InitGame", &Hook_AShooterGameMode_InitGame,
	              &AShooterGameMode_InitGame_original);
	//hooks.SetHook("APrimalStructure.Die", &Hook_APrimalStructure_Die,
	//&APrimalStructure_Die_original);
	hooks.SetHook("APrimalStructure.IsAllowedToBuild", &Hook_APrimalStructure_IsAllowedToBuild,
	              &APrimalStructure_IsAllowedToBuild_original);

	ArkApi::GetCommands().AddOnTimerCallback("StructuresLimit", &Update);

	ArkApi::GetCommands().AddChatCommand(*GetText("ShowLimitsCmd"), &ShowStructuresLimits);

	ArkApi::GetCommands().AddConsoleCommand("GetBp", &GetBpPathCmd);
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
