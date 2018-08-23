#include "Helper.h"

namespace Helper
{
	std::map<uint64, int> Timer::threads;

	bool IsPlayerExists(uint64 steam_id)
	{
		auto& db = ArkHome::GetDB();

		int count;

		try
		{
			SQLite::Statement query(db, "SELECT count(1) FROM Players WHERE SteamId = ?;");
			query.bind(1, static_cast<int64>(steam_id));
			query.executeStep();

			count = query.getColumn(0).getInt();
		}
		catch (const std::exception& exception)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, exception.what());
			return false;
		}

		return count != 0;
	}

	void DisableInput(AShooterPlayerController* player_controller, bool state)
	{
		player_controller->GetPlayerCharacter()->bPreventMovement() = state;
		player_controller->GetPlayerCharacter()->bPreventJump() = state;

		if (ArkApi::IApiUtils::IsRidingDino(player_controller))
		{
			APrimalDinoCharacter* dino = ArkApi::IApiUtils::GetRidingDino(player_controller);

			dino->bPreventMovement() = state;
			dino->bPreventJump() = state;
		}
	}
}
