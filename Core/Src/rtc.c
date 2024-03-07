/*************************************************************************
* COPYRIGHT - All Rights Reserved.
* 2019 Eastech Flow Controls, Inc.
*
* OWNER:
* Eastech Flow Controls, Inc.
* 4250 S. 76th E. Avenue
* Tulsa, Oklahoma 74145
*
* toll-free: 	(800) 226-3569
* phone: 	(918) 664-1212
* fax: 	(918) 664-8494
* email: 	info@eastechflow.com
* website: 	http://eastechflow.com/
*
* OBLIGATORY: All information contained herein is, and remains
* the property of EASTECH FLOW CONTROLS, INC. The intellectual
* and technical concepts contained herein are proprietary to
* EASTECH FLOW CONTROLS, INC. and its suppliers and may be covered
* by U.S. and Foreign Patents, patents in process, and are protected
* by trade secret or copyright law.
*
* Dissemination, transmission, forwarding or reproduction of this
* content in any way is strictly forbidden.
*************************************************************************/

/**
 *
 *
 *
 * https://www.epochconverter.com/
 */

#include "rtc.h"
#include "time.h"
#include "debug_helper.h"
#include "factory.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_rtc.h"
#include "stm32l4xx_hal_rtc_ex.h"
#include "stm32l4xx_hal_rcc.h"
#include "stm32l4xx_hal_rcc_ex.h"
#include "stm32l4xx_hal_pwr.h"
#include "stm32l4xx_hal_pwr_ex.h"

// RTC Handles
RTC_HandleTypeDef hrtc;
RTC_AlarmTypeDef sAlarm;

int rtc_BackupBatteryDepleted = 0;
int rtc_RequiresSync = 0;

static int rtc_initialized = 0;

// Local members.
uint32_t CurrSeconds = 0;	// Current RTC seconds
uint8_t LastDay_initialized = 0;
uint8_t LastDay = 0;
uint32_t SleepTime = 0;
uint32_t LowTime = 0;
float tmpfloat = 0;
float tmpfloat2 = 0;

// Private foward declarations.

uint8_t computeDayOfWeek(uint16_t y, uint8_t m, uint8_t d);
void convertTimeToDate(time_t t, struct tm *date);

