#pragma once

#include "Database/IDatabase.h"

#include "json.hpp"

namespace ArkShop
{
	inline nlohmann::json config;
	inline std::unique_ptr<IDatabase> database;

	FString GetText(const std::string& str);
	bool IsStoreEnabled(AShooterPlayerController* player_controller);
	void ToogleStore(bool enabled, const FString& reason = "");
} // namespace ArkShop
