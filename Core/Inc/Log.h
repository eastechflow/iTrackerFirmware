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

#include "stdint.h"

#ifndef INC_LOG_H_
#define INC_LOG_H_

#define LOG_DOES_NOT_WRAP

#define SECTOR_SIZE        	0x1000      // 4096 bytes
#define LOG_BOTTOM		   	0x200000
#define LOG_TOP			   	0xFFFFFF
#define LOG_SIZE			32
#define LAST_POSSIBLE       (LOG_TOP - sizeof(log_t))
// maximum possible log entries is 458,751

#define LOG_TRACK           0x1000
#define DQ_VALUES           0x2000
//#define CELL_BUFF         0x3000
#define CELL_CONFIG_STORE  	0x4000
#define MAIN_CONFIG_STORE   0x5000
#define SENSOR_CONFIG_STORE 0x6000
#define RECALC_TEMP_STORE  	0x7000
//#define TEMP_BUFF         0x10000

#define log_Get4KPageAddress(x)  ((x) & 0xFFF000)
#define log_IsStartOf4KPage(x)  ((x) == ((x) & 0xFFF000))
#define log_IsStartOf64KPage(x) ((x) == ((x) & 0xFF0000))




typedef struct {
	uint32_t start_datetime; 			// 4 bytes
	uint32_t next_logrec_addr;          // previously current_log_ptr
	uint32_t log_counter; 				// 4 + 4 = 8 bytes
	uint32_t datetime; 					// 4 bytes
	uint32_t last_download_ptr;
	uint32_t last_download_datetime;	// 4 bytes
	uint32_t unused_1;           		// 4 bytes  first_logrec_addr
	uint32_t last_rollover_datetime; 	// 4 bytes
	uint32_t last_rollover_record;
	uint32_t next_cellsend_addr;
	uint32_t last_cellsend_record;
	uint32_t last_sector_erased;
	float    accumulator;
	uint16_t write_times;    	//  fill for expansion
	uint16_t checksum;   		// + 1
} log_track_t;

#if 0 // This was the structure before 4.1.4045
typedef struct {
	uint32_t datetime; 	// 4 bytes
	uint32_t log_num; 	// 8 bytes
	float    distance; 	// 12 bytes  zg - note that the VerticalMount or level is not saved per data point.  If the MainConfig.VerticalMount is later changed, all prior data is invalid, unless qratio can be unwound.
	float    temp;   	// 16 byte
	float    qratio;	// 20 bytes
	uint16_t gain;		// 22 bytes
	int16_t  log_status; // 24 bytes
	char     fill[6];   // fill for expansion
	uint16_t checksum;  // + 2  (26 + 6 = 32)
} log_t;
#endif
#if 0
// 4.1.4045, 4.1.4046
typedef struct {
	uint32_t datetime; 	// 4 bytes
	uint32_t log_num; 	// 8 bytes
	float    distance; 	// 12 bytes
	float    temp;   	// 16 byte
	float    qratio;	// 20 bytes
	uint16_t gain;		// 22 bytes
	int16_t  spare;     // 24 bytes previously log_status; // 24 bytes
	float    level;     // 28
	char     fill[2];   // 30 fill for expansion
	uint16_t checksum;  // 32  not really used... + 2  (26 + 6 = 32)
} log_t;  // This is 32 bytes long
#endif
#if 0
// 4.1.4047
typedef struct {
	uint32_t datetime;
	uint32_t log_num;
	float    distance;
	float    temperature;
	float    spare2;	// previously qratio
	uint16_t gain;		//
	int16_t  spare;     // previously log_status; // 24 bytes
	float    vertical_mount;     // previously zero, or level in 4.1.4045 and 4.1.4046
	char     fill[2];   // fill for expansion
	uint16_t checksum;  // not really used... + 2  (26 + 6 = 32)
} log_t;  // This is 32 bytes long
#endif
#if 1
// 5.0.82
typedef struct {
	uint32_t datetime;
	uint32_t log_num;
	float    distance;
	float    temperature;
	float    spare2;	// previously qratio
	uint16_t gain;		//
	uint16_t accumulator;     // battery accumulator
	float    vertical_mount;     // previously zero, or level in 4.1.4045 and 4.1.4046
	char     fill[2];   // fill for expansion
	uint16_t checksum;  // not really used... + 2  (26 + 6 = 32)
} log_t;  // This is 32 bytes long
#endif

extern log_t       CurrentLog;
extern log_track_t LogTrack;

#ifdef LOG_DOES_NOT_WRAP
#else
extern uint32_t    LogTrack_first_logrec_addr;  // zg To preserve backwards compatibility with existing logs, had to move this out of the log_track_t structure
#endif

extern uint8_t LastLogHour;
extern uint8_t LastLogDay;

//#define NO_CELLULAR    0
//#define ALLOW_CELLULAR 1


void   log_init(void);
void   log_duplicate_last_entry(void);
void   log_ResetLogTrack();
int8_t log_write_recalc_init(uint32_t time);

#define LOG_DO_NOT_WRITE_LOG_TRACK  0
#define LOG_WRITE_LOG_TRACK         1

void   log_write(uint32_t time, float degF, float distance, uint16_t gain, float vertical_mount,int batt_info);
int8_t log_readstream_ptr(log_t * pLog, uint32_t ReadAddr);
int8_t log_set_last_download(void);
int8_t log_start(uint32_t time);

float  log_get_level(log_t * pLog);
float  log_get_qratio(log_t * pLog);
int    log_has_different_vertical_mount(log_t * pLogRec);


void log_GetRecord(log_t * pLog, uint32_t ReadAddr, uint8_t Direction);

#define LOG_FORWARD  1
#define LOG_REVERSE  2

void     log_EnumStart(uint32_t StartAddr, uint8_t Direction);
uint8_t  log_EnumGetNextRecord(log_t * pLogRec, uint32_t max_addr);
uint32_t log_EnumGetLastAddr(void);
void     log_EnumStop(void);
uint32_t log_FindFirstEntry(void);
uint32_t log_FindLastEntry(void);
uint32_t log_FindLastDownloadEntry(void); // Note: can return zero if no entries
uint32_t log_FindPreviousEntry(uint32_t Days);  // Note: can return zero if no entries
uint32_t log_FindRecAddr(uint32_t RecNo);  // returns address of log record, or zero if not found
uint32_t log_GetFirstTimestamp(void);
void     log_sync_rtc_from_last_entry(void);
//void     log_sync_accumulator_from_last_entry(void);

int log_ExtractDataRecords(void);

#endif
