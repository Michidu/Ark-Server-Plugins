#pragma once

#include <API/Ark/Ark.h>

class IDatabase
{
public:
	virtual ~IDatabase() = default;

	virtual bool TryAddNewPlayer(uint64 steam_id) = 0;
	virtual bool IsPlayerExists(uint64 steam_id) = 0;

	// Kits

	virtual std::string GetPlayerKits(uint64 steam_id) = 0;
	virtual bool UpdatePlayerKits(uint64 steam_id, const std::string& kits_data) = 0;
	virtual bool DeleteAllKits() = 0;

	// Points

	virtual int GetPoints(uint64 steam_id) = 0;
	virtual bool SetPoints(uint64 steam_id, int amount) = 0;
	virtual bool AddPoints(uint64 steam_id, int amount) = 0;
	virtual bool SpendPoints(uint64 steam_id, int amount) = 0;
	virtual bool AddTotalSpent(uint64 steam_id, int amount) = 0;
	virtual int GetTotalSpent(uint64 steam_id) = 0;
	virtual bool DeleteAllPoints() = 0;
};
