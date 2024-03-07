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

#include <Gen4_TempF_STLM20.h>
#include "main.h"
#include "math.h"
#include "Common.h"
#include "WiFi_Socket.h"
#include "Log.h"
#include "rtc.h"
#include "Sensor.h"
#include "DeltaQ.h"
#include "quadspi.h"
#include "Hydro.h"
#include "debug_helper.h"
#include "s2c.h"
#include "factory.h"
#include "fatfs.h" // SD Card
#include "sdmmc.h" // SD Card

// zg 2020-Jan-29  code version 5.0.9924
//
// Timing measurements on as-found code to read all log entries
// from the entire flash with indicated cache size.
// The total number of log records in flash is:
// (0xFFFFFF - 0x200000) = 14,680,032 bytes divided by 32 = 458,751 records
//
//                  Total
//  Rec     Cache   Time           Num       Time    Time
//  Per     Size    in             Cache     Per     Per
//  Cache   Bytes   Sec      Min   Reads     Cache   Record
//    1       32    1377    22.95  458,751   Read
//   10      320     138     2.3    45,875   0.003
//   20      640      74            22,937   0.003
//   32     1024      58            14,335   0.004
//   64     2048      35             7,167   0.005
//
//  One day = 24 hrs = 1440 min divided by 1 rec/15 min gives 96 data records
//  So, for a 30 day graph, it would take about 30 cache reads for a 2KB cache.
//
// What is not known is how much faster it would be if 4 data lines were used for SPI.
// Also, what is not known is if WiFi data transmission will impact overall speeds.
//



uint8_t LastLogHour = 0;
uint8_t LastLogDay = 0;

#define USE_READ_CACHE

#ifdef USE_READ_CACHE

#define CACHE_SIZE   1024  // must be a multiple of sizeof(log_t) (32 bytes)

static uint32_t cache_start_addr=0;
static uint32_t cache_end_addr=0;   // to handle rollovers, cache may not always be full size
static uint32_t cache_len=0;        // how many bytes of the cache contain valid data
static uint8_t  cached_bytes[CACHE_SIZE];
#endif


#if 0
static int8_t PartialLogDay = 1;
static float   DQ_HourLow = 0;
static float   DQ_HourAvg = 0;
static uint8_t DQ_HourAvgCnt = 0;
static float   DQ_DayAvg = 0;
static uint8_t DQ_DayAvgcnt = 0;

static void update_DQ(uint32_t time, float qratio)
{
	struct tm tmpdate;
	sec_to_dt(&time, &tmpdate);
	if (LastLogHour != tmpdate.tm_hour)
	{
		if (DQ_HourAvgCnt)
			DQ_HourAvg /= DQ_HourAvgCnt;

		if (LastLogDay != tmpdate.tm_mday)
		{
			if (!PartialLogDay)
			{
				if (DQ_DayAvgcnt)
					DQ_DayAvg /= DQ_DayAvgcnt;

				if (DQ_DayAvgcnt == 24)
				{
					Delta_Q_AddDaily(DQ_DayAvg, DQ_HourLow);
				}
			}
			else
				PartialLogDay = 0;

			DQ_DayAvg = DQ_HourAvg;
			DQ_HourLow = DQ_HourAvg;
			DQ_DayAvgcnt = 1;
			LastLogDay = tmpdate.tm_mday;
		}
		else
		{
			if (DQ_HourAvg < DQ_HourLow)
				DQ_HourLow = DQ_HourAvg;
			DQ_DayAvg += DQ_HourAvg;
			DQ_DayAvgcnt++;
		}
		DQ_HourAvgCnt = 1;
		DQ_HourAvg = qratio;
		LastLogHour = tmpdate.tm_hour;
	}
	else
	{
		DQ_HourAvgCnt++;
		DQ_HourAvg += qratio;
	}
}
#endif