uint8_t dayofweek(int d, int m, int y)
{
	static int mon[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	int t = 0;
	if (y % 4 == 0 && (y % 100 || y % 400 == 0) && m < 3)
		t = 1;
	return (y * 365 + y / 4 - y / 100 + y / 400 + mon[m - 1] - t + d - 1) % 7;
}

// Get the next time to take a measurement and record battery values once per day.
// zg - This is not really using the "time anchor" from the previous reading.  May result in "time extended" intervals depending on speed.
uint32_t get_next_reading(void)
{
	uint32_t tmp;
	uint32_t tmp32;

	struct tm tm_val;

	rtc_get_datetime(&tm_val);   // <=== this was just called prior to calling this function

	LastLogHour = tm_val.tm_hour;
	LastLogDay = tm_val.tm_mday;

	// At power-on, LastDay needs to be initialized
	if (LastDay_initialized == 0)
	{
		LastDay = tm_val.tm_mday;
		LastDay_initialized = 1;
	}


	if (LastDay != tm_val.tm_mday)
	{
		float new_accumulator;

		new_accumulator = ltc2943_get_accumulator();


            // zg - note that Monday is 1, Saturday is 6, and Sunday is 7.
			// At power on, LastDay is zero.
			// At power on, MainConfig.val might be zero (there is no previous days data stored in flash).
			// In that case (we) and (wd) cannot be updated.
			//
			// This code is supposed to compute and save a previous day's amount of
			// battery consumption.  Hence, if the current day is
			// Sunday the previous day was Saturday, and if the current day is
			// Monday, the previous day was Sunday.  Those cases identify
			// "weekend" (we) values.  Otherwise, it is a "weekday" (wd) value.
			// Note that (we) and (wd) are only valid at certain times,
			// e.g. once a full week has passed and provided a power-cycle didn't occur
			// and provided the batteries were not changed.

			if ((LastDay == 7) || (LastDay == 1))
			{
				MainConfig.we_current = MainConfig.val - new_accumulator;
			}
			else
			{
				MainConfig.wd_current = MainConfig.val - new_accumulator;
			}



		MainConfig.val = new_accumulator;

		LastDay = tm_val.tm_mday;

		saveMainConfig = 1; // Save new battery values once per day

		//Save_MainConfig();  // at power on, this is not a good idea, can corrupt main flash if magnet debounce is happening

	}

    // zg - note: this code does not work precisely for 1 minute intervals.
	// zg - it allows multiple readings per minute which is Ok for testing.

	// Compute in seconds.
	tmp = tm_val.tm_min * 60 / MainConfig.log_interval;
	tm_val.tm_min = 0;
	tm_val.tm_sec = 0;
	tmp32 = convertDateToTime(&tm_val);

	tmp32 += ((tmp + 1) * MainConfig.log_interval);
	return tmp32;
}

#if 0
// zg - Brian's wake up is wrong.  On Saturday's it wakes up on Sundays
//      On Sundays it wakes up on Tuesdays.
uint32_t calc_next_wifi_wakeup_brian(struct tm * tm_st)
{
	uint32_t LowTime;
	uint8_t tmp;
	uint8_t tmpday;

	tmpday = tm_st->tm_wday;

	if (MainConfig.wifi_wakeup_interval)
		tmp = tm_st->tm_min * 60 / MainConfig.wifi_wakeup_interval ;
	else
		tmp = tm_st->tm_min * 60 / 300 ;
	tm_st->tm_min = 0;
	tm_st->tm_sec = 0;

	dt_to_sec(tm_st, &LowTime);
	if (tmpday < 2)
	{
		LowTime += ((18 - tm_st->tm_hour)*60*60); // must be Sunday so add 24hr

		sec_to_dt(&LowTime,tm_st);
	}
	else if (tmpday > 6)
	{
		LowTime += ((42 - tm_st->tm_hour)*60*60); // must be Saturday so add 24hr
		sec_to_dt(&LowTime,tm_st);
	}
	if (tm_st->tm_hour > 17) // if past 6pm ie 5:59
	{
		LowTime += ((32 - tm_st->tm_hour)*60*60); // add hours to go past midnight
	}
	else if (tm_st->tm_hour < 8) // if before 8am
		LowTime += ((8 - tm_st->tm_hour) * 60 * 60);
	else
		LowTime += ((tmp + 1) * MainConfig.wifi_wakeup_interval);

	return LowTime;
}
#endif

// zg - jason's wakeup is correct.
uint32_t calc_next_wifi_wakeup_jason(struct tm * tm_st, int SnapTo)
{
	uint32_t LowTime;
	uint32_t tmp;
	uint32_t tmpday;
	uint32_t snap_sec;

	switch (SnapTo)
	{
	case RTC_SNAP_TO_1_MIN:    snap_sec =  1*60; break;
	case RTC_SNAP_TO_2_MIN:    snap_sec =  2*60; break;
	case RTC_SNAP_TO_3_MIN:    snap_sec =  3*60; break;
	case RTC_SNAP_TO_4_MIN:    snap_sec =  4*60; break;
	default:
	case RTC_SNAP_TO_5_MIN:    snap_sec =  5*60; break;
	case RTC_SNAP_TO_10_MIN:   snap_sec = 10*60; break;
	case RTC_SNAP_TO_15_MIN:   snap_sec = 15*60; break;
	case RTC_SNAP_TO_30_MIN:   snap_sec = 30*60; break;
	case RTC_SNAP_TO_60_MIN:   snap_sec = 60*60; break;
	}

	tmpday = tm_st->tm_wday;

	tmp = tm_st->tm_min * 60 / snap_sec;   // snap to desired minute mark

	tm_st->tm_min = 0;
	tm_st->tm_sec = 0;

	LowTime = convertDateToTime(tm_st);

	// No wifi on weekends.
	if (tmpday == 6) {
		// Saturday so add 48hr
		LowTime += ((42 - tm_st->tm_hour)*60*60);
		sec_to_dt(&LowTime,tm_st);
	}
	else if (tmpday == 7) {
		// Sunday so add 24hr
		LowTime += ((18 - tm_st->tm_hour)*60*60);
		sec_to_dt(&LowTime,tm_st);
	}

	// MON-FRI (1-5) 8am to 6pm only.
	if (tm_st->tm_hour > 17) {
		// After 6
		LowTime += ((32 - tm_st->tm_hour)*60*60);
	}
	else if (tm_st->tm_hour < 8) {
		// Before 8am - Don't wake up yet.
		LowTime += ((8 - tm_st->tm_hour) * 60 * 60);
	}
	else {
		// Within range of eligible wakeups (8am-6pm, Mon-Fri)
		LowTime += ((tmp + 1) * snap_sec);   // snap to desired minute mark
	}
	return LowTime;
}

uint32_t calc_next_wifi_wakeup_for_app(struct tm * tm_st, int SnapTo)
{
	uint32_t LowTime;
	uint32_t tmp;
	uint32_t snap_sec;

	switch (SnapTo)
	{
	case RTC_SNAP_TO_1_MIN:    snap_sec =  1*60; break;
	case RTC_SNAP_TO_2_MIN:    snap_sec =  2*60; break;
	case RTC_SNAP_TO_3_MIN:    snap_sec =  3*60; break;
	case RTC_SNAP_TO_4_MIN:    snap_sec =  4*60; break;
	default:
	case RTC_SNAP_TO_5_MIN:    snap_sec =  5*60; break;
	case RTC_SNAP_TO_10_MIN:   snap_sec = 10*60; break;
	case RTC_SNAP_TO_15_MIN:   snap_sec = 15*60; break;
	case RTC_SNAP_TO_30_MIN:   snap_sec = 30*60; break;
	case RTC_SNAP_TO_60_MIN:   snap_sec = 60*60; break;
	}

	tmp = tm_st->tm_min * 60 / snap_sec;

	tm_st->tm_min = 0;
	tm_st->tm_sec = 0;

	LowTime = convertDateToTime(tm_st);

	LowTime += ((tmp + 1) *  snap_sec);

	return LowTime;
}

#if 0
// The previous WiFi Wakeup schedule was:
// M-F, 8-6   which is 10 hrs/day x 5 days = 50 hours
// It was "on the fives" so wake up 12 times per hour, for a duration of 60 seconds each
// 50 hrs x 12 wakeups/hr= 600 wakeups
// 600 wakeups x 60 seconds / wakeup = 36,000 seconds of "on" time.
// If the minimum wakeup is 10 seconds (5 to power up, 5 to wait for BLE)
// The same number of seconds can be distributed across shorter 10 sec wakeups:
// 36,000 / 10 = 3600 wakeups
// 3600 / 5 days = 720 per day
// 720 / 10 hrs =  72 per hour
// which is faster than once per minute.
// If distribute across all 7 days of the week and hrs of the day:
// 3600 / 7 = 514 per day
// 514 / 24 = 21 per hour
// 60 mintues / 21 = every 2.85 minutes
// So a uniform 3 minute interval would distribute the "on" time evenly.
// Note that a shorter "on" time would make things better.
// Need to measure just how fast the overall process actually is.
//
#endif

// Get the next Wifi wakeup time.
uint32_t get_next_wifi_wakeup(void)
{
	struct tm tm_st;

	rtc_get_datetime(&tm_st);

	//return calc_next_wifi_wakeup_for_app(&tm_st, RTC_SNAP_TO_5_MIN);  //  FOR TESTING ON EVENINGS AND WEEKENDS !!

#ifdef LOG_SLEEP_WAKE
	return calc_next_wifi_wakeup_for_app(&tm_st, RTC_SNAP_TO_3_MIN);  //
#endif

#ifdef ENABLE_AUTO_WAKEUP
    // for more frequent, but shorter, autowakeups
	return calc_next_wifi_wakeup_for_app(&tm_st, RTC_SNAP_TO_3_MIN);  //
#else
  // original
  return calc_next_wifi_wakeup_jason(&tm_st, RTC_SNAP_TO_5_MIN);  // M-F, 8 am - 6 pm, on the fives, for one minute
#endif

}

// Get the next cell wakeup time.
uint32_t get_next_cell_registration_wakeup(int SnapTo)
{
	struct tm tm_st;

	rtc_get_datetime(&tm_st);

	return calc_next_wifi_wakeup_for_app(&tm_st, SnapTo);  //
}


#if 0
void test_wifi_wakeup(void)
{
	uint32_t testTime;
	uint32_t endTime;

	uint32_t brianTime;
	uint32_t jasonTime;

	struct tm test_tm_st;
	struct tm brian_tm_st;
	struct tm jason_tm_st;

	rtc_get_datetime(&test_tm_st);

	test_tm_st.tm_min = 0;
	test_tm_st.tm_sec = 0;

	testTime = convertDateToTime(&test_tm_st);
	endTime = testTime + (8L * 24L * 60L * 60L);  // test an 8 day span

	while (testTime < endTime)
	{
		sec_to_dt(&testTime, &test_tm_st);  // create a new test datetime struct

		brian_tm_st = test_tm_st;  // these need to be kept separate as calc function modifies
		jason_tm_st = test_tm_st;  // these need to be kept separate as calc function modifies


		brianTime = calc_next_wifi_wakeup_brian(&brian_tm_st);
		jasonTime = calc_next_wifi_wakeup_jason(&jason_tm_st);

		sec_to_dt(&brianTime, &brian_tm_st);  // convert back...
		sec_to_dt(&jasonTime, &jason_tm_st);  // convert back...

		if (brianTime != jasonTime)
		{
		    // now print all three date/time values
		    //testTime, brianTime, jasonTime
			for (int i = 0; i < 10; i++);  // for debug breakpoint
		}

		testTime += 300;  // test every 5 minute mark for 8 consecutive days
	}

}
#endif

// Get the next Cell wakeup time. (every 24 hours at midnight)
uint32_t get_next_cell_wakeup(void)
{
	// MON-SUN - wake up at midnight and transmit
	struct tm tm_st;
	rtc_get_datetime(&tm_st);
	tm_st.tm_hour = 23;
	tm_st.tm_min = 59;
	tm_st.tm_sec = 59;

	uint32_t wakeEpoch = convertDateToTime(&tm_st);
	return wakeEpoch;
}

uint32_t rtc_get_datetime(struct tm * tm_val)
{

	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;

	if (hrtc.Instance != RTC) // The RTC has not yet been initialized.
	{
      convertTimeToDate(RTC_SENTINEL_DATE_1_1_2018, tm_val);
      return RTC_SENTINEL_DATE_1_1_2018;
	}

	HAL_RTC_GetTime(&hrtc, &sTime, CLK_FORMAT);
	HAL_RTC_GetDate(&hrtc, &sDate, CLK_FORMAT);

	tm_val->tm_hour = sTime.Hours;
	tm_val->tm_min = sTime.Minutes;
	tm_val->tm_sec = sTime.Seconds;
	tm_val->tm_wday = sDate.WeekDay;
	tm_val->tm_mon = sDate.Month;
	tm_val->tm_mday = sDate.Date;
	tm_val->tm_year = sDate.Year + 2000;

	return convertDateToTime(tm_val);
}

uint32_t rtc_get_time(void)
{

	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;

	if (hrtc.Instance != RTC) return RTC_SENTINEL_DATE_1_1_2018;   // The RTC has not yet been initialized.

	HAL_RTC_GetTime(&hrtc, &sTime, CLK_FORMAT);
	HAL_RTC_GetDate(&hrtc, &sDate, CLK_FORMAT);

	if (sDate.WeekDay == RTC_WEEKDAY_SUNDAY) sDate.WeekDay = 0;   // sDate.WeekDay is 1..7 where 1=Monday; tm_wday is 0-6 where Sunday is zero

	struct tm tm_val;
	tm_val.tm_hour = sTime.Hours;
	tm_val.tm_min = sTime.Minutes;
	tm_val.tm_sec = sTime.Seconds;
	tm_val.tm_wday = sDate.WeekDay;   // sDate.WeekDay is 1..7 where 1=Monday; tm_wday is 0-6 where Sunday is zero
	tm_val.tm_mon = sDate.Month;
	tm_val.tm_mday = sDate.Date;
	tm_val.tm_year = sDate.Year + 2000;

	return convertDateToTime(&tm_val);

}

int8_t dt_to_sec(struct tm * tm_val, uint32_t * time)
{
	time_t timestamp;
	timestamp = convertDateToTime(tm_val);
	if (timestamp < 0)
		return -1;
	*time = (uint32_t) timestamp;

	return 0;
}

int8_t sec_to_dt(uint32_t * time, struct tm * tm_val)
{
	time_t tmptime = *time;
	convertTimeToDate(tmptime, tm_val);
	//tm_val->tm_year -= 4;
	return 0;
}


int8_t rtc_sync_time(struct tm * tm_val)
{
	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;

	if (hrtc.Instance != RTC) return -1;   // The RTC has not yet been initialized.

	sTime.Hours = tm_val->tm_hour;
	sTime.Minutes = tm_val->tm_min;
	sTime.Seconds = tm_val->tm_sec;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;
	sDate.Month = tm_val->tm_mon;
	sDate.Date = tm_val->tm_mday;
	sDate.Year = tm_val->tm_year - 2000;
	sDate.WeekDay = dayofweek(sDate.Date, sDate.Month, sDate.Year + 2000);

	if (HAL_RTC_SetTime(&hrtc, &sTime, CLK_FORMAT) != HAL_OK) {
		return -1;
	}

	if (HAL_RTC_SetDate(&hrtc, &sDate, CLK_FORMAT) != HAL_OK) {
		return -1;
	}


	if (rtc_RequiresSync)
	{
		//AppendStatusMessage("RTC set.");  // due to stack depth issues, don't risk calling a bunch of code from here.

		rtc_RequiresSync = 0;  // We are now running with a corrected RTC
	}


	// must update the main loop "stay awake" values, otherwise the unit may go to sleep unexpectedly or stay on longer than expected

	//Update the main variables that are time-based
	main_rtc_changed();


	return 0;
}




int8_t rtc_sync_time_from_ptime(uint32_t * ptime)
{

	struct tm tm_st;

	// convert time to struct

	sec_to_dt(ptime, &tm_st);

	return rtc_sync_time(&tm_st);
}

// https://electronics.stackexchange.com/questions/116745/stm32-rtc-alarm-not-configuring-for-the-second-time
/*void set_rtc_alarm(long delay) {
	if (delay >= 0) {
		struct tm tm_st;
		struct tm * tm_val = &tm_st;
		convertTimeToDate((time_t)delay, tm_val);

		// Turn off the alarm.
		HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);

		// Enable the Alarm A
		RTC_AlarmTypeDef sAlarm;
		sAlarm.AlarmTime.TimeFormat = RTC_HOURFORMAT_24;
		sAlarm.AlarmTime.Hours = tm_val->tm_hour;
		sAlarm.AlarmTime.Minutes = tm_val->tm_min;
		sAlarm.AlarmTime.Seconds = tm_val->tm_sec;
		sAlarm.AlarmTime.SubSeconds = 0x0;
		sAlarm.AlarmTime.SecondFraction = 0x0;
		sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
		sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
		sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
		sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
		sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
		sAlarm.AlarmDateWeekDay = tm_val->tm_wday;
		sAlarm.Alarm = RTC_ALARM_A;

		// Set the alarm.
 		if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, CLK_FORMAT) != HAL_OK) {
			_Error_Handler(__FILE__, __LINE__);
		}

		// Clear flags
		__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
		__HAL_RTC_ALARM_EXTI_CLEAR_FLAG();
		__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
		__HAL_RTC_CLEAR_FLAG(RTC_EXTI_LINE_WAKEUPTIMER_EVENT);
	}
}*/

void rtc_set_factory_date(void)
{
	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;

	if (hrtc.Instance != RTC) return;   // The RTC has not yet been initialized.

	sTime.Hours = 0;
	sTime.Minutes = 0;
	sTime.Seconds = 0;
	sTime.SubSeconds = 0;
	sTime.SecondFraction = 0;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;

	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}
	// Default RTC date is:  Monday 1/1/2018  00:00:00

	sDate.WeekDay = RTC_WEEKDAY_MONDAY;
	sDate.Month = RTC_MONTH_JANUARY;
	sDate.Date = 1; // Originally 0x24 = 36 which is beyond the allowed range for this data structure!
	sDate.Year = 18; // Originally 0x19 = 25 which is an unreasonable year for a default value (too far in the future)

	if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}
}

