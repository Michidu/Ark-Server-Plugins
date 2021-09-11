#pragma once

#include "../Public/ISafeZoneManager.h"
#include "Structs.h"

/*struct TClassCompiledInDefer_ATriggerSphere
{
	static UField* Register() { return NativeCall<UField *>(nullptr, "TClassCompiledInDefer<ATriggerSphere>.Register"); }
};

struct ATriggerBase : AActor
{
	FieldValue<TSubobjectPtr<UShapeComponent>> CollisionComponentField()
	{
		return {this, "ATriggerBase.CollisionComponent"};
	}
};*/

namespace SafeZones
{
	struct PlayerPos
	{
		SafeZone* zone{};
		bool in_zone{};
		FVector inzone_pos;
		FVector outzone_pos;
	};

	class SafeZoneManager : public ISafeZoneManager
	{
	public:
		static SafeZoneManager& Get();

		SafeZoneManager(const SafeZoneManager&) = delete;
		SafeZoneManager(SafeZoneManager&&) = delete;
		SafeZoneManager& operator=(const SafeZoneManager&) = delete;
		SafeZoneManager& operator=(SafeZoneManager&&) = delete;

		bool CreateSafeZone(const std::shared_ptr<SafeZone>& safe_zone) override;
		bool RemoveSafeZone(const FString& name) override;

		bool CanBuild(APlayerController* player, const FVector& location, bool notification) override;
		bool CheckActorAction(AActor* actor, int type) override;

		void ReadSafeZones();

		std::shared_ptr<SafeZone> FindZoneByName(const FString& name) override;

		TArray<std::shared_ptr<SafeZone>>& GetAllSafeZones();
		TArray<FString>& GetDefaultSafeZones();

		ATriggerSphere* SpawnSphere(FVector& location, int radius);

		std::shared_ptr<SafeZone> CheckActorOverlap(AActor* _this);

		void ClearAllTriggerSpheres();

	private:
		SafeZoneManager() = default;
		~SafeZoneManager() = default;

		TArray<std::shared_ptr<SafeZone>> all_safezones_;

		/**
		 * \brief List of safe zone names created from the default config file, it's needed for reloading
		 */
		TArray<FString> default_safezones_;
	};
}
