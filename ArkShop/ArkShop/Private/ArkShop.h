#pragma once

#include "Database/IDatabase.h"

#include "json.hpp"

namespace ArkShop
{
	inline nlohmann::json config;
	inline std::unique_ptr<IDatabase> database;
	inline UClass* NoglinBuffClass = nullptr;

	float getStatValue(float StatModifier, float InitialValueConstant, float RandomizerRangeMultiplier, float StateModifierScale, bool bDisplayAsPercent);
	void ApplyItemStats(TArray<UPrimalItem*> items, int armor, int durability, int damage);
	FString GetBlueprintShort(UObjectBase* object);
	FCustomItemData GetDinoCustomItemData(APrimalDinoCharacter* dino, UPrimalItem* saddle, bool Modded);
	bool GiveDino(AShooterPlayerController* player_controller, int level, bool neutered, std::string gender, std::string blueprint, std::string saddleblueprint);
	FString GetText(const std::string& str);
	bool IsStoreEnabled(AShooterPlayerController* player_controller);
	void ToogleStore(bool enabled, const FString& reason = "");

	//Discord Functions
	inline bool discord_enabled;
	inline std::string discord_sender_name;
	inline FString discord_webhook_url;
} // namespace ArkShop