#if 0
static int8_t log_record_to_flash()
{
	// zg
	// Note that a log record is 32 bytes, which is an exact multiple of a sector size (either 4K or 64K).
	// Therefore, a log record never spans a sector boundary.
	// Since a sector is the smallest unit that can be erased, it is sufficient to simply
	// read the current 32 bytes in the flash to detect if the segment is ready for writing.
	// One might be tempted to only check the beginning of the page rather than each 32 byte segment.
	// Checking only the beginning would work if no power interruption occurred on previous cycles
	// and the tracking data is in sync with the log data.
	// Checking each segment would give an indication that something is wrong.
	// One possible action would be to "repair" the sector by copying good data into a new sector,
	// erasing the damaged sector, then putting the good data back and continuing.
	// It is unclear if such a methodology is warranted for this application...

}
#endif

#if 0
int8_t log_write_recalc_init(uint32_t time)
{
	//time_t tmpdate;
	struct tm tmpdate;
	//log_t tmplog;
	PartialLogDay = 1;
	//struct tm *tm_val;
	//RTC_HAL_ConvertSecsToDatetime(&CurrentLog.datetime,&tmpdate);
	//tm_val = gmtime(&tmpdate);
	sec_to_dt(&CurrentLog.datetime,&tmpdate);
	LastLogHour = tmpdate.tm_hour;
	LastLogDay = tmpdate.tm_mday;
	DQ_HourAvg = 0;
	DQ_HourAvgCnt = 0;
	DQ_DayAvg = 0;
	DQ_HourLow = 0;
	DQ_DayAvgcnt = 0;

	log_write(CurrentLog.datetime);
	return 0;
}
#endif

#if 0
static void update_log_start(void)
{
	// 5-28-2022 zg
	// The first production run of iTracker5 units had bad RTC backup batteries on the boards
	// and they could not be replaced. Consequently, when these units are power cycled, the
	// date always comes up as 1/1/2018 because the RTC loses power when the main batteries are disconnected.
	// The App was changed to detect this and automatically sync the time if 1/1/2018 is detected.
	// As a further change, if the log is currently empty, there is a good chance that the
	// start date and last download date says 1/1/2018 due to the power-on sequence creating a file
	// from scratch long before the App connects.
	// So, if the log is empty go ahead and make it start as of now.
	// There is still the corner case of a /power on/sleep/data log cycle/ without a connect and
	// without a previous log entry to sync with.  In that case, 1/1/2018 is used
	// for the RTC time and is used as the timestamp for the first log entry, etc.

	if (LogTrack.log_counter == 0)
	{
		log_start(0);
	}
}
#endif



