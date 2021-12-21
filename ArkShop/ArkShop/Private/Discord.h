#pragma once
#include "ArkShop.h"

template <typename T, typename... Args>
void PostToDiscord(T* msg, Args&&... args)
{
	static_cast<AShooterGameState*>(ArkApi::GetApiUtils().GetWorld()->GameStateField())->HTTPPostRequest(ArkShop::discord_webhook_url, FString::Format(msg, std::forward<Args>(args)...));
}