// RTC init function
int8_t MX_RTC_Init(void)
{


	// Initialize RTC Only
	hrtc.Instance            = RTC;
	hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv   = 127; // https://www.st.com/content/ccc/resource/technical/document/application_note/group0/71/b8/5f/6a/8e/d5/45/0a/DM00226326/files/DM00226326.pdf/jcr:content/translations/en.DM00226326.pdf
	hrtc.Init.SynchPrediv    = 255;
	hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutRemap    = RTC_OUTPUT_REMAP_NONE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

	if (HAL_RTC_Init(&hrtc) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	rtc_initialized = 1;

	// Initialize RTC and set the Time and Date
	// Based on backup register as a default.
	uint32_t t = rtc_get_time();

//t = 0; // Force Test of status message (simulate depleted RTC backup batteries)

	// 1563900000 is Tuesday, July 23, 2019 16:40:00
	// Not entirely sure why that date is of any value
	// 1536969168 is the factory default date
  //if ((t < 1563900000UL) || (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2))
	//if ((t < RTC_SENTINEL_DATE_1_1_2018) || (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2))

	if (t < RTC_SENTINEL_DATE_1_1_2018)		// if the RTC really lost power, then the date it will have at power on is zero, so no need to read/write to a backup register
	{
		rtc_set_factory_date(); // Default RTC date is:  1/1/2018  00:00:00

		//HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F2);

		// 5-25-2022 zg The first production run of iTrackers epoxied bad RTC backup batteries in place and they could not be replaced.
		// Rather than brick the units, a decision was made to detect the fact that 1/1/2018 is a sentinel value if the RTC knows it lost power.
		// A decision was made for the App to automatically sync time and therefore mitigate the problem (mostly).
		// However, a message will be logged to the SD card reporting the state of the backup batteries to assist with future support issues.
		// Also, an entry in the log file will be made to indicate when a power-cycle occurs on units without a working RTC.
		// If the RTC is not synced before the unit goes to sleep, the unit will attempt to sync with the last known timestamp
		// taken from the log file.  A duplicate log entry will indicate when that sync occured.

	    rtc_BackupBatteryDepleted = 1;
	    rtc_RequiresSync = 1;
	}


	// Set Alarm A.
	sAlarm.AlarmTime.Hours = 0;
	sAlarm.AlarmTime.Minutes = 0;
	sAlarm.AlarmTime.Seconds = 0;
	sAlarm.AlarmTime.SubSeconds = 0;
	sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
	sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
	sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
	sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
	sAlarm.AlarmDateWeekDay = 1;
	sAlarm.Alarm = RTC_ALARM_A;

	if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	// Set Alarm B.
	sAlarm.AlarmDateWeekDay = 1;
	sAlarm.Alarm = RTC_ALARM_B;
	if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	return 0;
}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle) {
	if (rtcHandle->Instance == RTC) {
		__HAL_RCC_RTC_ENABLE();
		HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0x0, 0);
		HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
	}
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle) {
	if (rtcHandle->Instance == RTC) {
		__HAL_RCC_RTC_DISABLE();
		HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
	}
}

