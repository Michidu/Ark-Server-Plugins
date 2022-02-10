#pragma once

#include "ArkHomes.h"
#include <thread>

namespace Helper
{
	class Timer
	{
	public:
		template <class callable, class... arguments>
		Timer(int after, bool async, uint64 steam_id, callable&& f, arguments&&... args)
		{
			std::function<typename std::invoke_result<callable, arguments...>::type()> task(
				std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

			if (async)
			{
				int uid = GetUid();

				threads[steam_id] = uid;

				std::thread thread = std::thread([uid, after, task, steam_id]()
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(after));

					if (auto iter = threads.find(steam_id);
						iter != threads.end() && iter->second == uid)
					{
						threads.erase(steam_id);

						task();
					}
				});

				thread.detach();
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(after));
				task();
			}
		}

		static int GetUid()
		{
			static std::atomic<uint32_t> uid{0};
			return ++uid;
		}

		static std::map<uint64, int> threads;
	};

	bool IsPlayerExists(unsigned __int64 steam_id);
	void DisableInput(AShooterPlayerController* player_controller, bool state);
}
