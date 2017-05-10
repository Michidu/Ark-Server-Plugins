#include <windows.h>
#include <random>
#include "Tools.h"

namespace Tools
{
	int GetRandomNumber(int min, int max)
	{
		std::default_random_engine generator(std::random_device{}());
		std::uniform_int_distribution<int> distribution(min, max);

		int rnd = distribution(generator);

		return rnd;
	}

	std::string GetCurrentDir()
	{
		char buffer[MAX_PATH];
		GetModuleFileNameA(nullptr, buffer, MAX_PATH);
		std::string::size_type pos = std::string(buffer).find_last_of("\\/");

		return std::string(buffer).substr(0, pos);
	}
}
