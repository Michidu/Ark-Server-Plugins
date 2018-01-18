#pragma once

#include <API/ARK/Ark.h>

#ifdef ARK_EXPORTS
#define ARK_API __declspec(dllexport)
#else
#define ARK_API __declspec(dllimport)
#endif

namespace Permissions
{
	ARK_API bool IsPlayerInGroup(uint64 steam_id, const std::string& group);
	ARK_API bool IsPlayerInGroup(uint64 steam_id, const FString& group);

	ARK_API bool AddPlayerToGroup(uint64 steam_id, const std::string& group);
	ARK_API bool RemovePlayerFromGroup(uint64 steam_id, const std::string& group);

	ARK_API bool AddGroup(const std::string& group);
	ARK_API bool RemoveGroup(const std::string& group);
}
