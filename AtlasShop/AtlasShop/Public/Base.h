#pragma once

#include <API/Atlas/Atlas.h>
#include <API/UE/Math/ColorList.h>
#include <Tools.h>

#ifdef SHOP_EXPORTS
#define SHOP_API __declspec(dllexport)
#else
#define SHOP_API __declspec(dllimport)
#endif

namespace AtlasShop
{
}