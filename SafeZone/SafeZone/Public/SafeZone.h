#pragma once

#include <API/ARK/Ark.h>
#include "..\Private\Structs.h"

#ifdef ZONE_EXPORTS
#define ZONE_API __declspec(dllexport)
#else
#define ZONE_API __declspec(dllimport)
#endif

namespace SafeZones
{
	struct SafeZone
	{
		SafeZone(FString name, const FVector& position, int radius, bool prevent_pvp, bool prevent_structure_damage,
			bool prevent_building, bool kill_wild_dinos, bool prevent_leaving, bool prevent_entering, bool enable_events,
			bool screen_notifications, bool chat_notifications, const FLinearColor& success_color,
			const FLinearColor& fail_color, std::vector<FString> messages, bool cryopod_dinos, bool show_bubble, std::vector<float> bubble_colors,
			bool bEnableTeleport, const std::vector<float>& teleport_destination, const bool onlyKillAggresiveDinos, const bool prevent_friendly_fire, const bool prevent_wild_dino_damage);

		/**
		 * \brief Safe zone name
		 */
		FString name;

		/**
		 * \brief Sphere position
		 */
		FVector position;

		/**
		 * \brief Sphere radius
		 */
		int radius;

		bool prevent_pvp;
		bool prevent_structure_damage;
		bool prevent_building;
		bool prevent_friendly_fire;
		bool prevent_wild_dino_damage;

		/**
		 * \brief Kills wild dinos on entering to safe zone, requires events to be enabled
		 */
		bool kill_wild_dinos;

		/**
		 * \brief Doesn't allow players to leave safe zone
		 */
		bool prevent_leaving;

		bool prevent_entering;

		/**
		 * \brief Enables OnEnter/OnLeave events
		 */
		bool enable_events;

		bool screen_notifications;
		bool chat_notifications;

		FLinearColor success_color;
		FLinearColor fail_color;

		std::vector<FString> messages;

		bool cryopod_dinos;
		bool show_bubble;

		FLinearColor bubble_color;

		/**
		* \brief Sphere actor that detects overlaps
		*/
		TWeakObjectPtr<ATriggerSphere> trigger_sphere;

		// Callbacks

		TArray<std::function<void(AActor*)>> on_actor_begin_overlap;
		TArray<std::function<void(AActor*)>> on_actor_end_overlap;
		TArray<std::function<bool(AShooterPlayerController*)>> can_join_zone;

		TArray<TPair<int, int>> tribe_war_checks;

		bool bEnableOnEnterTeleport;
		FVector teleport_destination_on_enter;

		bool bOnlyKillAggressiveDinos;

		// Functions

		ZONE_API bool IsOverlappingActor(AActor* other) const;
		ZONE_API void SendNotification(AShooterPlayerController* player, const FString& message,
		                               const FLinearColor& color) const;

		void OnEnterSafeZone(AActor* other_actor);
		void OnLeaveSafeZone(AActor* other_actor);

		void DoPawnPush(APrimalCharacter* Pawn, bool bIsLeavePrevention);

		void SpawnBubble();

		// Functions to enable PvP zones in PvE servers
		void AddTribesPairForTribeWarCheck(const int Id1, const int Id2);
		void RemoveTribesFromTribeWarPair(const int Id1, const int Id2);
		bool AreTribesInTribeWarCheck(const int Id1, const int Id2);

		// On enter teleport functions
		void CheckTeleportForCharacter(APrimalCharacter* character);

		/**
		 * \brief Actors that are currently in safe zone
		 */
		TArray<AActor*> GetActorsInsideZone() const;

		ZONE_API bool CanJoinZone(AShooterPlayerController* player) const;

		void LogEvent(AShooterPlayerController* actor, bool bIsEnter) const;
	};
}