uint8_t computeDayOfWeek(uint16_t y, uint8_t m, uint8_t d) {
	uint32_t h;
	uint32_t j;
	uint32_t k;

	//January and February are counted as months 13 and 14 of the previous year
	if (m <= 2) {
		m += 12;
		y -= 1;
	}

	//J is the century
	j = y / 100;
	//K the year of the century
	k = y % 100;

	//Compute H using Zeller's congruence
	h = d + (26 * (m + 1) / 10) + k + (k / 4) + (5 * j) + (j / 4);

	//Return the day of the week
	return ((h + 5) % 7) + 1;
}

time_t convertDateToTime(const struct tm *date)
{
	uint32_t y;
	uint32_t m;
	uint32_t d;
	uint32_t t;

	// Year
	y = date->tm_year;

	// Month of year
	m = date->tm_mon;

	// Day of month
	d = date->tm_mday;

	//January and February are counted as months 13 and 14 of the previous year
	if (m <= 2) {
		m += 12;
		y -= 1;
	}

	// Convert years to days
	t = (365 * y) + (y / 4) - (y / 100) + (y / 400);

	// Convert current months to days
	t += (30 * m) + (3 * (m + 1) / 5) + d;

	// Deduct the unix time: starts on January 1st, 1970
	t -= 719561;

	// Convert total days to seconds
	t *= 86400;

	// Add current time: hours, minutes and seconds
	t += (3600 * date->tm_hour) + (60 * date->tm_min) + date->tm_sec;

	// Return Unix time
	return t;
}

