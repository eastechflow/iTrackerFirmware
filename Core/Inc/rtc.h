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

#ifndef __rtc_H
#define __rtc_H

#include "stm32l4xx_hal.h"
#include "main.h"

extern uint32_t CurrSeconds;
extern RTC_HandleTypeDef hrtc;

extern int rtc_BackupBatteryDepleted;
extern int rtc_RequiresSync;

#define RTC_SYNC_FROM_LOG   0
#define RTC_SYNC_FROM_APP   1
#define RTC_SYNC_FROM_CELL  2

#define RTC_SENTINEL_DATE_1_1_2018    1536969168UL

int8_t MX_RTC_Init(void);

uint32_t get_next_reading(void);
uint32_t get_next_wifi_wakeup(void);
uint32_t get_next_cell_wakeup(void);
uint32_t get_next_cell_registration_wakeup(int SnapTo);
int8_t sec_to_dt(uint32_t * time, struct tm * tm_val); //gmtime
int8_t dt_to_sec(struct tm * tm_val, uint32_t * time); //mktime

int8_t rtc_sync_time(struct tm * tm_val);
int8_t rtc_sync_time_from_ptime(uint32_t * ptime);

uint32_t rtc_get_time(void);
uint32_t rtc_get_datetime(struct tm * tm_val);
void set_rtc_alarm(long delay);
time_t convertDateToTime(const struct tm *date);

void change_from_UTC_to_local(void);
void change_from_local_to_UTC(void);

void rtc_set_factory_date(void);
void rtc_report_battery_charging_bit(void);

#define RTC_SNAP_TO_1_MIN   1
#define RTC_SNAP_TO_2_MIN   2
#define RTC_SNAP_TO_3_MIN   3
#define RTC_SNAP_TO_4_MIN   4
#define RTC_SNAP_TO_5_MIN   5
#define RTC_SNAP_TO_10_MIN  6
#define RTC_SNAP_TO_15_MIN  7
#define RTC_SNAP_TO_30_MIN  8
#define RTC_SNAP_TO_60_MIN  9

#endif
