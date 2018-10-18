#pragma once

#include "windows.h"

namespace ArkShop::Tools
{
	inline int GetRandomNumber(int min, int max)
	{
		const int n = max - min + 1;
		const int remainder = RAND_MAX % n;
		int x;

		do
		{
			x = rand();
		}
		while (x >= RAND_MAX - remainder);

		return min + x % n;
	}
} // namespace Tools // namespace ArkShop
