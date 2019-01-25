#pragma once

#ifdef ARK_EXPORTS
#define ARK_API __declspec(dllexport)
#else
#define ARK_API __declspec(dllimport)
#endif

class FString;

namespace Permissions::DB
{
	/**
	 * \brief Checks if player exists in database
	 */
	ARK_API bool IsPlayerExists(unsigned long long steam_id);

	/**
	* \brief Checks if group exists in database
	*/
	ARK_API bool IsGroupExists(const FString& group);
}
