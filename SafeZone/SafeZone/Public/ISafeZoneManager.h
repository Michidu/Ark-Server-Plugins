#pragma once

#include "SafeZone.h"

namespace SafeZones
{
	class ZONE_API ISafeZoneManager
	{
	public:
		virtual ~ISafeZoneManager() = default;

		virtual bool CreateSafeZone(const std::shared_ptr<SafeZone>& safe_zone) = 0;
		virtual bool RemoveSafeZone(const FString& name) = 0;

		virtual bool CanBuild(APlayerController* player, const FVector& location, bool notification = false) = 0;
		virtual bool CheckActorAction(AActor* actor, int type) = 0;
		virtual std::shared_ptr<SafeZone> FindZoneByName(const FString& name) = 0;
	};

	ZONE_API ISafeZoneManager& APIENTRY GetSafeZoneManager();
}
