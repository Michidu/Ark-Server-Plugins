#pragma once

#include <functional>
#include <thread>

class Timer
{
public:
	template <class callable, class... arguments>
	Timer(int after, bool async, callable&& f, arguments&&... args)
	{
		std::function<typename std::result_of<callable(arguments ...)>::type()> task(
			std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

		if (async)
		{
			std::thread([after, task]()
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(after));
				task();
			}).detach();
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(after));
			task();
		}
	}
};


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
