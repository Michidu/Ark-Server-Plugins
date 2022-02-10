#include "HomeSystem.h"
#include "Helper.h"

#include <ArkPermissions.h>
#include <Points.h>

namespace HomeSystem
{
	DECLARE_HOOK(AShooterCharacter_Die, bool, AShooterCharacter*, float, FDamageEvent*, AController*, AActor*);
	DECLARE_HOOK(APrimalCharacter_TakeDamage, float, APrimalCharacter*, float, FDamageEvent*, AController*, AActor*);

	std::vector<uint64> teleporting_players;

	nlohmann::basic_json<> GetPlayerHomesConfig(uint64 steam_id)
	{
		auto& db = ArkHome::GetDB();

		std::string homes_config = "{}";

		try
		{
			SQLite::Statement query(db, "SELECT Homes FROM Players WHERE SteamId = ?;");
			query.bind(1, static_cast<int64>(steam_id));
			query.executeStep();

			homes_config = query.getColumn(0).getString();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return nlohmann::json::parse(homes_config);
	}

	bool SaveConfig(const std::string& dump, uint64 steam_id)
	{
		auto& db = ArkHome::GetDB();

		try
		{
			SQLite::Statement query(db, "UPDATE Players SET Homes = ? WHERE SteamId = ?;");
			query.bind(1, dump);
			query.bind(2, static_cast<int64>(steam_id));
			query.exec();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return true;
	}

	void AddPlayer(uint64 steam_id)
	{
		auto& db = ArkHome::GetDB();

		try
		{
			SQLite::Statement query(db, "INSERT INTO Players (SteamId) VALUES (?);");
			query.bind(1, static_cast<int64>(steam_id));
			query.exec();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	void SetTpHomeCooldown(uint64 steam_id, int64 cooldown)
	{
		auto& db = ArkHome::GetDB();

		try
		{
			SQLite::Statement query(db, "UPDATE Players SET TpHomeCooldown = ? WHERE SteamId = ?;");
			query.bind(1, cooldown);
			query.bind(2, static_cast<int64>(steam_id));
			query.exec();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	void SetAddHomeCooldown(uint64 steam_id, int64 cooldown)
	{
		auto& db = ArkHome::GetDB();

		try
		{
			SQLite::Statement query(db, "UPDATE Players SET AddHomeCooldown = ? WHERE SteamId = ?;");
			query.bind(1, cooldown);
			query.bind(2, static_cast<int64>(steam_id));
			query.exec();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}
	}

	int64 GetTpHomeCooldown(uint64 steam_id)
	{
		auto& db = ArkHome::GetDB();

		int64 cooldown = 0;

		try
		{
			SQLite::Statement query(db, "SELECT TpHomeCooldown FROM Players WHERE SteamId = ?;");
			query.bind(1, static_cast<int64>(steam_id));
			query.executeStep();

			cooldown = query.getColumn(0).getInt64();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return cooldown;
	}

	int64 GetAddHomeCooldown(uint64 steam_id)
	{
		auto& db = ArkHome::GetDB();

		int64 cooldown = 0;

		try
		{
			SQLite::Statement query(db, "SELECT AddHomeCooldown FROM Players WHERE SteamId = ?;");
			query.bind(1, static_cast<int64>(steam_id));
			query.executeStep();

			cooldown = query.getColumn(0).getInt64();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
		}

		return cooldown;
	}

	int GetNearbyStructuresCount(AShooterPlayerController* player_controller, int radius)
	{
		UWorld* world = ArkApi::GetApiUtils().GetWorld();

		TArray<AActor*> new_actors;

		TArray<AActor*> actors_ignore;
		TArray<TEnumAsByte<enum EObjectTypeQuery>> types;

		UKismetSystemLibrary::SphereOverlapActors_NEW(world, ArkApi::IApiUtils::GetPosition(player_controller),
		                                              static_cast<float>(radius), &types,
		                                              APrimalStructure::GetPrivateStaticClass(), &actors_ignore,
		                                              &new_actors);

		int count = 0;

		for (const auto& actor : new_actors)
		{
			APrimalStructure* structure = static_cast<APrimalStructure*>(actor);

			if (structure->TargetingTeamField() == player_controller->TargetingTeamField())
				++count;
		}

		return count;
	}

	bool IsEnemyStructureNear(AShooterPlayerController* player_controller, int radius)
	{
		UWorld* world = ArkApi::GetApiUtils().GetWorld();

		TArray<AActor*> new_actors;

		TArray<AActor*> actors_ignore;
		TArray<TEnumAsByte<enum EObjectTypeQuery>> types;

		UKismetSystemLibrary::SphereOverlapActors_NEW(world, ArkApi::IApiUtils::GetPosition(player_controller),
		                                              static_cast<float>(radius), &types,
		                                              APrimalStructure::GetPrivateStaticClass(), &actors_ignore,
		                                              &new_actors);

		for (const auto& actor : new_actors)
		{
			APrimalStructure* structure = static_cast<APrimalStructure*>(actor);

			if (structure->TargetingTeamField() != 0 && structure->TargetingTeamField() != player_controller->TargetingTeamField())
				return true;
		}

		return false;
	}

	bool IsEnemyStructureNear(const FVector& pos, int team_id, int radius)
	{
		UWorld* world = ArkApi::GetApiUtils().GetWorld();

		TArray<AActor*> new_actors;

		TArray<AActor*> actors_ignore;
		TArray<TEnumAsByte<enum EObjectTypeQuery>> types;

		UKismetSystemLibrary::SphereOverlapActors_NEW(world, pos, static_cast<float>(radius), &types,
		                                              APrimalStructure::GetPrivateStaticClass(), &actors_ignore,
		                                              &new_actors);

		for (const auto& actor : new_actors)
		{
			APrimalStructure* structure = static_cast<APrimalStructure*>(actor);

			if (structure->TargetingTeamField() != 0 && structure->TargetingTeamField() != team_id)
				return true;
		}

		return false;
	}

	void AddHome(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
	{
		if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
			return;

		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			std::string name = parsed[1].ToString();

			const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);
			if (!Helper::IsPlayerExists(steam_id))
				AddPlayer(steam_id);

			const bool use_permission = ArkHome::config["General"].value("UsePermissions", false);
			if (ArkApi::Tools::IsPluginLoaded("Permissions")) {
				if (use_permission && !Permissions::IsPlayerHasPermission(steam_id, "ArkHomes.Teleport"))
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
						*ArkHome::GetText("NoPermissions"));
					return;
				}
			}

			auto player_home_json = GetPlayerHomesConfig(steam_id);

			const auto home_json_iter = player_home_json.find(name);
			if (home_json_iter != player_home_json.end())
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("HomeNameExists"));
				return;
			}

			const auto homes_count = player_home_json.size();
			const int max_homes = ArkHome::config["General"].value("MaxHomes", 2);
			if (homes_count >= max_homes)
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("NoMoreHomes"), max_homes);
				return;
			}

			const auto cooldown = std::chrono::system_clock::from_time_t(GetAddHomeCooldown(steam_id));
			if (std::chrono::system_clock::now() < cooldown)
			{
				auto time_left = std::chrono::duration_cast<std::chrono::seconds>(
					cooldown - std::chrono::system_clock::now()).count();

				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("HomeCooldown"), time_left);
				return;
			}

			const int min_structures = ArkHome::config["General"].value("MinStructures", 3);
			const int radius = ArkHome::config["General"].value("Radius", 5000);

			if (const int count = GetNearbyStructuresCount(player_controller, radius);
				count < min_structures)
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("NotEnoughStructures"), count, min_structures);
				return;
			}

			FVector pos = ArkApi::IApiUtils::GetPosition(player_controller);

			player_home_json[name] = {pos.X, pos.Y, pos.Z};

			std::string dump;

			try
			{
				dump = player_home_json.dump();
			}
			catch (const std::exception& exception)
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"), "Unexpected error");
				Log::GetLog()->error("({} {}) Unexpected error {}", __FILE__, __FUNCTION__, exception.what());
				return;
			}

			if (SaveConfig(dump, steam_id))
			{
				const int add_cooldown = ArkHome::config["General"].value("AddHomeCooldown", 1);
				const auto new_cooldown = std::chrono::system_clock::to_time_t(
					std::chrono::system_clock::now() + std::chrono::minutes(add_cooldown));

				SetAddHomeCooldown(steam_id, new_cooldown);

				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("AddedHome"));
			}
			else
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("FailedAddHome"));
			}
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
			                                      *ArkHome::GetText("AddHomeUsage"));
		}
	}

	void RemoveHome(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
	{
		if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
			return;

		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			std::string name = parsed[1].ToString();

			const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);
			if (!Helper::IsPlayerExists(steam_id))
				AddPlayer(steam_id);

			auto player_home_json = GetPlayerHomesConfig(steam_id);

			const auto home_json_iter = player_home_json.find(name);
			if (home_json_iter == player_home_json.end())
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("HomeNameNotExists"));
				return;
			}

			player_home_json.erase(home_json_iter);

			if (SaveConfig(player_home_json.dump(), steam_id))
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("RemovedHome"));
			}
			else
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("FailedRemoveHome"));
			}
		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
			                                      *ArkHome::GetText("RemoveHomeUsage"));
		}
	}

	void ListHomes(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
	{
		if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
			return;

		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);
		if (!Helper::IsPlayerExists(steam_id))
			AddPlayer(steam_id);

		const bool use_permission = ArkHome::config["General"].value("UsePermissions", false);
		if (ArkApi::Tools::IsPluginLoaded("Permissions")) {
			if (use_permission && !Permissions::IsPlayerHasPermission(steam_id, "ArkHomes.Teleport"))
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
					*ArkHome::GetText("NoPermissions"));
				return;
			}
		}

		FString store_str = "";

		auto player_home_json = GetPlayerHomesConfig(steam_id);
		for (auto iter = player_home_json.begin(); iter != player_home_json.end(); ++iter)
		{
			auto name = iter.key();

			store_str += FString::Format("{} ", name);
		}

		if (store_str.IsEmpty()) {
			ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"), *ArkHome::GetText("NoHomesSet"));
		}
		else {
			ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"), *ArkHome::GetText("ListOfHomes"),  *store_str);
		}

	}

	void DoTp(AShooterPlayerController* player_controller, const FVector& pos)
	{
		if (!player_controller || ArkApi::IApiUtils::IsPlayerDead(player_controller))
			return;

		const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);

		const auto player_iter = find(teleporting_players.begin(), teleporting_players.end(), steam_id);
		if (player_iter == teleporting_players.end())
			return;

		teleporting_players.erase(remove(teleporting_players.begin(), teleporting_players.end(), *player_iter),
		                          teleporting_players.end());

		player_controller->SetPlayerPos(pos[0], pos[1], pos[2]);

		Helper::DisableInput(player_controller, false);

		const int tp_cooldown = ArkHome::config["General"].value("TpHomeCooldown", 1);
		const auto cooldown = std::chrono::system_clock::to_time_t(
			std::chrono::system_clock::now() + std::chrono::minutes(tp_cooldown));

		SetTpHomeCooldown(steam_id, cooldown);

		ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
		                                      *ArkHome::GetText("HomeTeleported"));


		const bool use_points = ArkHome::config["General"].value("UseArkShop", false);
		const int cost_hometp = ArkHome::config["General"].value("CostPerHomeTeleport", 20);
		if (ArkApi::Tools::IsPluginLoaded("ArkShop")) {
			if (use_points) {
				ArkShop::Points::SpendPoints(cost_hometp, steam_id);
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
					*ArkHome::GetText("ChargedPoints"), cost_hometp);
			}
		}

	}

	void TpHome(AShooterPlayerController* player_controller, FString* message, EChatSendMode::Type)
	{
		if (ArkApi::IApiUtils::IsPlayerDead(player_controller))
			return;

		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			std::string name = parsed[1].ToString();

			const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player_controller);
			if (!Helper::IsPlayerExists(steam_id))
				AddPlayer(steam_id);

			const auto player_iter = find(teleporting_players.begin(), teleporting_players.end(), steam_id);
			if (player_iter != teleporting_players.end())
				return;

			const bool use_permission = ArkHome::config["General"].value("UsePermissions", false);
			if(ArkApi::Tools::IsPluginLoaded("Permissions")) {
				if (use_permission && !Permissions::IsPlayerHasPermission(steam_id, "ArkHomes.Teleport"))
				{
					ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
						*ArkHome::GetText("NoPermissions"));
					return;
				}
			}

			const bool use_points = ArkHome::config["General"].value("UseArkShop", false);
			const int cost_hometp = ArkHome::config["General"].value("CostPerHomeTeleport", 20);
			if (ArkApi::Tools::IsPluginLoaded("ArkShop")) {
				const int player_points = ArkShop::Points::GetPoints(steam_id);
				if (use_points && player_points < cost_hometp) {
					ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
						*ArkHome::GetText("NoEnoughPoints"), cost_hometp);
					return;
				}
			}

			const bool can_tp_with_dino = ArkHome::config["General"].value("CanTpWithDino", true);
			if (!can_tp_with_dino && ArkApi::IApiUtils::IsRidingDino(player_controller))
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("CantRideDino"));
				return;
			}

			auto player_home_json = GetPlayerHomesConfig(steam_id);

			const auto home_json_iter = player_home_json.find(name);
			if (home_json_iter == player_home_json.end())
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("HomeNameNotExists"));
				return;
			}

			const auto cooldown = std::chrono::system_clock::from_time_t(GetTpHomeCooldown(steam_id));
			if (std::chrono::system_clock::now() < cooldown)
			{
				auto time_left = std::chrono::duration_cast<std::chrono::seconds>(
					cooldown - std::chrono::system_clock::now()).count();

				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("HomeCooldown"), time_left);
				return;
			}

			const int enemy_min_distance = ArkHome::config["General"].value("EnemyStructureMinDistance", 10000);
			if (IsEnemyStructureNear(player_controller, enemy_min_distance))
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("CantTpNearEnemy"));
				return;
			}

			auto pos = player_home_json[name];
			FVector pos_vec = FVector(pos[0], pos[1], pos[2]);

			if (IsEnemyStructureNear(pos_vec, player_controller->TargetingTeamField(), enemy_min_distance))
			{
				ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
				                                      *ArkHome::GetText("CantTpNearEnemy"));
				return;
			}

			const int delay = ArkHome::config["General"].value("HomeTpDelay", 20);

			Helper::Timer(delay * 1000, true, steam_id, &DoTp, player_controller, pos_vec);

			teleporting_players.push_back(steam_id);

			Helper::DisableInput(player_controller, true);

			ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"), *ArkHome::GetText("HomeTeleportStart"), delay);
			//ArkApi::GetApiUtils().SendNotification(player_controller, FColorList::Orange, 1.5, delay, nullptr, *ArkHome::GetText("HomeTeleportStart"), delay);

		}
		else
		{
			ArkApi::GetApiUtils().SendChatMessage(player_controller, ArkHome::GetText("Sender"),
			                                      *ArkHome::GetText("HomeUsage"));
		}
	}

	bool Hook_AShooterCharacter_Die(AShooterCharacter* _this, float KillingDamage, FDamageEvent* DamageEvent,
	                                AController* Killer, AActor* DamageCauser)
	{
		AShooterPlayerController* player = ArkApi::GetApiUtils().FindControllerFromCharacter(_this);
		if (player)
		{
			const uint64 steam_id = ArkApi::IApiUtils::GetSteamIdFromController(player);

			const auto player_iter = find(teleporting_players.begin(), teleporting_players.end(), steam_id);
			if (player_iter != teleporting_players.end())
			{
				teleporting_players.erase(remove(teleporting_players.begin(), teleporting_players.end(), *player_iter),
				                          teleporting_players.end());
			}
		}

		return AShooterCharacter_Die_original(_this, KillingDamage, DamageEvent, Killer, DamageCauser);
	}

	float Hook_APrimalCharacter_TakeDamage(APrimalCharacter* _this, float Damage, FDamageEvent* DamageEvent, AController* EventInstigator, AActor* DamageCauser)
	{
		if (_this->IsA(AShooterCharacter::GetPrivateStaticClass()))
		{
			AShooterPlayerController* player = ArkApi::GetApiUtils().FindControllerFromCharacter(
				static_cast<AShooterCharacter*>(_this));
			if (player)
			{
				const uint64 steam_id = ArkApi::GetApiUtils().GetSteamIdFromController(player);

				const auto player_iter = find(teleporting_players.begin(), teleporting_players.end(), steam_id);
				if (player_iter != teleporting_players.end())
				{
					teleporting_players.erase(remove(teleporting_players.begin(), teleporting_players.end(), *player_iter),
					                          teleporting_players.end());

					Helper::Timer::threads.erase(steam_id);

					Helper::DisableInput(player, false);

					ArkApi::GetApiUtils().SendChatMessage(player, ArkHome::GetText("Sender"),
					                                      *ArkHome::GetText("HomeInterrupt"));
				}
			}
		}

		return APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
	}

	void Init()
	{
		auto& commands = ArkApi::GetCommands();

		commands.AddChatCommand(ArkHome::GetText("AddHomeCmd"), &AddHome);
		commands.AddChatCommand(ArkHome::GetText("RemoveHomeCmd"), &RemoveHome);
		commands.AddChatCommand(ArkHome::GetText("ListHomesCmd"), &ListHomes);
		commands.AddChatCommand(ArkHome::GetText("HomeCmd"), &TpHome);

		ArkApi::GetHooks().SetHook("AShooterCharacter.Die", &Hook_AShooterCharacter_Die, &AShooterCharacter_Die_original);
		ArkApi::GetHooks().SetHook("APrimalCharacter.TakeDamage", &Hook_APrimalCharacter_TakeDamage,
		                           &APrimalCharacter_TakeDamage_original);
	}

	void RemoveHooks() {

		ArkApi::GetHooks().DisableHook("APrimalCharacter.TakeDamage", &Hook_APrimalCharacter_TakeDamage);
		ArkApi::GetHooks().DisableHook("AShooterCharacter.Die", &Hook_AShooterCharacter_Die);

		ArkApi::GetCommands().RemoveChatCommand(ArkHome::GetText("AddHomeCmd"));
		ArkApi::GetCommands().RemoveChatCommand(ArkHome::GetText("RemoveHomeCmd"));
		ArkApi::GetCommands().RemoveChatCommand(ArkHome::GetText("ListHomesCmd"));
		ArkApi::GetCommands().RemoveChatCommand(ArkHome::GetText("HomeCmd"));

	}

}
