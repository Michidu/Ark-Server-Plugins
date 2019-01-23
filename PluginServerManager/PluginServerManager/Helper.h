#pragma once

#include <functional>
#include <thread>

namespace Helper
{
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
					std::this_thread::sleep_for(std::chrono::seconds(after));
					task();
				}).detach();
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::seconds(after));
				task();
			}
		}
	};

	inline void LaunchApp(const std::wstring& application_name, LPWSTR cmd)
	{
		STARTUPINFOW si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof si);
		si.cb = sizeof si;
		ZeroMemory(&pi, sizeof pi);

		CreateProcessW(application_name.c_str(), cmd, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}