void log_write(uint32_t time, float degF, float distance, uint16_t gain, float vertical_mount, int write_log_track)
{


	LogTrack.log_counter++;
	LogTrack.datetime = time;

	// On the very first write is to an empty log file, synchronize the last download and start times to the first data point.
	// That keeps the LogTrack logically consistent.
	if (LogTrack.log_counter == 1)
	{
		LogTrack.last_download_datetime = time;
		LogTrack.start_datetime         = time;
	}


	CurrentLog.datetime = time;
	CurrentLog.log_num  = LogTrack.log_counter;
	CurrentLog.distance = distance; //LastGoodMeasure;
    CurrentLog.temperature = degF;
    //CurrentLog.spare2      = 0.0;
	CurrentLog.gain        = gain; // Actual_Gain;
    CurrentLog.accumulator = ltc2943_get_raw_accumulator(); // For diagnostics, this is either zero (ltc2943 not operating) or the raw value in the ltc2943.  May or may not be battery related.

	// Save a copy of the vertical mount in effect at the time the data point is taken
	CurrentLog.vertical_mount = vertical_mount; // MainConfig.vertical_mount;
	//CurrentLog.fill[0];
	//CurrentLog.fill[1];

	CurrentLog.checksum = 0;
	CurrentLog.checksum = GetChkSum((uint8_t *) &CurrentLog, LOG_SIZE);

#ifdef USE_READ_CACHE
	//Must be sure to invalidate the read cache in case it previously read past the end of data and would not therefore not have this new data
	//An invalid cache causes problems with the Alert feature which transmits a set of 12 previous readings once the last one triggers an alert.
    if ((cache_start_addr <= LogTrack.next_logrec_addr) && (LogTrack.next_logrec_addr <= cache_end_addr) )
    {
	  cache_start_addr=0;
	  cache_end_addr=0;
	  cache_len=0;
    }
#endif

	BSP_Log_Write((uint8_t *) &CurrentLog, LogTrack.next_logrec_addr, LOG_SIZE);

#ifdef	LOG_DOES_NOT_WRAP
#else
	if (LogTrack_first_logrec_addr == 0) // If no data record has yet been logged
	{
		LogTrack_first_logrec_addr = LOG_BOTTOM;  // keep track of where first log record starts  (will change when rollover occurs)
	}
#endif

	LogTrack.next_logrec_addr += LOG_SIZE;
	if (LogTrack.next_logrec_addr >= LOG_TOP)
	{
		LogTrack.next_logrec_addr        = LOG_BOTTOM;
		LogTrack.last_rollover_datetime = CurrentLog.datetime;
		LogTrack.last_rollover_record   = LogTrack.log_counter;
	}

	Hydro_NewDataLogged(); // Note the fact that new data is available for Hydrograph decomposition


	// zg - Mark requested that a backup copy be made to the SD card, if present and if it doesn't consume much power...
    if (write_log_track)
    {
	  BSP_Log_Track_Write(1);  // update battery accumulator value when requested
    }


	if (sd_FactoryTest)
	{
		if (CurrentLog.gain < FactoryTest.min_gain) FactoryTest.status = FACTORY_TEST_FAILED;
		if (CurrentLog.gain > FactoryTest.max_gain) FactoryTest.status = FACTORY_TEST_FAILED;

		if (CurrentLog.distance < FactoryTest.min_distance) FactoryTest.status = FACTORY_TEST_FAILED;
		if (CurrentLog.distance > FactoryTest.max_distance) FactoryTest.status = FACTORY_TEST_FAILED;
	}

}


// In 4.1.4047 the actual vertical mount in effect at the time a data point is taken is recorded in the log file.
// At the time the level is requested, the two vertical mounts might be different.  This is intended to allow
// for after-the-fact corrections of two types of errors:
//
//   1) Erratic "off-the-bench" installations where the original vertical mount is just plain wrong.
//      This typically results in negative level values being reported.  A manual "recalc" must be performed
//      by entering a new vertical mount value and performing a download.
//
//   2) Mistaken adjustments when a customer milks data in the middle of a study and does a "set level" when resuming
//      the log file.  Subsequent downloads of the entire data set will mis-apply the new vertical mount to the old data
//      thereby causing a slight shift of numbers.
//
// Basically, the only action the firmware can unilaterally make is to report the existence of two different VM values when producing a CSV file.
int log_has_different_vertical_mount(log_t * pLogRec)
{
	if (MainConfig.vertical_mount != pLogRec->vertical_mount)
		return 1;
	else
		return 0;
}

float  log_get_level(log_t * pLogRec)
{
#if 1
	float level;
	level = MainConfig.vertical_mount - pLogRec->distance;
	return level;
#else
	float level_1;
	float level_2;

	level_1 = MainConfig.vertical_mount - pLogRec->distance;

	level_2 = pLogRec->vertical_mount - pLogRec->distance;

	if (MainConfig.vertical_mount == pLogRec->vertical_mount) return level_1;

	//if (level_2 < 0)

	return level_1;
#endif
}

float  log_get_qratio(log_t * pLogRec)
{

	float level;

	// Compute level
	level = log_get_level(pLogRec);

	// This boundary was always used when computing qratio
	if (level < .1)
		level = .1;

    return Delta_Q(MainConfig.pipe_id, level);

}

