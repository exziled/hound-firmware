/**
 ******************************************************************************
 * @file    spark_wiring.cpp
 * @author  Satish Nair, Zachary Crockett, Zach Supalla and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#include "spark_wiring.h"


/*
 * @brief Should return the number of milliseconds since the processor started up.
 * 		  This is useful for measuring the passage of time.
 * 		  For now, let's not worry about what happens when this overflows (which should happen after 49 days).
 * 		  At some point we'll have to figure that out, though.
 */
system_tick_t millis(void)
{
    return GetSystem1MsTick();
}

/*
 * @brief Should return the number of microseconds since the processor started up.
 */
unsigned long micros(void)
{
	return (DWT->CYCCNT / SYSTEM_US_TICKS);
}

/*
 * @brief This should block for a certain number of milliseconds and also execute spark_wlan_loop
 */
void delay(unsigned long ms)
{
#ifdef SPARK_WLAN_ENABLE
	volatile system_tick_t spark_loop_elapsed_millis = SPARK_LOOP_DELAY_MILLIS;
	spark_loop_total_millis += ms;
#endif

	volatile system_tick_t last_millis = GetSystem1MsTick();

	while (1)
	{
	        KICK_WDT();

		volatile system_tick_t current_millis = GetSystem1MsTick();
		volatile long elapsed_millis = current_millis - last_millis;

		//Check for wrapping
		if (elapsed_millis < 0)
		{
			elapsed_millis = last_millis + current_millis;
		}

		if (elapsed_millis >= ms)
		{
			break;
		}

#ifdef SPARK_WLAN_ENABLE
		if (!SPARK_WLAN_SETUP || SPARK_WLAN_SLEEP)
		{
			//Do not yield for SPARK_WLAN_Loop()
		}
		else if ((elapsed_millis >= spark_loop_elapsed_millis) || (spark_loop_total_millis >= SPARK_LOOP_DELAY_MILLIS))
		{
			spark_loop_elapsed_millis = elapsed_millis + SPARK_LOOP_DELAY_MILLIS;
			//spark_loop_total_millis is reset to 0 in SPARK_WLAN_Loop()
			do
			{
				//Run once if the above condition passes
				SPARK_WLAN_Loop();
			}
			while (SPARK_FLASH_UPDATE);//loop during OTA update
		}
#endif
	}
}

/*
 * @brief This should block for a certain number of microseconds.
 */
void delayMicroseconds(unsigned int us)
{
	Delay_Microsecond(us);
}



