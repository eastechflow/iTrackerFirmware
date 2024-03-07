/*
 * battery.c
 *
 *  Created on: Mar 31, 2020
 *      Author: Zachary
 */

#include "LTC2943.h"
#include "quadspi.h"
#include "log.h"
#include "rtc.h"
#include "battery.h"

/*

3-30-2020 The existing code uses three variables:

LogTrack.accumulator  : saved to flash at each data point.  Can use last data point to get an actual timestamp.
MainConfig.val        : saved once per day, at the "first data point" taken on that day, so usually just after midnight.
                        so, LogTrack.accumulator will equal MainConfig.val for that particular data point only.
                        Usually LogTrack.accumulator will be larger (more accumulated charge means more accumulated on-time)
                        Note that this value can be used to get an amount of current used between two points in time.
MainConfig.we_current : Only contains data for one day (either Saturday or Sunday).  Requires rtc day-change (up to 24 hours of on-time or more) before first available.
MainConfig.wd_current : Only contains data for one day (M-F). Requires rtc day-change (up to 24 hours of on-time or more) before first available.


At power-on:
  ltc2493 initializes to mid-scale (half full)
  If flash is empty then
     LogTrack.accumulator =  MAX_BATTERY = 15000.0
     MainConfig.val = 0
     MainConfig.we_current = 35
     MainConfig.wd_current = 80
  otherwise
     LogTrack.accumulator = last saved value if a data point was taken, otherwise MAX_BATTERY from initialization
     ltc2493 is set to this value
     if a new day has passed...(partial or full?)
     if a Saturday has passed...(partial or full ?)
     if a Sunday has passed...(partial or full?)
     if a data point was taken

So, to report an estimated number of days remaining is tricky, depending on how much time the unit has been on.
If it has not been on for 24 hours, then none of the "day" based values are technically correct.
But the code does not check for 24 hours, it just checks the day setting.
If the code has not run for a weekend day or a weekday, those values are also incorrect.



*/

typedef struct
{
	uint32_t timestamp;
	float voltage;
	float current;
	float temperature;
} BATTERY_INFO;

typedef struct
{
	BATTERY_INFO at_install;
	BATTERY_INFO at_previous_power_on;
} LOGGED_BATTERY_INFO;


void battery_init(void)
{
	// Get last recorded battery info from flash

	// Get current battery info

	// Compare, decide if new batteries were installed ??  Might need "at_auto_install" and "at_manual_install" to keep track ??

	// Save battery info to flash

	// Initialize the battery accumulator



}
