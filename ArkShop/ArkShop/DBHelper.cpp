#include "DBHelper.h"

namespace DBHelper
{	
	bool IsPlayerEntryExists(__int64 steamId)
	{
		auto db = GetDB();

		int count = 0;

		try
		{
			db << "SELECT count(1) FROM Players WHERE SteamId = ?;" << steamId >> count;
		}
		catch (sqlite::sqlite_exception& e)
		{
			std::cout << "Unexpected DB error " << e.what() << std::endl;

			return false;
		}

		return count != 0;
	}
}