void log_ResetLogTrack()
{
	uint32_t current_time;
	current_time = rtc_get_time();
	LogTrack.start_datetime         = current_time;
	LogTrack.next_logrec_addr       = LOG_BOTTOM;
	LogTrack.log_counter             = 0;
	LogTrack.datetime               = current_time;
	LogTrack.last_download_ptr      = LOG_BOTTOM;
	LogTrack.last_download_datetime = current_time;
	LogTrack.unused_1               = 0;
	LogTrack.last_rollover_datetime = 0;
	LogTrack.last_rollover_record   = 0;
	LogTrack.next_cellsend_addr      = LOG_BOTTOM;
	LogTrack.last_cellsend_record   = 0;
	LogTrack.last_sector_erased     = LOG_BOTTOM >> 12;

	LogTrack.write_times            = 0;

#ifdef	LOG_DOES_NOT_WRAP
#else
	LogTrack_first_logrec_addr      = 0;
#endif

	ltc2943_set_battery_accumulator(MAX_BATTERY);
}

#if 0 // zg - see comment below
static uint8_t isLogTrackChecksumOk()
{
	uint16_t sd_card_checksum;
	uint16_t expected_checksum;
	sd_card_checksum = LogTrack.checksum;
	LogTrack.checksum = 0;
	expected_checksum = GetChkSum((uint8_t *) &LogTrack, sizeof(LogTrack));
	if (sd_card_checksum != expected_checksum) return 0;
	return 1;
}
#endif


static int get_last_log_entry(log_t * pLog)
{

	uint32_t addr_of_last_entry;

	if (LogTrack.log_counter == 0) return 0;  // no log entries

	// The log has at least one record.   Find it.
	if (LogTrack.next_logrec_addr > LOG_BOTTOM)
	{
      addr_of_last_entry = LogTrack.next_logrec_addr - LOG_SIZE;
	}
	else
	{
		addr_of_last_entry = LOG_BOTTOM;
	}

	log_readstream_ptr(pLog, addr_of_last_entry);

	return 1;

}

void log_sync_rtc_from_last_entry(void)
{
	log_t tmplog;

	if (get_last_log_entry(&tmplog))
	{
		uint32_t dt;

		dt = tmplog.datetime;  // make this a known distance away?

		rtc_sync_time_from_ptime(&dt);
	}

}

#if 0
void log_sync_accumulator_from_last_entry(void)
{
	log_t tmplog;

	if (get_last_log_entry(&tmplog))
	{
		ltc2943_set_raw_accumulator(tmplog.accumulator);
	}
}
#endif


void log_init(void)
{

	int update_log_track = 0;

	if (BSP_QSPI_Read((uint8_t *) &LogTrack, LOG_TRACK, sizeof(LogTrack)) != QSPI_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}
	else
	{
		// Detect empty flash
		if (isErased((uint8_t *)&LogTrack, sizeof(LogTrack)))
		{
			log_ResetLogTrack();
		}
	}
#if 0
	// zg - although the checksum code is present on 4.0.13 and later, it was never used.
	// The best course of action would be to attempt a repair by scanning and repairing flash.
	// Not now...
	if (!isLogTrackChecksumOk())
	{
		// Attempt repair ???  or just initialize ???
		_Error_Handler(__FILE__, __LINE__);
	}
#endif

	// Find the first log entry
#ifdef	LOG_DOES_NOT_WRAP
#else

	LogTrack_first_logrec_addr = log_FindFirstEntry();  // can be zero if no log entries at all
#endif

	if (LogTrack.log_counter == 0)
	{
		CurrentLog.log_num = 0;   //There are no logs at all
		return;
	}


	// The log has at least one record.   Attempt to sync LogTrack timestamps to the first recorded log entry (at LOG_BOTTOM)
	// Also, sync the CurrentLog to the last recorded log

	log_readstream_ptr(&CurrentLog, LOG_BOTTOM);


	// Correct and save any illogical LogTrack header information, relative to the first recorded log entry.
    #define SEC_IN_24_HOURS 86400

	if (LogTrack.start_datetime > CurrentLog.datetime) update_log_track = 1;

	if (LogTrack.last_download_datetime > CurrentLog.datetime) update_log_track = 1;

	if (LogTrack.start_datetime < (CurrentLog.datetime - SEC_IN_24_HOURS)) update_log_track = 1;

	if (update_log_track)
	{
		LogTrack.start_datetime         = CurrentLog.datetime;
		LogTrack.datetime               = CurrentLog.datetime;
		LogTrack.last_download_datetime = CurrentLog.datetime;
		LogTrack.last_rollover_datetime = 0;
		LogTrack.last_rollover_record   = 0;

		BSP_Log_Track_Write(0);  // Do not update battery accumulator, leave it alone.
	}
}


