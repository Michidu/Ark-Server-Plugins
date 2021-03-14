#include "API/ARK/Ark.h"
#include "RandomRewards.h"
#include <random>
#include <iostream>
#include "Public/Points.h"
#include "Private/ArkShop.h"

namespace ArkShop::Random {


	void generateAndGiveRewards(AShooterPlayerController* sender, const FString& lootbox)
	{
		// Decrease the uses in Database


		// Items
		uint64 itemRolls = ArkShop::config["Cajas"][lootbox.ToString()]["Items"]["Count"];
		for (int i = 0; i < itemRolls; i++) {

			const nlohmann::json itemArray = ArkShop::config["Cajas"][lootbox.ToString()];
			auto itemIter = itemArray.find("Items");
			const nlohmann::basic_json<> item = itemIter.value();
			auto possibleItems = item.value("PossibleItems", nlohmann::json::array());


			// Selecting Random Item from Itemlist in config.json

			int size = possibleItems.size();
			std::random_device rd;
			std::mt19937 eng(rd());
			std::uniform_int_distribution<> rand(0, size - 1);

			nlohmann::json selectedItem = possibleItems.at(rand(eng));

			// Get the Data from the random item

			int Amount = selectedItem["Amount"];
			int MinQuality = selectedItem["MinQuality"];
			int MaxQuality = selectedItem["MaxQuality"];
			int BlueprintChance = selectedItem["BlueprintChance"];

			std::string BlueprintPath = selectedItem["BlueprintPath"];
			FString FBlueprintPath(BlueprintPath.c_str());

			bool isBlueprint = false;
			std::uniform_int_distribution<> blueprint(1, 100);
			if (blueprint(eng) <= BlueprintChance) isBlueprint = true;
			std::uniform_int_distribution<> quality(MinQuality, MaxQuality);
			int RandomQuality = quality(eng);

			UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(sender->CheatManagerField());
			if (cheatManager) cheatManager->GiveItemToPlayer((int)sender->LinkedPlayerIDField(), &FBlueprintPath, Amount, (float)RandomQuality, isBlueprint);


		}

		// Dinos
		int dinoRolls = ArkShop::config["Cajas"][lootbox.ToString()]["Dinos"]["Count"];
		for (int i = 0; i < dinoRolls; i++) {
			const nlohmann::json itemArray = ArkShop::config["Cajas"][lootbox.ToString()];
			auto itemIter = itemArray.find("Dinos");
			const nlohmann::basic_json<> item = itemIter.value();
			auto possibleItems = item.value("PossibleDinos", nlohmann::json::array());


			// Selecting Random Dinos from DinoList in config.json

			int size = possibleItems.size();
			std::random_device rd;
			std::mt19937 eng(rd());
			std::uniform_int_distribution<> rand(0, size - 1);

			nlohmann::json selectedItem = possibleItems.at(rand(eng));

			// Get the Data from the random dino

			int levelMax = selectedItem["LevelMax"];
			int levelMin = selectedItem["LevelMin"];
			std::string BlueprintPath = selectedItem["BlueprintPath"];
			FString FBlueprintPath(BlueprintPath.c_str());
			std::uniform_int_distribution<> quality(levelMin, levelMax);
			int RandomQuality = quality(eng);
			ArkApi::GetApiUtils().SpawnDino(sender, FBlueprintPath, nullptr, (float)RandomQuality, true, false);
		}

		// Resources
		int resourcesRolls = ArkShop::config["Cajas"][lootbox.ToString()]["Resources"]["Count"];
		for (int i = 0; i < resourcesRolls; i++) {
			const nlohmann::json itemArray = ArkShop::config["Cajas"][lootbox.ToString()];
			auto itemIter = itemArray.find("Resources");
			const nlohmann::basic_json<> item = itemIter.value();
			auto possibleItems = item.value("PossibleResources", nlohmann::json::array());


			// Selecting Random Resources from ResourceList in config.json

			int size = possibleItems.size();
			std::random_device rd;
			std::mt19937 eng(rd());
			std::uniform_int_distribution<> rand(0, size - 1);

			nlohmann::json selectedItem = possibleItems.at(rand(eng));

			// Get the Data from the random resource

			int Amount = selectedItem["Amount"];
			std::string BlueprintPath = selectedItem["BlueprintPath"];
			FString FBlueprintPath(BlueprintPath.c_str());

			UShooterCheatManager* cheatManager = static_cast<UShooterCheatManager*>(sender->CheatManagerField());
			if (cheatManager) cheatManager->GiveItemToPlayer((int)sender->LinkedPlayerIDField(), &FBlueprintPath, Amount, (float)1, false);
		}

		// RCONCommand
		int RCONRolls = ArkShop::config["Cajas"][lootbox.ToString()]["RCONCommands"]["Count"];
		for (int i = 0; i < RCONRolls; i++) {
			const nlohmann::json itemArray = ArkShop::config["Cajas"][lootbox.ToString()];
			auto itemIter = itemArray.find("RCONCommands");
			const nlohmann::basic_json<> item = itemIter.value();
			auto possibleCommands = item.value("PossibleCommands", nlohmann::json::array());

			int size = possibleCommands.size();
			std::random_device rd;
			std::mt19937 eng(rd());
			std::uniform_int_distribution<> rand(0, size - 1);

			nlohmann::json selectedCommand = possibleCommands.at(rand(eng));

			std::string command = selectedCommand["Command"];
			int MinPointsShop = selectedCommand["MinPointShop"];
			int MaxPointsShop = selectedCommand["MaxPointShop"];

			std::uniform_int_distribution<> points(MinPointsShop, MaxPointsShop);

			int shoppoints = points(eng);
			uint64 steamId = ArkApi::GetApiUtils().GetSteamIdFromController(sender);

			// Use the ArkShop API to add points
			ArkShop::Points::AddPoints(shoppoints, steamId);

		}
	}
}