void convertTimeToDate(time_t t, struct tm *date)
{
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint32_t e;
	uint32_t f;

	//Negative Unix time values are not supported
	if (t < 0)
		t = 0;

	//Retrieve hours, minutes and seconds
	date->tm_sec = t % 60;
	t /= 60;
	date->tm_min = t % 60;
	t /= 60;
	date->tm_hour = t % 24;
	t /= 24;

	//Convert Unix time to date
	a = (uint32_t) ((4 * t + 102032) / 146097 + 15);
	b = (uint32_t) (t + 2442113 + a - (a / 4));
	c = (20 * b - 2442) / 7305;
	d = b - 365 * c - (c / 4);
	e = d * 1000 / 30601;
	f = d - e * 30 - e * 601 / 1000;

	//January and February are counted as months 13 and 14 of the previous year
	if (e <= 13) {
		c -= 4716;
		e -= 1;
	} else {
		c -= 4715;
		e -= 13;
	}

	//Retrieve year, month and day
	date->tm_year = c;    //-1900;
	date->tm_mon = e;
	date->tm_mday = f;

	//Calculate day of week
	date->tm_wday = computeDayOfWeek(c, e, f);
}

void rtc_report_battery_charging_bit(void)
{
	if (GetRTCBatt() == RTC_BATT_RECHARGE_ON_1_5)
	{
      AppendStatusMessage("RTC battery charging is ON");
	}
	else
	{
	  AppendStatusMessage("RTC battery charging is OFF");
	}
}