void log_duplicate_last_entry(void)
{
	log_t tmplog;
	if (get_last_log_entry(&tmplog))
	{
		// duplicate the last recorded log entry
		log_write(tmplog.datetime, tmplog.temperature, tmplog.distance, tmplog.gain, tmplog.vertical_mount, LOG_WRITE_LOG_TRACK);
	}
}




int8_t log_start(uint32_t time)
{
	int8_t status = 0;
	uint32_t current_time;

	if (time)
	{
		current_time = time;
	}
	else
	{
		current_time = rtc_get_time();
	}

	LogTrack.next_logrec_addr       = LOG_BOTTOM;
	LogTrack.last_download_ptr      = LOG_BOTTOM;
	LogTrack.last_download_datetime = current_time;
	LogTrack.start_datetime         = current_time;
	LogTrack.datetime               = current_time;
	LogTrack.last_cellsend_record   = 0;
	LogTrack.next_cellsend_addr     = LOG_BOTTOM;
	LogTrack.unused_1               = 0;
	LogTrack.log_counter            = 0;
	if (LogTrack.write_times == 0xffff)
		LogTrack.write_times = 0;
	LogTrack.last_rollover_datetime = 0;
	LogTrack.last_rollover_record   = 0;
	LogTrack.last_sector_erased     = LOG_BOTTOM >> 12;

#ifdef	LOG_DOES_NOT_WRAP
#else
	LogTrack_first_logrec_addr      = 0;
#endif

	status = BSP_QSPI_Erase_Sector(LOG_BOTTOM);

	BSP_Log_Track_Write(1);  // Ok to also update battery accumulator

	Hydro_NewDataLogged(); // invalidate hydrograph decomposition

	// invalidate alarms
	s2c_current_alarm_state = NO_ALARM;
	s2c_last_alarm_state_sent =  NO_ALARM;
	s2c_pAlertLogRec = NULL;

	return status;
}

int8_t log_set_last_download(void)
{
	LogTrack.last_download_datetime = rtc_get_time();
	LogTrack.last_download_ptr      = LogTrack.next_logrec_addr;  // zg I don't think this makes sense...
	BSP_Log_Track_Write(1);  // Ok to also update battery accumulator here
	return 0;
}



int8_t log_readstream_ptr(log_t * pLog, uint32_t ReadAddr)
{

	int8_t stat;
#if 0
	stat = BSP_QSPI_Read((uint8_t *)pLog, ReadAddr, LOG_SIZE);
#else
	stat = 0;
	log_GetRecord(pLog, ReadAddr, LOG_FORWARD);
#endif
	return stat;
}



#ifdef USE_READ_CACHE
static void copyCachedRecord(log_t * pLog, uint32_t ReadAddr)
{
    uint8_t * p;
    int i, j, start, stop;

    p = (uint8_t *)pLog;

	// Calculate the position of the requested record in the cache
    start = (ReadAddr - cache_start_addr);
    stop = start + sizeof(log_t);
    j = 0;
	for (i = start; i < stop; i++)
	{
	  p[j] = cached_bytes[i];
	  j++;
	}
}

