#pragma once

#include <API/UE/Containers/FString.h>

#include "json.hpp"

namespace ArkShop
{
	extern nlohmann::json config;

	FString GetText(const std::string& str); 
	bool IsStoreEnabled(AShooterPlayerController* player_controller);
	void ToogleStore(bool Enabled, const FString& Reason = "");
} // namespace ArkShop
