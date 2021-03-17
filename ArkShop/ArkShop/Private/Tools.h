#pragma once

#include "windows.h"

namespace ArkShop::Tools
{
	/* ============================================ [type definitions] ============================================ */

	/**
	*  \brief time struct
	*/
	typedef struct
	{
		int hours;       /**< \brief value for hours */
		int minutes;     /**< \brief value for minutes */
		int secounds;    /**< \brief value for secounds */

	}Time_t;


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

	/**
	* \brief Gets a Time_t 
	*
	* This functions returns a Time_t with difference of the time
	*
	* \param[in] AvailableTime the timer
	* \param[in] ServerRunTime the current server runntime
	* \return Time_t struct with the times
	*/
	inline Time_t GetAvailableTimeDiff(long double AvailableTime, long double ServerRunTime)
	{
		Time_t result = { 0 };

		int timediff = int(AvailableTime - ServerRunTime);

		if (0 < timediff)
		{
			result.hours = (int)(timediff / 3600);
			result.minutes = (int)(((float)(timediff % 3600)) / 60);
			result.secounds = (int)((float)((timediff % 3600) % 60));
		}

		return result;
	}


} // namespace Tools // namespace ArkShop