void log_GetRecord(log_t * pLog, uint32_t ReadAddr, uint8_t Direction)
{

	// Note:  it is assumed:
    // 1. ReadAddr is always a multiple of sizeof(log_t) starting at LOG_BOTTOM
    // 2. that LOG_BOTTOM <= ReadAddr < LOG_TOP

	// Check if the cache has anything in it
	if (cache_len > 0)
	{
		// Check if ReadAddr is already in the cache
		if ((cache_start_addr <= ReadAddr) && (ReadAddr <= cache_end_addr) )
		{
#if 0
			if (ReadAddr == 2188256)
			{
				int i;
				for (i=0; i < 10 ; i++);   // for debugging set a breakpoint here
			}
#endif
			return copyCachedRecord(pLog, ReadAddr);
		}
	}

	// Cache miss, reload cache using CacheDirection as a guide
	if (Direction == LOG_FORWARD)
	{
		cache_start_addr = ReadAddr;
		cache_len = LOG_TOP - cache_start_addr;
		if (cache_len > CACHE_SIZE)  cache_len = CACHE_SIZE;
	}
	else
	{
		cache_start_addr = ReadAddr + sizeof(log_t) - CACHE_SIZE;
		if (cache_start_addr < LOG_BOTTOM) cache_start_addr = LOG_BOTTOM;
		cache_len = CACHE_SIZE;
	}
	cache_end_addr = cache_start_addr + cache_len - 1;

	// Fill the cache
	BSP_QSPI_Read(cached_bytes, cache_start_addr, cache_len);

#if 0
	if (ReadAddr == 0x202be0)
	{
		static int debug = 0;
		debug = 1;
	}
#endif

	// Return the requested record
	copyCachedRecord(pLog, ReadAddr);

}

#else


void log_GetRecord(log_t * pLog, uint32_t ReadAddr, uint8_t Direction)
{

  BSP_QSPI_Read((uint8_t *)pLog, ReadAddr, LOG_SIZE);
}

#endif


#if 1

static uint32_t enum_last_addr;
static uint32_t enum_current_addr;
static uint8_t  enum_direction;

void log_EnumStart(uint32_t StartAddr, uint8_t Direction)
{
	enum_current_addr = StartAddr;
	enum_direction = Direction;
}

uint32_t log_EnumGetLastAddr(void)
{
	return enum_last_addr;
}


uint8_t log_EnumGetNextRecord(log_t * pLogRec, uint32_t max_addr)
{
	if (enum_current_addr == 0) return 0;

#ifdef	LOG_DOES_NOT_WRAP
#else
	// Check if any log records at all
	if (LogTrack_first_logrec_addr == 0)
	{
		enum_current_addr = 0;
		return 0;
	}
#endif

	if (enum_current_addr == max_addr)
	{
		enum_current_addr = 0;
		return 0;
	}

	log_GetRecord(pLogRec, enum_current_addr, enum_direction);
	enum_last_addr = enum_current_addr;

	// Advance to next record
	if (enum_direction == LOG_FORWARD)
	{
		enum_current_addr += LOG_SIZE;

		// Check for rollover
		if (enum_current_addr >= LOG_TOP)
		{
#ifdef	LOG_DOES_NOT_WRAP
			enum_current_addr = 0;  // no more records, stop enum
#else
			enum_current_addr = LOG_BOTTOM;
#endif
		}
		// Check if last record
		if (enum_current_addr >= max_addr)
		{
			enum_current_addr = 0;  // no more records, stop enum
		}
	}
	else
	{
		enum_current_addr -= LOG_SIZE;

#ifdef	LOG_DOES_NOT_WRAP
		if (enum_current_addr < LOG_BOTTOM)
		{
			enum_current_addr = 0;  // no more records, stop enum
		}
#else

		// Check for rollover
		if (enum_current_addr < LOG_BOTTOM)
		{
			enum_current_addr = LOG_TOP - LOG_SIZE;
		}

		// Check if last record
		if (enum_current_addr < LogTrack_first_logrec_addr)
		{
			enum_current_addr = 0;  // no more records, stop enum
		}
#endif
	}

	return 1;
}

void log_EnumStop(void)
{
	enum_current_addr = 0;
}

