#pragma once

#include <API/ARK/Ark.h>
#include "Database/IDatabase.h"
#include "Database/SqlLiteDB.h"
#include "Database/MysqlDB.h"
#include <map>

IDatabase* GetDB();
/*
std::map<uint64, TArray<size_t>>& GetPlayerCache();
std::map<size_t, TArray<size_t>>& GetGroupCache();*/