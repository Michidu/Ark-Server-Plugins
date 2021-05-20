#pragma once

#include "Database/IDatabase.h"

#include "json.hpp"

namespace ArkShop
{
	inline nlohmann::json config;
	inline std::unique_ptr<IDatabase> database;

	FCustomItemData GetDinoCustomItemData(APrimalDinoCharacter* dino, UPrimalItem* saddle);
	bool GiveDino(AShooterPlayerController* player_controller, int level, bool neutered, std::string blueprint, std::string saddleblueprint);
	FString GetText(const std::string& str);
	bool IsStoreEnabled(AShooterPlayerController* player_controller);
	void ToogleStore(bool enabled, const FString& reason = "");
} // namespace ArkShop