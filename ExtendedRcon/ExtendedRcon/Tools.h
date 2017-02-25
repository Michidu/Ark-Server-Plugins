#pragma once
#include <windows.h>
#include "API/Base.h"

AShooterPlayerController* FindPlayerControllerFromSteamId(unsigned __int64 steamId);
wchar_t* ConvertToWideStr(const std::string& str);