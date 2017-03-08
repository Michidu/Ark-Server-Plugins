#pragma once

#ifdef SHOP_EXPORTS
#define SHOP_API __declspec(dllexport) 
#else
#define SHOP_API __declspec(dllimport)
#endif

namespace ShopAPI
{
	SHOP_API bool AddPoints(int amount, __int64 steamId);
	SHOP_API bool SpendPoints(int amount, __int64 steamId);
	SHOP_API int GetPoints(__int64 steamId);
}