#pragma once

#include "ArkHomes.h"

namespace HomeSystem
{
	extern std::vector<uint64> teleporting_players;

	void Init();
	void RemoveHooks();
	void AddPlayer(uint64 steam_id);
	bool IsEnemyStructureNear(AShooterPlayerController* player_controller, int radius);
}
