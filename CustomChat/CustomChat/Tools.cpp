#include <windows.h>
#include "Tools.h"

namespace Tools
{
	std::string GetCurrentDir()
	{
		char buffer[MAX_PATH];
		GetModuleFileNameA(nullptr, buffer, MAX_PATH);
		std::string::size_type pos = std::string(buffer).find_last_of("\\/");

		return std::string(buffer).substr(0, pos);
	}

	wchar_t* ConvertToWideStr(const std::string& str)
	{
		size_t newsize = str.size() + 1;

		wchar_t* wcstring = new wchar_t[newsize];

		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, wcstring, newsize, str.c_str(), _TRUNCATE);

		return wcstring;
	}
}