uint32_t log_FindFirstEntry(void)
{
#ifdef LOG_DOES_NOT_WRAP
	return LOG_BOTTOM;
#else
	log_t log_rec;

	// Check if wrap has occurred by examining the last possible data record of flash
	log_GetRecord(&log_rec, LAST_POSSIBLE, LOG_FORWARD);

	// Detect empty flash
	if (isErased((uint8_t *)&log_rec, sizeof(log_rec)))
	{
		// no wrap has occurred, first entry might at beginning, go check
		log_GetRecord(&log_rec, LOG_BOTTOM, LOG_FORWARD);

		if (isErased((uint8_t *)&log_rec, sizeof(log_rec)))
		{
			return 0; // no log records at all
		}
		// Ok, the first record is at the bottom
		return LOG_BOTTOM;
	}

	// wrap occurred, but has a new record been written yet?
	if (LogTrack.next_logrec_addr == LOG_BOTTOM)
	{
		return LAST_POSSIBLE;
	}

	// wrap occurred, and at least one record was written,
	// which means a page was erased, so skip over that part
	// Note: this could potentially be a search, eliminating
	// dependency on next_logrec_addr, but for now, assume
	// next_logrec_addr is viable, like the reset of the program.
	return log_Get4KPageAddress(LogTrack.next_logrec_addr) + SECTOR_SIZE;
#endif

}

uint32_t log_FindLastEntry(void)
{
	uint32_t addr;
	addr = LogTrack.next_logrec_addr - LOG_SIZE;
    if (addr < LOG_BOTTOM) addr = 0;
    return addr; // Note: can be zero if no log records
}
#if 0
static int is_log_record_ok(log_t *p)
{
	if (p->accumulator)
	{

	}
	if (p->checksum)
	{

	}
	if (p->datetime)
	{

	}
	if (p->distance)
	{

	}

	if (p->gain)
	{

	}
	if (p->log_num)
	{

	}
	if (p->temperature)
	{

	}
	if (p->vertical_mount)
	{

	}
	return 1;
}
#endif
static void create_file_name(char * path, char *ext)
{
	int i;
	FRESULT fr;
	FIL fstatus;

	for (i = 1; i < 9999; i++)
	{
		sprintf(path,"FactoryExtract%04d.%s", i, ext);

		if (sd_init() != FR_OK) return;  // nothing to do (e.g. no SD card present)

		fr = f_open(&fstatus, path, FA_READ);
		if (fr == FR_OK)
		{
			// file name is in use, close and generate another
			f_close(&fstatus);
		}
		else
		{
			// file name is not in use
			return;
		}
	}

	// Should never get here
	sprintf(path,"FactoryExtract0000.%s", ext);
}

extern void prep_uart2_buffer(log_t * p, char * rtc_alert, char * flume_str, uint8_t report_accumulator);
extern void __writeDownload(uint32_t DL_Type, FIL * sdFileName);
#include "uart2.h"

