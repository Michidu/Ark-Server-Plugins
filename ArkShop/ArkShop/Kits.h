#pragma once

#include "ArkShop.h"

namespace Kits
{
	void Init();
	void AddKit(const std::string& kitName, int newAmount, __int64 steamId);
}