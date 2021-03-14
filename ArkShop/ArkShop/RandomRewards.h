#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "API/ARK/Ark.h"
#include "Private/ArkShop.h"

namespace ArkShop::Random {
	
	
	void generateAndGiveRewards(AShooterPlayerController* sender, const FString& lootbox);
}