//int log_ScanDataRecords(int RepairLogTrack)
int log_ExtractDataRecords(void)
{

	log_t log_rec;
	char null_str[1];

	null_str[0]=0;

	// SD Variables.
	FIL sdFileName;
	FRESULT sd_res;

	char path[_MAX_LFN + 1];
	int prev_pattern;

	sd_res = sd_init();  // Initialize the SD card and file system
	if (sd_res != FR_OK) return sd_res;

	create_file_name(path, "txt");  // FactoryExtract0001.txt etc  number increments with each execution so as to not overwrite


	// Open the file for writing
	sd_res = f_open(&sdFileName, path, FA_WRITE | FA_CREATE_ALWAYS);
	if (sd_res != FR_OK) return sd_res;

	sprintf((char *)uart2_outbuf, "Factory Extract log records for SN: %s   MAC %s\r", MainConfig.serial_number, MainConfig.MAC);
	__writeDownload(DL_TYPE_SDCARD, &sdFileName);

#if 0
	if (RepairLogTrack)
	{
		LogTrack.log_counter = 0;
		LogTrack.next_logrec_addr = LOG_BOTTOM;
	}
#endif

	log_EnumStart(LOG_BOTTOM, LOG_FORWARD);

    prev_pattern = led_set_pattern(LED_PATTERN_FAST_YEL);

	while (log_EnumGetNextRecord(&log_rec, LOG_TOP))
	{
		// Examine log record to determine if it is erased flash
		if (isErased((uint8_t *)&log_rec, sizeof(log_rec)))
		{
			// keep scanning
		}
		else
		{
			// Assume the log record is ok
#if 0
			if (RepairLogTrack)
			{

				LogTrack.datetime = log_rec.datetime;

				LogTrack.log_counter++;
				// On the very first write is to an empty log file, synchronize the last download and start times to the first data point.
				// That keeps the LogTrack logically consistent.
				if (LogTrack.log_counter == 1)
				{
					LogTrack.last_download_datetime = log_rec.datetime;
					LogTrack.start_datetime         = log_rec.datetime;
				}

				LogTrack.next_logrec_addr += LOG_SIZE;
			}
#endif



			prep_uart2_buffer(&log_rec, null_str, null_str, 1);

			__writeDownload(DL_TYPE_SDCARD, &sdFileName);


			//process_log_record(&log_rec);
		}

		led_heartbeat();
	}

#if 0
	if (RepairLogTrack)  BSP_Log_Track_Write(1);  // Ok to also update battery accumulator
#endif

	f_close(&sdFileName);

    led_set_pattern(prev_pattern);

	return FR_OK;

}



uint32_t log_FindPreviousEntry(uint32_t Days)  // Note: can return zero if no entries
{

    uint32_t addr;
    uint32_t prev_addr;
    uint32_t time_anchor;
    log_t    logrec;


    addr = log_FindLastEntry();
    if (addr == 0) return 0;  // there are no log entries at all
    prev_addr = addr;

    // Get the last data record
    log_GetRecord(&logrec, addr, LOG_REVERSE);

    time_anchor = logrec.datetime;  // set the initial anchor

#if 1 // snap to just past midnight for a complete day - zg this was ADDED for Hydrograph Decomp
    struct tm date_anchor;
    sec_to_dt(&logrec.datetime, &date_anchor);
	date_anchor.tm_hour  = 0;
	date_anchor.tm_min = 0;
	date_anchor.tm_sec = 0;
	dt_to_sec(&date_anchor, &time_anchor);
#endif


    // Compute a time_anchor using last logged data record or the current date/time (problematic)
    //time_anchor = rtc_get_time() - (Days * 86400);	// 86400 seconds per day
    time_anchor = time_anchor - (Days * 86400);	// 86400 seconds per day


    // Search backwards through the log records for a timestamp that precedes the requested start time
    log_EnumStart(addr, LOG_REVERSE);
    while (log_EnumGetNextRecord(&logrec, LogTrack.next_logrec_addr))
    {

        prev_addr = addr;  // save previous address
    	addr = log_EnumGetLastAddr();  // grab address of current record

    	// test if the time_anchor was passed
        if (logrec.datetime < time_anchor)
        {
        	// go back to previous address and exit
        	addr = prev_addr;
        	log_EnumStop();
        }
        led_heartbeat();
    }

	return addr;  // This is the record closest to the time_anchor (we ran out of log records)
}

#endif

uint32_t log_FindRecAddr(uint32_t RecNo)  // returns address of log record, or zero if not found
{

    uint32_t addr;

    if (RecNo == 0) return 0;

    addr = LOG_BOTTOM + ((RecNo - 1) * sizeof(log_t));

    if (addr > log_FindLastEntry()) return 0;

    return addr;

}
#if 0
uint32_t log_GetFirstTimestamp()
{

	if (LogTrack.log_counter)
	{
		log_t log_rec;

		log_readstream_ptr(&log_rec, LOG_BOTTOM);

		return log_rec.datetime;
	}
	return 0;
}
#endif
