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

#include "debug_helper.h"
#include "stm32l4xx_hal.h"
#include "Common.h"
#include "WiFi.h"
#include "WiFi_Socket.h"
#include "fatfs.h" // SD Card
#include "sdmmc.h" // SD Card
#include "string.h"
#include "stdio.h"
#include "LTC2943.h"
#include "Log.h"
#include "Sensor.h"
#include "version.h"
#include "DeltaQ.h"
#include "Sample.h"
#include "stdlib.h"
#include "time.h"
#include "stdbool.h"
#include "usart.h"
#include "uart1.h"
#include "uart2.h"
#include "uart4.h"
#include "lpuart1.h"
#include "rtc.h"
//#include <ctype.h>
#include "stm32l476xx.h"
#include "quadspi.h"
#include "history.h"
#include "s2c.h"

#include "trace.h"
#include "factory.h"
#include "flume.h"

uint8_t zg_debug = 0;



// Control variables.
uint8_t  json_complete = 0;  // A complete JSON string has been received.
uint16_t json_cmd_id = 0;
uint16_t json_sub_cmd_id = 0;
char     json_cloud_packet_type = 0;
uint32_t json_cloud_record_number = 0;

uint8_t SleepDelayInMinutes = 0;

uint8_t wifi_report_seconds = 0;  // don't report logged timestamp seconds unless asked

//char DebugStringFromUI[40];
//char DebugStringToSendBack[WIFI_BUF_SIZE];

// WIFI User action processing in the websocket.
#define JSON_NAME_SIZE  50
#define JSON_VALUE_SIZE 50
char json_name[JSON_NAME_SIZE + 1];
char json_value[JSON_VALUE_SIZE + 1];
uint8_t json_name_cnt = 0;
uint8_t json_value_cnt = 0;
uint8_t json_step = 0;
uint8_t json_type = 0;
uint8_t json_inString = 0;

#define JSON_INCOMPLETE  0
#define JSON_COMPLETE    1



uint8_t CurrentGraphType = 0;
uint8_t CurrentGraphDays = 0;
uint8_t newSite = 0;
uint32_t Download_startTime = 0;
uint32_t Download_type = 0; // Disk or SD Card
int saveMainConfig = 0;
int saveCellConfig = 0;
int saveSensorConfig = 0;

// PRIVATE members

void parse_datetime(char * strdate, struct tm * tm_st);


// ASSERT:  only null-terminated strings are passed to this function
// ASSERT:  size on entry is identical to strlen(buf)
static int8_t strChkNum(char * buf)
{
	int8_t stat = 0;
	uint8_t dec = 0;
	uint8_t size;
	size = strlen(buf);

	//buf[size] = 0;

	// note:  leading whitespace cannot be present

	for (int i = 0; i < size; i++)
	{
		if ((buf[i] < '0') || (buf[i] > '9'))  // examine all non-digit characters
		{
			if (!dec && (buf[i] == '.'))   // one decimal point is ok
			{
				dec = 1;
			}
			else
			{
				if (buf[i] != 0)
				{
					stat = -1;
				}
				else if (i == 0)  // must begin with a digit
				{
					stat = -1;
				}
				buf[i] = 0;   // null-terminate at first non-digit character and exit
				break;
			}
		}
	}

	return stat;
}




// User clicked "live"
void wifi_send_live(void)
{
	//char txBuff[WIFI_BUF_SIZE];


	int dec1, dec2, dec3, dec4, dec5, dec6, dec7, dec9, dec10 = 0;
	float tmp_pipe_id;
	float tmpLevel = 0.f;
	struct tm tm_st;
	int reported_gain;

	rtc_get_datetime(&tm_st);

	tmpLevel = MainConfig.vertical_mount - LastGoodMeasure;      // Note use of global variables

	//2-2-2023 Allow negative readings to be reported to app
	if (tmpLevel < 0) {
		tmpLevel = 0.0;   // zg - note this differs from logging a data point where 0.1 is used...
	}

	// Pipe ID
	tmp_pipe_id = MainConfig.pipe_id;


	// Battery
	dec7 = ltc2943_get_battery_percentage();


	// Unit conversion.
	if (MainConfig.units == UNITS_ENGLISH)
	{
		// Level
		convfloatstr_2dec(tmpLevel, &dec1, &dec2);

		// Pipe ID
		convfloatstr_2dec(tmp_pipe_id, &dec3, &dec4);

		// Temp DegF
		convfloatstr_1dec(LastGoodDegF, &dec5, &dec6);  // Note use of global variable

		// Distance
		convfloatstr_2dec(LastGoodMeasure, &dec9, &dec10);  // Note use of global variable

	}
	else
	{
		float DegC;

		// Level
		tmpLevel *= 25.4;
		convfloatstr_2dec(tmpLevel, &dec1, &dec2);

		//Pipe ID
		tmp_pipe_id *= 25.4;
		convfloatstr_2dec(tmp_pipe_id, &dec3, &dec4);

		// Temp DegC
		DegC = (LastGoodDegF - 32.0) * 5.0 / 9.0;    // Note use of global variable
		convfloatstr_1dec(DegC, &dec5, &dec6);

		// Distance
		convfloatstr_2dec(LastGoodMeasure * 25.4, &dec9, &dec10);  // Note use of global variable
	}

	if (isGen5())
	{
	  reported_gain = Actual_Gain;
	}
	else if (isGen4())
	{
	  reported_gain = (255-(int)Actual_Gain);
	}


	// Build and send the message.
	if (MainConfig.units == UNITS_ENGLISH) {
		sprintf((char *)uart2_outbuf,
		    "{\"Site_Name\":\"%s\",\"level\":\"%d.%02d in\",\"Pipe_ID\":%d.%02d,\"temp\":\"%d.%d F\",\"gain\":%d,\"blanking\":%lu,\"capturecnt\":%d,\"battery\":%d,\"Log_DateTime\":\"%02d/%02d/%02d %02d:%02d:%02d\",\"log_results\":%lu,\"distance\":\"%d.%02d in\"}",
		    MainConfig.site_name, 	// Site Name
		    dec1, dec2, 			// Level
		    dec3, dec4, 			// Pipe ID
		    dec5, dec6, 			// Temp
			reported_gain,		    // Gain
		    Actual_Blanking,	    // Blanking
		    (int)PulseCnt,          // Pulses captured
		    dec7, //dec8, 		 	// Battery
		    tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year, tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec,	// Log DateTime
		    LogTrack.log_counter,			// Log Results
		    dec9, dec10);			// Distance
	} else {
		sprintf((char *)uart2_outbuf,
		    "{\"Site_Name\":\"%s\",\"level\":\"%d.%02d mm\",\"Pipe_ID\":%d.%02d,\"temp\":\"%d.%d C\",\"gain\":%d,\"blanking\":%lu,\"capturecnt\":%d,\"battery\":%d,\"Log_DateTime\":\"%02d/%02d/%02d %02d:%02d:%02d\",\"log_results\":%lu,\"distance\":\"%d.%02d mm\"}",
		    MainConfig.site_name, 	// Site Name
		    dec1, dec2, 			// Level
		    dec3, dec4, 			// Pipe ID
		    dec5, dec6, 			// Temp
			reported_gain,		    // Gain
		    Actual_Blanking,	    // Blanking
		    (int)PulseCnt,          // Pulses captured
		    dec7, //dec8, 		 	// Battery
		    tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year, tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec,	// Log DateTime
		    LogTrack.log_counter,			// Log Results
		    dec9, dec10);			// Distance
	}

	// Send the whole string.
	uart2_tx_outbuf();
}

// User clicked "logs"
void wifi_send_logs(void)
{
	//char txBuff[WIFI_BUF_SIZE];

	struct tm tm_st;
	struct tm tm_st2;

	sec_to_dt(&LogTrack.start_datetime, &tm_st);
	sec_to_dt(&LogTrack.last_download_datetime, &tm_st2);

	sprintf((char *)uart2_outbuf,
	    "{\"Site_Name\":\"%s\",\"startDate\":\"%02d/%02d/%d %02d:%02d\",\"lastDate\":\"%02d/%02d/%d %02d:%02d\",\"log_results\":%lu,\"logInterval\":%lu}",
	    MainConfig.site_name, tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year, tm_st.tm_hour, tm_st.tm_min, tm_st2.tm_mon, tm_st2.tm_mday, tm_st2.tm_year,
	    tm_st2.tm_hour, tm_st2.tm_min, LogTrack.log_counter, (MainConfig.log_interval / 60));
	uart2_tx_outbuf();
}


void __writeDownload(uint32_t DL_Type, FIL * sdFileName)
{
	// FILE OR SD CARD?
	if (DL_Type == DL_TYPE_WIFI)
	{
		uart2_tx_outbuf();
	}
	else if (DL_Type == DL_TYPE_SDCARD)
	{
		if (BSP_SD_IsDetected() == SD_PRESENT)  // zg - not sure if this needs to be here.  If the file write was started, it will finish pretty fast...
		{
			UINT sdCardBytes;
			f_write(sdFileName, uart2_outbuf, strlen((char *)uart2_outbuf), &sdCardBytes);
		}
	}
}

#if 0
static void report_download_error(char *msg)
{
	//char txBuff[WIFI_BUF_SIZE];

	sprintf((char *)uart2_outbuf, "{\"error\":\"%s\"}", msg);
	uart2_tx_outbuf();
}
#endif

void wifi_send_download_error_code(int code)
{
	//char txBuff[WIFI_BUF_SIZE];

	sprintf((char *)uart2_outbuf, "{\"error\":\"%d\"}", code);
	uart2_tx_outbuf();
}

void wifi_send_trace_data(uint32_t DL_Type, FIL * sdFileName)
{
	sprintf((char *)uart2_outbuf, "%s", TraceBuffer);
	__writeDownload(DL_Type, sdFileName);

}


// User clicked "download diag"
int wifi_send_download_diag(uint32_t DL_Type)
{

	uint32_t tmpsec = 0;

	struct tm tm_st;

	// SD Variables.

	FIL sdFileName;

	FRESULT sd_res;


	// Initialize SD Card peripheral.
	// After initialization, mount the FAT volume
	// and then open the file.
	if (DL_Type == DL_TYPE_SDCARD)
	{
		// Initialize the SD card.
		if (BSP_SD_IsDetected() == SD_PRESENT)
		{

			char path[_MAX_LFN + 1];
			char site_name[20];

			sd_res = sd_init();
			if (sd_res != FR_OK) return sd_res;


			// Create file name.
			tmpsec = rtc_get_time();
			sec_to_dt(&tmpsec, &tm_st);

			// Note: If MainConfig.site_name has ????? don't use it
			strcpy(site_name, MainConfig.site_name);
			if (strcmp(site_name, "?????") == 0)
			{
				strcpy(site_name, "00000");
			}

			// LONG FILENAME (up to 255)
			sprintf(path, "DIAG_%s_%04d-%02d-%02d_%02d-%02d-%02d.csv",	site_name, tm_st.tm_year, tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec);



			// Open the file for writing
			sd_res = f_open(&sdFileName, path, FA_WRITE | FA_CREATE_ALWAYS);
			if (sd_res != FR_OK) return sd_res;
		}
	}

	// zg Hack
	//wifi_send_raw_data(DL_Type, &sdFileName);
	wifi_send_trace_data(DL_Type, &sdFileName);


	// Set last download date in settings.
	//log_set_last_download();

	// Close SD file handle if writing to SD card.
	if (DL_Type == DL_TYPE_SDCARD)
	{
		// Close the file handle. User can remove SD card now.
		f_close(&sdFileName);
	}

	return 0;
}

#if 1
// Original method of estimating battery life
static float estimate_battery_life_in_days(void)
{
	float tmp_battery;

	tmp_battery = MainConfig.val;

	// Estimate an average daily current use by looking at weekend and weekday values (if available)
	if ((MainConfig.we_current > 0) && (MainConfig.wd_current > 0))
	{
		float tmpfloat;
		tmpfloat = (MainConfig.we_current * 0.2857) + (MainConfig.wd_current * 0.7143);
		tmp_battery /= tmpfloat;   // calculate remaining battery life in days
	}
	return tmp_battery;
}

#else

static float estimate_battery_life_in_days(void)
{
  uint32_t seconds_since_battery_changed;
  uint32_t now;
  float battery_used_per_day;
  float remaining_battery;
  float estimated_life_in_days;

  now = rtc_get_time();
  if (now > MainConfig.battery_reset_datetime)
  {
    seconds_since_battery_changed =  now - MainConfig.battery_reset_datetime;
  }
  else
  {
	  // This only happens if the RTC was reset to a time preceding the last battery change.
	  // Highly unlikely, but need a positive number for estimate, which is arguably wrong anyway...
	  seconds_since_battery_changed =  MainConfig.battery_reset_datetime - now;
  }

  remaining_battery = LogTrack.accumulator; // ltc2943_get_accumulator();

  battery_used_per_day = (MAX_BATTERY - remaining_battery ) / ((float)(seconds_since_battery_changed/86400));

  // Alternatively, if no time has passed since last battery reset, need values determined from bench testing:
  // energy_per_datapoint (at 30" target)
  // data points per day (use currently configured log rate)
  // energy_per_cell_call (assume once per day, clean data connection)
  // seconds asleep per day (use this to calculate energy used during sleep)

  // estimated remaining life in days
  estimated_life_in_days = remaining_battery / battery_used_per_day;

  {
	  char msg[50];
	  int dec5, dec6 = 0;

	  convfloatstr_2dec(estimated_life_in_days, &dec5, &dec6);

	  sprintf(msg, "Estimated Battery Life %d.%d days", dec5, dec6);
	  AppendStatusMessage(msg);
  }

  return estimated_life_in_days;
}
#endif

static void remove_problematic_characters(char *site_name)
{
	// NUL \ / : * ? " < > |

	int i;

	// Kludge - force site name with all bad characters expect "a_b_c_d_e_f_g_h_i_" as output.
	// If add other chars in future, note site_name buffer is max of 20 characters for this test code.
	//strcpy(site_name, "a\\b/c:d*e?f\"g<h>i|");

	for (i=0; i < strlen(site_name); i++)
	{

		if (site_name[i] == '\\') site_name[i] = '_';
		if (site_name[i] == '/')  site_name[i] = '_';
		if (site_name[i] == ':')  site_name[i] = '_';
		if (site_name[i] == '*')  site_name[i] = '_';
		if (site_name[i] == '?')  site_name[i] = '_';
		if (site_name[i] == '\"') site_name[i] = '_';
		if (site_name[i] == '<')  site_name[i] = '_';
		if (site_name[i] == '>')  site_name[i] = '_';
		if (site_name[i] == '|')  site_name[i] = '_';
	}
}

#if 0
void flume_
{
	static double total;
// calculate and sum the volumetric flow between the two logged data points using SmartFlume calculations
// do not "average", just keep a running total

pInterval->average += flume_calc_gallons(pInterval->K, &(pInterval->prev_rec), pLogRec);
}
#endif

void prep_uart2_buffer(log_t * p, char * rtc_alert, char * flume_str, uint8_t report_seconds)
{
	int dec1, dec2, dec3, dec4, dec5, dec6 = 0;
	char batt_accum[20];  // either a null string, or the text "ttttttttt,aaaaa" where t is timestamp and a is raw accum
	char sec_str[5]; // either a null string, or the text ":nn", where nn is the seconds value of the timestamp
	uint16_t tmp16 = 0;
	float tmpfloat = 0.0f;
	struct tm tm_st;

	// Calculate all fields first.

	// Log date and time
	sec_to_dt(&p->datetime, &tm_st);

	// Level
	tmpfloat = log_get_level(p); //MainConfig.vertical_mount - p->distance;
	if (MainConfig.units == UNITS_METRIC) {
		tmpfloat *= 25.4;
	}
	convfloatstr_2dec(tmpfloat, &dec1, &dec2);

	// Distance
	tmpfloat = p->distance;
	if (MainConfig.units == UNITS_METRIC) {
		tmpfloat *= 25.4;
	}
	convfloatstr_2dec(tmpfloat, &dec3, &dec4);

	// Gain
	tmp16 = 255 - (p->gain & 0xff);

	// Temperature
	if (MainConfig.units == UNITS_ENGLISH) {
		tmpfloat = p->temperature;
	} else {
		tmpfloat = (p->temperature - 32) * 5 / 9;
	}
	convfloatstr_1dec(tmpfloat, &dec5, &dec6);

	// Battery accumulator
	sprintf(batt_accum, "%d", p->accumulator);
	if (report_seconds)
	{
		sprintf(sec_str, ":%02d", tm_st.tm_sec);
	}
	else
	{
		sec_str[0]=0; // do not report seconds
	}

	// Battery Current as mAhr
	//tmp_battery = ltc2943_get_mAh(p->accumulator);
	//convfloatstr_2dec(tmp_battery, &dec7, &dec8);

	// Build the string to send in one log entry.
	sprintf((char *)uart2_outbuf, "%lu,%02d/%02d/%04d %d:%02d%s,%d.%02d,%d.%02d,%d,%d.%d,%s,%s,%s\r",
		p->log_num, // Record Number
	    tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year, tm_st.tm_hour, tm_st.tm_min, sec_str, // Log date time, with optional seconds
	    dec1, dec2, // Level nn.nn
	    dec3, dec4, // Distance nn.nn
	    tmp16, 		// Gain nnn
	    dec5, dec6,	// Temp nn.n
		batt_accum, // Raw battery accumulator
		flume_str,  // Q, Total (if any)
		rtc_alert   // rtc_alert  (if any)
	    );
}


static float flume_K = 1.0;
static float flume_total = 0.0;
static float flume_Q1 = 0.0;
static float flume_Q2 = 0.0;

static void flume_start_total(void)
{
	flume_total = 0.0;
	flume_Q1 = 0.0;
	flume_Q2 = 0.0;
	flume_K = flume_get_K_factor(MainConfig.pipe_id);
}

static void flume_add_to_total(log_t * pPrevLogRec, log_t * pLogRec)
{
	// calculate and sum the volumetric flow between the two logged data points using SmartFlume calculations
	// do not "average", just keep a running total

	flume_total += flume_calc_gallons(flume_K, pPrevLogRec, pLogRec, &flume_Q1, &flume_Q2);
}

// User clicked "download"
int wifi_send_download(uint32_t DL_Type, uint32_t start_sec)
{
	//char txBuff[WIFI_BUF_SIZE];

	int dec1, dec2, dec3, dec4, dec5, dec6 = 0;
	//int dec7, dec8 = 0;
	uint32_t tmpsec = 0;
	log_t tmplog;
	log_t prev_log;
	float tmpfloat = 0.0f;
	float tmp_battery = 0.0f;
	uint32_t tmpPtr = LOG_BOTTOM;
	uint32_t tmp32 = 0;
	uint16_t tmp16 = 0;
	uint32_t tmpvar = 0;
	struct tm tm_st;
	struct tm tm_st2;
	int8_t itmp8 = 0;
	char level_alert_str[30];  // ,Level Alert 1,nn.nn\n
#if 0
	char batt_accum[20];  // either a null string, or the text "ttttttttt,aaaaa" where t is timestamp and a is raw accum
	char sec_str[5]; // either a null string, or the text ":nn", where nn is the seconds value of the timestamp
#endif
	char rtc_alert[30];  // either a null string, or the text ", Power on. RTC reset."
	char flume_str[30];

	// SD Variables.
	FIL sdFileName;
	FRESULT sd_res;

	rtc_alert[0] = 0;  // no rtc alert
	flume_str[0] = 0;  // no flume

	if (DL_Type == DL_TYPE_SDCARD)
	{

		char path[_MAX_LFN + 1];
		char site_name[20];

		sd_res = sd_init();  // Initialize the SD card and file system
		if (sd_res != FR_OK) return sd_res;

		// Create file name.
		tmpsec = rtc_get_time();
		sec_to_dt(&tmpsec, &tm_st);

		// Note: If MainConfig.site_name has ????? don't use it
		strcpy(site_name, MainConfig.site_name);
		if (strcmp(site_name, "?????") == 0)
		{
			strcpy(site_name, "00000");
		}

		// Remove problematic characters from site name
		remove_problematic_characters(site_name);

		// LONG FILENAME (up to 255)
		sprintf(path, "SITE_%s_%02d-%02d-%04d.csv",	site_name, tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year);

		if (sd_FactoryTest)
		{
			char mac_no_colons[20];
			int i;

			for (i=0; i < 20; i++)
			{
				char ch;
				ch = MainConfig.MAC[i];

				if ( ch == ':') ch = '_';

				mac_no_colons[i] = ch;
			}
			sprintf(path,"%s.csv", mac_no_colons);
		}


		// Open the file for writing
		sd_res = f_open(&sdFileName, path, FA_WRITE | FA_CREATE_ALWAYS);
		if (sd_res != FR_OK) return sd_res;

	}

	// Site ID
	sprintf((char *)uart2_outbuf, "Site ID,%s\r", MainConfig.site_name);
	__writeDownload(DL_Type, &sdFileName);

	// Location
	sprintf((char *)uart2_outbuf, "Location,%s\r", MainConfig.site_name);
	__writeDownload(DL_Type, &sdFileName);

	// Population
	sprintf((char *)uart2_outbuf, "Population,%lu\r", MainConfig.population);
	__writeDownload(DL_Type, &sdFileName);

	// Pipe ID, Vertical Mount, Damping
	tmpfloat = MainConfig.pipe_id;
	if (MainConfig.units == UNITS_METRIC)
	{
		tmpfloat *= 25.4;
	}
	convfloatstr_2dec(tmpfloat, &dec1, &dec2);



	tmpfloat = MainConfig.vertical_mount;
	if (MainConfig.units == UNITS_METRIC)
	{
		tmpfloat *= 25.4;
	}
	convfloatstr_2dec(tmpfloat, &dec3, &dec4);


	if (s2c_CellModemFound)
	{
		// Level Alert 1
		tmpfloat = MainConfig.AlertLevel1;
		if (MainConfig.units == UNITS_METRIC) {
			tmpfloat *= 25.4;
		}
		convfloatstr_2dec(tmpfloat, &dec5, &dec6);
		sprintf(level_alert_str,",Level Alert 1,%d.%02d", dec5, dec6);
	}
	else
	{
		level_alert_str[0]='\0';
	}

	sprintf((char *)uart2_outbuf, "Pipe ID,%d.%02d,Vertical Mount,%d.%02d,Damping,%d%s\r",
		dec1, dec2,         // Pipe ID
	    dec3, dec4,			// Vertical Mount
	    MainConfig.damping,	// Damping
		level_alert_str
	    );
	__writeDownload(DL_Type, &sdFileName);


	if (s2c_CellModemFound)
	{
		// Level Alert 2
		tmpfloat = MainConfig.AlertLevel2;
		if (MainConfig.units == UNITS_METRIC) {
			tmpfloat *= 25.4;
		}
		convfloatstr_2dec(tmpfloat, &dec5, &dec6);
		sprintf(level_alert_str,",Level Alert 2,%d.%02d", dec5, dec6);
	}
	else
	{
		level_alert_str[0]='\0';
	}

	// FW Ver, MAC, Serial Number
	sprintf((char *)uart2_outbuf, "FW Ver,%d.%d.%d,MAC,%s,Serial Number,%s%s\r",
			VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD,
			MainConfig.MAC,
			MainConfig.serial_number,
			level_alert_str);
	__writeDownload(DL_Type, &sdFileName);

	// Calc DownloadID from Serial Number
	tmpsec = rtc_get_time();
	sec_to_dt(&tmpsec, &tm_st);
	tmp32 = tm_st.tm_mon + tm_st.tm_mday + tm_st.tm_year + tm_st.tm_hour + tm_st.tm_min;
	tmpsec = LogTrack.last_download_datetime;
	sec_to_dt(&tmpsec, &tm_st2);
	itmp8 = strChkNum(MainConfig.serial_number);
	if (!itmp8) {
		tmpvar = atoi(MainConfig.serial_number);
		tmp32 += tmpvar;
		tmp32 -= LogTrack.log_counter;
		tmp16 = strlen(MainConfig.site_name);
		tmpvar = 0;

		for (int i = 0; i < tmp16; i++) {
			tmpvar += MainConfig.site_name[i];
		}

		if (tmpvar) {
			tmpvar /= tmp16;
			tmp32 += tmpvar;
		}
	}

	// Download Date, Last Download, Download ID
	sprintf((char *)uart2_outbuf, "Download Date,%02d/%02d/%04d %02d:%02d,Last Download Date,%02d/%02d/%d %02d:%02d,Download ID,%lu\r",
		tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year, tm_st.tm_hour, tm_st.tm_min,		// Download Date
	    tm_st2.tm_mon, tm_st2.tm_mday, tm_st2.tm_year, tm_st2.tm_hour, tm_st2.tm_min,	// Last Download
	    tmp32																			// Download ID
	    );
	__writeDownload(DL_Type, &sdFileName);

	// Battery Percent
	dec1 = ltc2943_get_battery_percentage();

	// Battery Current (remaining battery mAHr)
	tmp_battery = LogTrack.accumulator;
	convfloatstr_2dec(tmp_battery, &dec3, &dec4);

	// Estimated Battery Life
	tmp_battery = estimate_battery_life_in_days();

    // Why impose an arbitrary 900 day maximum battery life ???
	if ((tmp_battery > 0) && (tmp_battery < 900))
	{
		convfloatstr_2dec(tmp_battery, &dec5, &dec6);
	}
	else
	{
		dec5 = 0;
		dec6 = 0;
	}
	sprintf((char *)uart2_outbuf, "Battery Percent,%d,Battery Current,%d.%02d,Estimated Battery Life,%d.%02d days\r",
		dec1, //dec2, // Battery Percent
	    dec3, dec4, // Battery Current
	    dec5, dec6 	// Estimated Battery Life in days
	    );
	__writeDownload(DL_Type, &sdFileName);

	// Units, Log Interval, Logged Length
	if (MainConfig.units == UNITS_ENGLISH)
		sprintf((char *)uart2_outbuf, "Units,English,Log Interval,%lu minutes,Logged Length,%lu\r", (MainConfig.log_interval / 60), // Log Interval
		    LogTrack.log_counter // Logged Length
		    );
	else
		sprintf((char *)uart2_outbuf, "Units,Metric,Log Interval,%lu minutes,Logged Length,%lu\r", (MainConfig.log_interval / 60), // Log Interval
		    LogTrack.log_counter // Logged Length
		    );
	__writeDownload(DL_Type, &sdFileName);

	// Log rows header field labels
	strcpy((char *)uart2_outbuf, "Record #,DateTime,Level,Distance,Gain,Temp,Acc");

	if (ProductType == PRODUCT_TYPE_SMARTFLUME) // SmartFlume
	{
		strcat((char *)uart2_outbuf, ",Q,Total\r");
	}
	else
	{
		strcat((char *)uart2_outbuf, "\r");
	}
	__writeDownload(DL_Type, &sdFileName);

	// PROCESS ALL LOG ENTRIES. Get pointers first.
	if (start_sec)
	{
		tmpPtr = LogTrack.last_download_ptr;
	}
	else if (LogTrack.last_rollover_datetime)
	{
		tmpPtr = LogTrack.next_logrec_addr;
	}
	else
	{
		tmpPtr = LOG_BOTTOM;
	}

	// Establish the previous log record as the first log record extracted
	log_readstream_ptr(&prev_log, tmpPtr);
	flume_start_total();

	// Now for each log entry.
	uint8_t tmpData = 1;
	while (tmpData)
	{
        led_heartbeat();

		// Read a record.
		log_readstream_ptr(&tmplog, tmpPtr);

		if (tmplog.log_num <= LogTrack.log_counter)
		{

			// Watch for RTC inserted records
			if ((tmplog.log_num == (prev_log.log_num + 1)) && (tmplog.datetime == prev_log.datetime))
			{
				strcpy(rtc_alert, "Power on. RTC reset.");  // alert about rtc reset event
			}
			else
			{
				rtc_alert[0] = 0;  // no alert
			}

			// SmartFlume
			if ((tmplog.log_num == (prev_log.log_num + 1)))  // make sure there is a previous and current log record
			{
				if (ProductType == PRODUCT_TYPE_SMARTFLUME)
				{
					int dec1, dec2, dec3, dec4;
					flume_add_to_total(&prev_log, &tmplog);  // side-effects: sets globals flume_Q1, flume_Q2, flume_total

					convfloatstr_2dec(flume_Q2, &dec1, &dec2);
					convfloatstr_2dec(flume_total, &dec3, &dec4);
					sprintf(flume_str,"%d.%02d,%d.%02d", dec1, dec2, dec3, dec4);  // In CSV file: Q, RunningTotal
				}
				else
				{
					flume_str[0] = 0;  // no flume
				}
			}
			else
			{
				flume_str[0] = 0;  // no flume
			}

			prep_uart2_buffer(&tmplog, rtc_alert, flume_str, wifi_report_seconds);

			__writeDownload(DL_Type, &sdFileName);

			prev_log = tmplog;

			// zg - perhaps send periodic status updates to SewerWatch so it can show the user...
			// {tr:nnn, rw :nnn, bw:nnn}  tr=totalrecords, rw=recordswritten, bw=bytes_written

			// Move pointer to next log record.
			tmpPtr += LOG_SIZE;

			if (tmpPtr >= LOG_TOP) {
				tmpPtr = LOG_BOTTOM;
			}

			if (tmplog.log_num > LogTrack.log_counter) {
				tmpData = 0;
			}
		} else {
			tmpData = 0;
		}
	}

	// Set last download date in settings.
	log_set_last_download();

	// Close SD file handle if writing to SD card.
	if (DL_Type == DL_TYPE_SDCARD)
	{
		f_close(&sdFileName);
	}

	return 0;
}



// User clicked "settings"
void wifi_send_settings(void)
{
	//char txBuff[WIFI_BUF_SIZE];

	int dec1, dec2, dec3, dec4, dec5 = 0;
	int itype;
	//struct tm tm_st;
	time_t current_time;
	uint32_t seconds_since_power_on;
	uint32_t seconds_since_battery_reset;
	char cell_alert_1_str[40];
	char cell_alert_2_str[40];
	char log_datetime_str[TIME_STR_SIZE]; // mm/dd/yyyy hh:mm:ss
	char battery_reset_datetime_str[TIME_STR_SIZE]; // mm/dd/yyyy hh:mm:ss

	float tmp_AlertLevel1;
	float tmp_AlertLevel2;
	float tmp_pipe_id;
	float tmp_setLevel;


	// Make sure Alert1 and Alert2 are sensible.
	// This test can only be done pair-wise, because the JSON processing is sequential.
	// For example, if the pair is say (2,3)  and you want it to be (7,10) the firmware has
	// no way of knowing that the 10 is coming and it compares 7 to 3 and swaps them.
	// Consequently, the best place to perform the swap is when settings are requested,
	// both in the cell code and the wifi code.
	if (MainConfig.AlertLevel1 > MainConfig.AlertLevel2)
	{
		float swap;
		swap = MainConfig.AlertLevel1;
		MainConfig.AlertLevel1 = MainConfig.AlertLevel2;
		MainConfig.AlertLevel2 = swap;
		Save_MainConfig();
	}

	// Current time.
	//rtc_get_datetime(&tm_st);
	//current_time = convertDateToTime(&tm_st);
	current_time = rtc_get_time();
	time_to_str(current_time, log_datetime_str);  // "mm/dd/yyyy hh:mm:ss"

	// Calculate how long power has been on (to enable battery change reminder in UI)
	seconds_since_power_on =  current_time - TimeAtPowerOn;
	seconds_since_battery_reset = current_time - MainConfig.battery_reset_datetime;

	time_to_str(MainConfig.battery_reset_datetime, battery_reset_datetime_str);  // "mm/dd/yyyy hh:mm:ss"

	tmp_AlertLevel1   = MainConfig.AlertLevel1;
	tmp_AlertLevel2   = MainConfig.AlertLevel2;
	tmp_pipe_id       = MainConfig.pipe_id;
	tmp_setLevel      = MainConfig.setLevel;

	if (MainConfig.units == UNITS_METRIC)
	{
		tmp_AlertLevel1 *= 25.4;
		tmp_AlertLevel2 *= 25.4;
		tmp_pipe_id     *= 25.4;
		tmp_setLevel    *= 25.4;
	}



	convfloatstr_2dec(tmp_AlertLevel1, &dec1, &dec2);
	sprintf(cell_alert_1_str,"%d.%02d", dec1, dec2);

	convfloatstr_2dec(tmp_AlertLevel2, &dec1, &dec2);
	sprintf(cell_alert_2_str,"%d.%02d", dec1, dec2);

	// Pipe ID
	convfloatstr_2dec(tmp_pipe_id, &dec1, &dec2);

	// Level
	convfloatstr_2dec(tmp_setLevel, &dec3, &dec4);

	// Battery
	dec5 = ltc2943_get_battery_percentage();

	// Units
	char units[12];
	if (MainConfig.units == UNITS_METRIC) {
		strcpy(units, "Metric");
	} else {
		strcpy(units, "English");
	}

#if 0
	// FirstResponseIQ
	if (IsFirstResponse())
	{
		itype = 1;
	}
	else
	{
		//if (BoardId == BOARD_ID_SEWERWATCH_1)
		//{
		//    itype = 2;
		//}

	    itype = 0;
	}
#else
	itype = ProductType;
#endif



#if 0

	// Comments from Max on Settings

	//Unknown
//itrackertype
//loc
//setLevel
//cell_imeisv
//cell_error
	//not needed
//mode
//cell_carrier
//cell_apn
//wifiAntenna
	// to add
// CellEnabled
// alert1
// alert2
	// Uniform naming style


#endif


	// Settings
char * fmt = "{\"Site_Name\":\"%s\","
		      "\"Pipe_ID\":%d.%02d,"
		      "\"Damping\":%d,"
		      "\"Population\":%lu,"
		      "\"Firm_v\":\"%d.%d.%d\","
		      "\"sn\":\"%s\","
              "\"type\":%d,"
		      "\"logInterval\":%lu,"
		      "\"sysm\":\"%s\","
              "\"Log_DateTime\":\"%s\","
		      "\"setLevel\":%d.%02d,"
		      "\"Battery\":%d,"
	          "\"BatDate\":\"%s\","
              "\"BatOn\":%lu,"
		      "\"TimeOn\":%lu,"
              "\"cell_found\":%d,"
		      "\"cell_enabled\":%d,"
		      "\"cell_imei\":\"%s\","
		      "\"cell_status\":\"%s\","
		      "\"cell_alert_1\":%s,"
		      "\"cell_alert_2\":%s,"
		      "\"wifiWakeUp\":%lu}";

// Compose whole response and send.
sprintf((char *)uart2_outbuf, fmt,
		MainConfig.site_name,			// Site Name
		dec1, dec2, 					// Pipe ID
		MainConfig.damping, 			// Damping
		MainConfig.population, 			// Population
		VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD, // Firmware Version
		MainConfig.serial_number,		// SN
		itype, 							// type  0=iTracker   1=FirstResponseIQ  2=SewerWatch?
		(MainConfig.log_interval / 60), // Log interval
		units, 							// sysm
		log_datetime_str,               // Current timedate ;  tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year, tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec, // Current time.
		dec3, dec4,						// Level
		dec5, //dec6,						// Battery
		battery_reset_datetime_str,     // BatDate
		seconds_since_battery_reset,    // BatOn
		seconds_since_power_on,         // TimeOn
		s2c_CellModemFound,             // 0 = no cell; 1 = telit; 2 = xbee3, 3 = Laird
		(IsCellEnabled()?1:0),
		s2c_imei,
		//CellConfig.status,
		s2c_cell_status,
		cell_alert_1_str,
		cell_alert_2_str,
		MainConfig.wifi_wakeup_interval
);

	// Send the whole settings string.
	uart2_tx_outbuf();
}

void wifi_send_alarm(void)
{
	//char txBuff[128];

	// Convert for units.
	if (MainConfig.units == UNITS_ENGLISH)
	{
		sprintf((char *)uart2_outbuf, "{\"Alarm_1\":\"%s\",\"Alarm_2\":\"%s\",\"Alarm_3\":\"%s\"}",
			"None", "None", "None");
	}
	else
	{
		sprintf((char *)uart2_outbuf, "{\"Alarm_1\":\"%s\",\"Alarm_2\":\"%s\",\"Alarm_3\":\"%s\"}",
			"None", "None", "None");
	}
	uart2_tx_outbuf();
}

void wifi_send_id()
{
	//char txBuff[128];
	char device_type[14];
	char fw_ver[28];

	sprintf(device_type, "iTracker");
	sprintf(fw_ver, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);

	sprintf((char *)uart2_outbuf, "{\"DeviceType\":\"%s\",\"FirmwareVersion\":\"%s\"}", device_type, fw_ver);
	uart2_tx_outbuf();
}

void wifi_send_cloud_packet(char PacketType, uint32_t record_number)
{
	if (record_number == 0)
	{
		s2c_prep_settings_packet((char *)uart2_outbuf);
	}
	else
	{
	  if (PacketType == 'R')
	  {
		s2c_prep_log_packet((char *)uart2_outbuf, 'L', record_number, 12);
	  }
	  else if (PacketType == 'T')
	  {
		s2c_prep_log_packet((char *)uart2_outbuf, 'V', record_number, 6);  // only send 6 to keep packet size reasonable
	  }
	  else
	  {
		  return;   // unrecognized packet type
	  }
	}

	uart2_tx_outbuf();

}


static void wifi_send_cell_response_buffer(void)
{
	sprintf((char *)uart2_outbuf, "[%s]", uart1_inbuf);
	uart2_tx_outbuf();
}

#if 0

void wifi_append_debug_string(char *Msg)
{
	int space_available;
	int space_needed;

	space_available = sizeof(DebugStringToSendBack) - strlen(DebugStringToSendBack);
	space_needed = strlen(Msg) + 1;

    if (space_available > space_needed)
    	strcat(DebugStringToSendBack, Msg);
    else
    	strncat(DebugStringToSendBack, Msg, space_available - 1);
}

void wifi_empty_debug_string(void)
{
	DebugStringToSendBack[0] = '\0';
}

void wifi_send_debug()
{

	// Start the JSON response here by echoing the command.  Additional variables must precede themselves with a leading comma.
	sprintf(DebugStringToSendBack, "{\"DEBUG\":\"%s\"", DebugStringFromUI);

	// Perform the command and trace things here...
	if (strcmp(DebugStringFromUI, "TraceBuffer") == 0)
	{
		wifi_append_debug_string(",\"TraceBuffer\":\"");
		wifi_append_debug_string(TraceBuffer);
		wifi_append_debug_string("\"");

	}
	else if (strcmp(DebugStringFromUI, "TraceInit") == 0)
	{
		trace_Init();
	}
	else if (strcmp(DebugStringFromUI, "TraceSave") == 0)
	{
		trace_SaveBuffer();
	}

	// Close the JSON response here
	wifi_append_debug_string("}");

	uart2_tx_string(DebugStringToSendBack);
}
#endif


void reset_to_factory_defaults(void)
{
	Reset_MainConfig(DO_NOT_RESET_SERIAL_NUMBER);  // delay saving until all JSON variables processed
	Actual_Blanking = 900;	// Power-on default usec for 5.6 in.  This does not match the sensor_get_blanking(uint8_t gain) function which returns a number from 1000 to 1500
	Actual_Gain = 127;		// Power-on default
	Reset_CellConfig();
	log_start(0);
}

void process_NumericCommands(char * val)
{
	// scans text of the form <digits_1>,<digits_2>

	// Find the comma and split into two null-terminated strings
	char * digits_1;
	char * digits_2;

	digits_1 = val;
	digits_2 = NULL;
	for (int i=0; i < strlen(val); i++)
	{
		if (val[i] == ',')
		{
		    digits_2 = &val[i+1];
		    break;
		}
	}

	json_cmd_id = atoi(digits_1);
	json_sub_cmd_id = 0;
	if (digits_2)
	{
		json_sub_cmd_id = atoi(digits_2);
	}

	//trace_AppendMsg("\n");
	//trace_AppendMsg(val);
	//trace_AppendMsg("\n");
}

void process_Command(char * val)
{
	// Convert initial caps into lowercase
	val[0] = tolower(val[0]);


	if (strcmp(val, "demo") == 0)
	{
		strcpy(MainConfig.site_name, "demo");
		loadSampleData(SAMPLE_DATA_ONE_MONTH, SAMPLE_DATA_TYPE_PIPE_8);
	}
	else if (strcmp(val, "clear") == 0)
	{
		log_start(0);
	}
	else if (strcmp(val, "flume4") == 0)
	{
		strcpy(MainConfig.site_name, "flume4");
		loadSampleData(SAMPLE_DATA_ONE_MONTH, SAMPLE_DATA_TYPE_FLUME_4);
	}
	else if (strcmp(val, "flume6") == 0)
	{
		strcpy(MainConfig.site_name, "flume6");
		loadSampleData(SAMPLE_DATA_ONE_MONTH, SAMPLE_DATA_TYPE_FLUME_6);
	}
	else if (strcmp(val, "flume8") == 0)
	{
		strcpy(MainConfig.site_name, "flume8");
		loadSampleData(SAMPLE_DATA_ONE_MONTH, SAMPLE_DATA_TYPE_FLUME_8);
	}
	else if (strcmp(val, "flume10") == 0)
	{
		strcpy(MainConfig.site_name, "flume10");
		loadSampleData(SAMPLE_DATA_ONE_MONTH, SAMPLE_DATA_TYPE_FLUME_10);
	}
	else if (strcmp(val, "flume12") == 0)
	{
		strcpy(MainConfig.site_name, "flume12");
		loadSampleData(SAMPLE_DATA_ONE_MONTH, SAMPLE_DATA_TYPE_FLUME_12);
	}


	else if (strcmp(val, "new") == 0)
	{
		log_start(0);
		MainConfig.damping = 3;
		MainConfig.vertical_mount = 0;
		strcpy(MainConfig.site_name, DEFAULT_SITE_NAME);
		strcpy(MainConfig.AddText, " ");
		MainConfig.AlertLevel1 = 0;
		MainConfig.AlertLevel2 = 0;
		saveMainConfig = 1;
	}

	else if (strcmp(val, "battery") == 0)
	{
		ltc2943_set_battery_percentage(100);
	}

#if 0
	else if (strcmp(val, "sleep") == 0)
	{
		json_cmd_id = CMD_SLEEP;
	}
#else
	else if (strstr(val, "sleep"))
	{
		// check for variations
		switch (val[5])
		{
		default:
		case '\0': SleepDelayInMinutes = 0; break;
		case '1':  SleepDelayInMinutes = 1; break;
		case '2':  SleepDelayInMinutes = 2; break;
		case '3':  SleepDelayInMinutes = 3; break;
		case '4':  SleepDelayInMinutes = 4; break;
		case '5':  SleepDelayInMinutes = 5; break;
		case '6':  SleepDelayInMinutes = 6; break;
		case '7':  SleepDelayInMinutes = 7; break;
		case '8':  SleepDelayInMinutes = 8; break;
		case '9':  SleepDelayInMinutes = 9; break;
		}
		json_cmd_id = CMD_SLEEP;
		val[0] = '\0';  // kludge to fix strange error when sleep is issued and network ssid gets changed
	}

#endif

	else if (strcmp(val, "reset") == 0)
	{
		reset_to_factory_defaults();

		saveCellConfig = 1;
		saveMainConfig = 1;
	}

	else if (val[0] == 'n')
	{
		if (val[1] == ' ')  // must have a blank after the n
		{
			if (val[2])  // cannot be a null-terminated string
			{
				val[11] = '\0';
				strcpy(MainConfig.serial_number, val + 2);  // string is of the form "n 234567890"  and will yield at least 1 character for serial number

				//5/9/2022
				// Note:  changing the serial number technically changes the Bluetooth advertisement name.
				// Unclear how best to get that to happen since the connection is in progress ?  Same with Site Name?
				// Maybe for now just keep the connection open and at the next ESP32 power cycle it will correct itself...

				//reset_to_factory_defaults();  // 5/9/2022 zg removed at Matt's request.  As there is already other ways to reset factory defaults, it is not needed here.

				saveMainConfig = 1;
			}
		}
	}

	else if (val[0] == 's')
	{
		val[21] = '\0';
		for (int x = 0; x < 20; x++)
		{
			MainConfig.SSID[x] = '\0';
		}
		strcpy(MainConfig.SSID, val + 2);
		saveMainConfig = 1;
		json_cmd_id = CMD_WIFI_CONFIG;  // Perform a wifi reconfig now
	}

	else if (val[0] == 'a')
	{
		if (val[1] == '1')
		{
			SetWiFiInternalAnt();
		}
		else
		{
			ClearWiFiInternalAnt();
		}
		saveMainConfig = 1;
		json_cmd_id = CMD_WIFI_CONFIG;  // Perform a wifi reconfig now

	}
#if 0	// deprecated, use Force_xxx SD card files to force ProductType
	else if (val[0] == 'b')
	{
		if (val[1] == '1')
		{
			SetBLENameOverride();
		}
		else
		{
			ClearBLENameOverride();
		}
		saveMainConfig = 1;
	}
	else if (val[0] == 'f')
	{
		if (val[1] == '1')
		{
			SetFirstResponse();
		}
		else
		{
			ClearFirstResponse();
		}
		saveMainConfig = 1;
	}
#endif
	else if (val[0] == 'k')
	{
		val[10] = '\0';
		if (strlen(val) == 10)
		{
			if (wifi_chkkey(val + 2) == 0)
			{
				strcpy(MainConfig.PassKey, val + 2);
				saveMainConfig = 1;
				json_cmd_id = CMD_WIFI_CONFIG;  // Perform a wifi reconfig now
			}
		}
	}

	// Alternate SYNC DATE
	else if (val[0] == 'd')
	{
		struct tm tm_st;
		parse_datetime(val, &tm_st);
		rtc_sync_time(&tm_st);   // Cellular also syncs to the network UTC at power on.
	}


}

static void parse_setLevel(char *val)
{
	int8_t tmpStat;

	//Check valid data in float string
	tmpStat = strChkNum(val);

	if (!tmpStat)
	{
		float tmp_float_val;
		tmp_float_val = strtof(val, 0);
		if ((tmp_float_val >= 0.0) && (tmp_float_val < 4000))
		{
			if (tmp_float_val < 0.01)
				tmp_float_val = 0.01;
			if (MainConfig.units == UNITS_METRIC)
				tmp_float_val /= 25.4;


			int old = MainConfig.damping;

			MainConfig.damping = 9;  // zg - why is the damping being overridden here ?

			get_sensor_measurement_0();  // Side effect: sets LastGoodMeasure

			MainConfig.damping = old;

			// Set the level.
			MainConfig.setLevel = tmp_float_val;

			// Compute the new vertical mount.
			MainConfig.vertical_mount = MainConfig.setLevel + LastGoodMeasure;
			saveMainConfig = 1;
		}
	}
}

static void parse_cell_alert_1(char *val)
{
	int8_t tmpStat;


	//Check valid data in float string
	tmpStat = strChkNum(val);

	if (!tmpStat)
	{
		float tmp_float_val;
		tmp_float_val = strtof(val, 0);
		if ((tmp_float_val >= 0.0) && (tmp_float_val < 4000))
		{
			if (MainConfig.units == UNITS_METRIC)
				tmp_float_val /= 25.4;

			MainConfig.AlertLevel1 = tmp_float_val;

			saveMainConfig = 1;
		}
	}
}

static void parse_cell_alert_2(char *val)
{
	int8_t tmpStat;

	//Check valid data in float string
	tmpStat = strChkNum(val);

	if (!tmpStat)
	{
		float tmp_float_val;
		tmp_float_val = strtof(val, 0);
		if ((tmp_float_val >= 0.0) && (tmp_float_val < 4000))
		{
			if (MainConfig.units == UNITS_METRIC)
				tmp_float_val /= 25.4;

			MainConfig.AlertLevel2 = tmp_float_val;

			saveMainConfig = 1;
		}
	}
}

int8_t parse_JSON_variable(char * jsonname, char * jsonvalue, uint8_t jsontype)
{
	float tmp_float_val = 0;
	uint32_t tmp_int_val = 0;
	int8_t itmp8 = 0;





	if (strcmp(jsonname, "comm") == 0)  // zg - note there is also a "Command" keyword below...
	{
		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);
		json_cmd_id = tmp_int_val;  // See CMD_ID etc values in WiFi_socket.h
	}

	else if (strcmp(jsonname, "cell_band") == 0)  // {"cell_band":n} where n = 2, 4, 12, 13, 17, 9001, 9064      9001 (all USA bands)   n=9064 (all NZ bands)  (9000 + Country Code)
	{
		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);
		json_sub_cmd_id = tmp_int_val;
		json_cmd_id = CMD_CELL_BAND;
	}

#if 1
    // Parse Cloud Protocol here.  Cloud protocol requests are spread across two name/value pairs in a single JSON packet
    //       Resend  {"1":"R","N":"123"}
    //    or Verbose {"1":"T","N":"123"}
    else if (strcmp(jsonname, "1") == 0)
    {
    	json_cloud_packet_type = jsonvalue[0];  // Only R or T is active
    }
    else if (strcmp(jsonname, "N") == 0)  // Note this is distinct from {"Command":"n 123"} which sets serial number
    {
    	if ((json_cloud_packet_type == 'R') || (json_cloud_packet_type == 'T'))
		{
			// parse starting record number
			if (jsontype == 3)
			{
				strChkNum(jsonvalue);
				json_cloud_record_number = atoi(jsonvalue);  // parse starting cloud record number
			    json_cmd_id = CMD_SEND_CLOUD_PKT;
			}

		}
    }
#endif

	else if (strcmp(jsonname, "Log_DateTime") == 0)
	{
#if 0
		5/13/2022 zg debugging problem with RTC from App on 5.0.58 and 5.0.59
Earlier versions of the App would send a string like this:
{"comm":6,"Log_DateTime":"\"05/13/2022 12:33:58\""}
		which is a length of 51 but broken into two name/value pairs
		use the debugger to examine the contents of
		jsonname and jsonvalue to see how the escaped quotes were handled
jsonname=Log_DateTime
jsonvalue=\\05/13/2022 12:33:58\\

Posted to Basecamp:

It will perform them all, from left to right.  The "execution" happens at the comma or the closing brace {.

So, that means that order is significant.  If you type  {"comm":6,"Log_DateTime":"\"05/13/2022 07:48:13\""}
the Settings are issued BEFORE the time is synchronized.  Consequently, the UI may give the wrong impression
about setting the time to a new time value.
The other thing I found is that even though the date time string is malformed (has extra characters in it),
the parser was only looking for digits, forward slashes, maybe the space, and the colons.  So, it will correctly
parse what is being sent.  However, I would prefer the syntax be fixed at some point in the future.

#endif
		struct tm tm_st;
		parse_datetime(jsonvalue, &tm_st);
		rtc_sync_time(&tm_st);   // Cellular also syncs to the network UTC at power on.
	}


	// Set the water level. Vertical Mount is calculated from this.
	else if (strcmp(jsonname, "setLevel") == 0)
	{
		parse_setLevel(jsonvalue);
	}

	else if (strcmp(jsonname, "Command") == 0)
	{
		process_Command(jsonvalue);
	}

	else if (strcmp(jsonname, "Site_Name") == 0)
	{
		if (jsontype == 3)
		{
            // limit incoming site name to the max possible
			jsonvalue[ sizeof(MainConfig.site_name) - 1] = 0;  // 19 characters plus a null
			strcpy(MainConfig.site_name, jsonvalue);
			saveMainConfig = 1;
		}
	}

	else if (strcmp(jsonname, "Pipe_ID") == 0)
	{
		strChkNum(jsonvalue);
		tmp_float_val = atof(jsonvalue);

		if (tmp_float_val > 0)
		{
			if (MainConfig.units == UNITS_METRIC)
				tmp_float_val /= 25.4;
			MainConfig.pipe_id = tmp_float_val;
		}
		saveMainConfig = 1;
	}

	else if (strcmp(jsonname, "Damping") == 0)
	{
		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);
		if ((tmp_int_val > 0) && (tmp_int_val <= 9))
		{
			MainConfig.damping = tmp_int_val;
			saveMainConfig = 1;
		}
	}
#if 0 // zg -removed in iTracker5
	else if (strcmp(jsonname, "ID") == 0)
	{
		if (jsonvalue[0] == '?')
		{
			json_cmd_id = CMD_ID;  // zg - technically, this should be in the "Command" parsing...
		}
	}
#endif

#if 0
	else if (strcmp(jsonname, "DEBUG") == 0)
	{
		strcpy(DebugStringFromUI, jsonvalue);
		json_cmd_id = CMD_DEBUG;  // zg - technically, this should be in the "Command" parsing...
	}
#endif

	else if (strcmp(jsonname, "sysm") == 0)
	{
		if (jsonvalue[0] == '1')
			MainConfig.units = UNITS_METRIC;
		else if (jsonvalue[0] == '2')
			MainConfig.units = UNITS_ENGLISH;
		else if (strcmp(jsonvalue, "Metric") == 0)
			MainConfig.units = UNITS_METRIC;
		else
			MainConfig.units = UNITS_ENGLISH;
		saveMainConfig = 1;
	}

	else if (strcmp(jsonname, "Population") == 0)
	{
		itmp8 = strChkNum(jsonvalue);
		if (itmp8 == 0)
		{
			tmp_int_val = atoi(jsonvalue);
			if (tmp_int_val > 0)
			{
				MainConfig.population = tmp_int_val;
			}
			saveMainConfig = 1;
		}
	}

	else if (strcmp(jsonname, "logInterval") == 0)
	{
		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);
		if ((tmp_int_val == 1) || (tmp_int_val == 5) || (tmp_int_val == 10) || (tmp_int_val == 15) || (tmp_int_val == 30) || (tmp_int_val == 60))
		{
			MainConfig.log_interval = tmp_int_val * 60;
			saveMainConfig = 1;
		}
	}

	else if (strcmp(jsonname, "wifiWakeUp") == 0)  // "u"
	{
		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);
		MainConfig.wifi_wakeup_interval = tmp_int_val;
		saveMainConfig = 1;
	}

	else if (strcmp(jsonname, "cell_alert_1") == 0)
	{
		parse_cell_alert_1(jsonvalue);
	}

	else if (strcmp(jsonname, "cell_alert_2") == 0)
	{
		parse_cell_alert_2(jsonvalue);
	}

	else if (strcmp(jsonname, "cell_apn") == 0) {
		strcpy(CellConfig.apn, jsonvalue);
		saveCellConfig = 1;
	}

	else if (strcmp(jsonname, "cell_ctx") == 0) {
		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);
		CellConfig.ctx = tmp_int_val;  // Context (Carrier)  0 = no override configured; 1 = ATT; 3 = Verizon
		saveCellConfig = 1;
	}

	else if (strcmp(jsonname, "cell_enabled") == 0) {
		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);

		if (tmp_int_val == 1)
		{
			if (!IsCellEnabled())
			{
		      SetCellEnabled();
		      saveMainConfig = 1;
			}
		}
		else
		{
			if (IsCellEnabled())
			{
		      ClearCellEnabled();
		      saveMainConfig = 1;
			}
		}

	}

	else if (strcmp(jsonname, "cell_ip") == 0) {
		strcpy(CellConfig.ip, jsonvalue);
		saveCellConfig = 1;
	}

	else if (strcmp(jsonname, "cell_ping") == 0) {
		//strChkNum(val);
		//tmp_int_val = atoi(val);
		if (jsonvalue[0] == '1')
		{

		}
		//saveCellConfig = 1;
	}

	else if (strcmp(jsonname, "cell_port") == 0) {
		strcpy(CellConfig.port, jsonvalue);
		saveCellConfig = 1;
	}


#if 0
	else if (strcmp(jsonname, "xbee3_reset") == 0) {
		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);

		if (tmp_int_val == 1)  // must specify "1" to perform an xbee3 reset
		{
			if (!IsCellEnabled())
			{
		      SetCellEnabled();
		      saveMainConfig = 1;
		      change_from_local_to_UTC();
			}
			xbee3_reset();  // this is not yet fully implemented
		}
	}
#endif


	else if (strcmp(jsonname, "CellSamples") == 0) {
		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);
		CellConfig.SampleCnt = tmp_int_val;  // Number of samples to send in one phone call
		saveCellConfig = 1;
	}

	else if (strcmp(jsonname, "GraphType") == 0) {

		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);
		CurrentGraphType = tmp_int_val;
	}

	else if (strcmp(jsonname, "GraphDay") == 0) {

		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);
		CurrentGraphDays = tmp_int_val;
	}

	else if (strcmp(jsonname, "startTime") == 0) {

		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);
		if (tmp_int_val) {
			Download_startTime = LogTrack.last_download_datetime;
		} else {
			Download_startTime = 0;
		}
	}

	else if (strcmp(jsonname, "dlType") == 0) {

		strChkNum(jsonvalue);
		tmp_int_val = atoi(jsonvalue);
		Download_type = tmp_int_val;
	}

	else if (strcmp(jsonname, "alert1") == 0) {

		strChkNum(jsonvalue);
		tmp_float_val = atof(jsonvalue);

		if (MainConfig.units == UNITS_METRIC)
			tmp_float_val /= 25.4;

		MainConfig.AlertLevel1 = tmp_float_val;
		saveMainConfig = 1;
	}

	else if (strcmp(jsonname, "alert2") == 0) {

		strChkNum(jsonvalue);
		tmp_float_val = atof(jsonvalue);

		if (MainConfig.units == UNITS_METRIC)
			tmp_float_val /= 25.4;
		MainConfig.AlertLevel2 = tmp_float_val;
		saveMainConfig = 1;
	}

	else if (strcmp(jsonname, "Clear_Alarm") == 0) {

		strChkNum(jsonvalue);
		tmp_float_val = atof(jsonvalue);

		if (MainConfig.units == UNITS_METRIC)
			tmp_float_val /= 25.4;

		MainConfig.ClearLevel = tmp_float_val;  // zg this is not used anywhere...
		saveMainConfig = 1;
	}

	else if (strcmp(jsonname, "Battery_P") == 0) {

		strChkNum(jsonvalue);
		tmp_float_val = atof(jsonvalue);
		MainConfig.AlarmBatt = tmp_float_val;
		saveMainConfig = 1;
	}

	#ifdef CELL_SMS
	else if (jsonname[0] == 'P')
	{
		memcpy(tmpstr,jsonname,6);
		tmpstr[6] = 0;
		if (strcmp(tmpstr,"Phone_") == 0) {
			if ((jsontype == 3) && (strlen(jsonvalue) <= 16)) {
				if (wifi_chk_number(jsonvalue) == 0) {
					tmp = (atoi(jsonname+6)) - 1;
					if (tmp < 7) {
						jsonvalue[15] = '\0';
						strcpy(MainConfig.PhoneNum[tmp], jsonvalue);
						saveMainConfig = 1;
					}
				}
			}
		}
		else {
			jsonvalue[15] = '\0';
		}
	}
	#endif

	else if (strcmp(jsonname, "Add_Text") == 0)
	{
		if (jsontype == 3)
		{
            // limit incoming text to the max possible
			jsonvalue[ sizeof(MainConfig.AddText) - 1] = 0;  // 23 characters plus a null
			strcpy(MainConfig.AddText, jsonvalue);
			saveMainConfig = 1;
		}
	}

	return 0;
}

// Parses a string of the form "8/27/2014 13:15"  or "8/27/2014 13:15:32"
void parse_datetime(char * strdate, struct tm * tm_st)
{
	int step = 1;
	char tmpstr[6];
	uint8_t incnt = 0;
	uint8_t flag = 0;


	tm_st->tm_sec = 0;  // in case no seconds are provided in the string passed in


	for (int i = 0; i < strlen(strdate) + 1; i++) {
		switch (step) {
			case 1:
				if ((strdate[i] >= '0') && (strdate[i] <= '9')) {
					tmpstr[incnt++] = strdate[i];
					flag = 1;
				} else if (flag) {
					tmpstr[incnt] = 0;
					tm_st->tm_mon = atoi(tmpstr);
					step = 2;
					flag = 0;
					incnt = 0;
				}
				break;
			case 2:
				if ((strdate[i] >= '0') && (strdate[i] <= '9')) {
					tmpstr[incnt++] = strdate[i];
					flag = 1;
				} else if (flag) {
					tmpstr[incnt] = 0;
					tm_st->tm_mday = atoi(tmpstr);
					step = 3;
					flag = 0;
					incnt = 0;
				}
				break;
			case 3:
				if ((strdate[i] >= '0') && (strdate[i] <= '9')) {
					tmpstr[incnt++] = strdate[i];
					flag = 1;
				} else if (flag) {
					tmpstr[incnt] = 0;
					tm_st->tm_year = atoi(tmpstr);
					step = 4;
					flag = 0;
					incnt = 0;
				}
				break;
			case 4:
				if ((strdate[i] >= '0') && (strdate[i] <= '9')) {
					tmpstr[incnt++] = strdate[i];
					flag = 1;
				} else if (flag) {
					tmpstr[incnt] = 0;
					tm_st->tm_hour = atoi(tmpstr);
					step = 5;
					flag = 0;
					incnt = 0;
				}
				break;
			case 5:
				if ((strdate[i] >= '0') && (strdate[i] <= '9')) {
					tmpstr[incnt++] = strdate[i];
					flag = 1;
				} else if (flag || (strdate[i] == 0x00)) {
					tmpstr[incnt] = 0;
					tm_st->tm_min = atoi(tmpstr);
					step = 6;
					flag = 0;
					incnt = 0;
				}
				break;
			case 6:
				if ((strdate[i] >= '0') && (strdate[i] <= '9')) {
					tmpstr[incnt++] = strdate[i];
					flag = 1;
				} else if (flag || (strdate[i] == 0x00)) {
					tmpstr[incnt] = 0;
					tm_st->tm_sec = atoi(tmpstr);
					step = 7;
					flag = 0;
					incnt = 0;
				}
				break;
			default:
				break;

		}
	}

	// Initialize unused fields to 0, just to be safe
	tm_st->tm_isdst = 0;
	tm_st->tm_wday = 0;
	tm_st->tm_yday = 0;
}

uint32_t parse_timestamp(char *value)
{
    struct tm tm_st;
    uint32_t timestamp;
    parse_datetime(value, &tm_st);
    dt_to_sec(&tm_st, &timestamp);
    return timestamp;
}

static char prev_ch = 0;

static int add_char_to_JSON_string(char ch)
{

#if 0   // Gen5: This is probably not needed when processing JSON
	//Discard CR LF and nulls here (previously was inside uart2)
	if ((ch == '\r') || (ch == '\n') || (ch == '\0'))
	{
		return JSON_INCOMPLETE;
	}
#endif

	if (ch == '{')
	{
		if (sd_WiFiDataLogEnabled)
		{
			if (json_step != 0) trace_AppendMsg("\nUnexpected {\n");
			trace_AppendMsg("\n");
		}

		json_step = 0;
	}
	else if (ch == '[')   // watch for square bracket command
	{
		if (json_step == 0)
		{
			json_step = 5;  // switch to square bracket parsing mode
		}
	}


	if (sd_WiFiDataLogEnabled)
	{
		trace_AppendChar(ch);
	}

	switch (json_step)
	{
		case 0: {
			if (ch == '{') {
				json_step = 1;
				json_name_cnt = 0;
				json_value_cnt = 0;
				json_type = 1;
				// zg null-terminate both buffers here ?
			}
			break;
		}

		case 1: {
			if (ch == '"') {
				json_step = 2;
			}
			break;
		}

		case 2: {
			if (ch != '"') {
				if (json_name_cnt < JSON_NAME_SIZE) {
					json_name[json_name_cnt++] = ch;
				}
			} else if (json_name_cnt > 0) {
				json_step = 3;
			}
			break;
		}

		case 3: {
			if (ch == ':') {
				json_step = 4;
				json_inString = 0;
			}
			break;
		}

		case 4: {
			if (((ch == ',') && !json_inString) || (ch == '}'))
			{
				// zg - this is potentially dangerous the "cnt2" variables can exceed the buffer size

				if (json_name_cnt < JSON_NAME_SIZE)
				    json_name[json_name_cnt] = 0x00;  // null-terminate
				else
					json_name[JSON_NAME_SIZE] = 0x00;  // null-terminate

				if (json_value_cnt < JSON_VALUE_SIZE)
				  json_value[json_value_cnt] = 0x00;  // null-terminate
				else
					json_value[JSON_VALUE_SIZE] = 0x00;  // null-terminate

				// workaround to allow shortcut notation for numeric commands in the Admin text box on the App
				if ((strcmp(json_name,"Command") == 0) && isdigit(json_value[0])) // scan for shortcut notation for numeric commands
				{
					process_NumericCommands(json_value);
					ch = '}';  // discard any remaining json to force cmd clicks for example: {"Command":"38,9001","comm":6}
				}
				else
				{
				  parse_JSON_variable(json_name, json_value, json_type);
				}

				if (ch == ',') {
					json_step = 1;
					json_value_cnt = 0;
					json_name_cnt = 0;
					json_type = 1;
				} else {
					json_complete = 1; // MAIN LOOP PROCESSING FOR USER PAGE CLICKS.
					json_step = 0;

					if (sd_WiFiDataLogEnabled)
					{
						trace_AppendMsg("\n");
					}

					// zg - MUST EXIT WHILE LOOP HERE OTHERWISE CAN DROP AN ENTIRE JSON AT NEXT OPEN BRACE
					return JSON_COMPLETE;

				}
			}
			else
			{
				if (ch == '"')
					json_inString = 0;
				if (json_type == 1) {
					if (ch == '"') {
						json_type = 3;
						if (!json_inString)
							json_inString = 1;
						else
							json_inString = 0;
					}
					if (ch == '.')
						json_type = 2;

				}
				if (ch != '"') {
					if (json_value_cnt < JSON_VALUE_SIZE) {
						json_value[json_value_cnt++] = ch;
					}
				} else {
				}
			}
			break;
		}

		// Using Advanced text box on App.  Typing the following and then pressing Submit
		// results in the JSON shown
		// [ON]  {"Command":"[ON]","comm":6}
		// 33    {"Command":"33","comm":6}
		// 33,46 {"Command":"33,46","comm":6}
		// this is a test    {"Command":"this is a test","comm":6}
		// So.. not exactly the pass-thru that was intended

		case 5: // enter square bracket parsing mode
			if (ch == '[')
			{
				prev_ch = ch;
				json_step = 6;
				json_name_cnt = 0;
				json_value_cnt = 0;
				json_type = 1;
				// zg null-terminate both buffers here ?
			}
			break;
		case 6: // gather characters watching for square brackets
			if (ch == ']')
			{
				json_step = 0;  // exit the bracket parsing mode
				if (prev_ch == '[')  // watch for pattern [] which is a request to retrieve the response from the cell (uart1) and forward to the WiFi (uart2)
				{
					// Backdoor to turning on cell power is to use "comm":33, "comm":32, or "comm":31 commands before using this syntax
					// Note that cell powerup takes at least 10 seconds
					wifi_send_cell_response_buffer();
				}
				else
				{
					// ASSERT this is the start of an outbound command to the cell modem

					// check for tokens [ON]
					if (strstr(json_name,"[ON]"))
					{
						AppendStatusMessage("[ON] detected");
						s2c_trigger_power_on();   // this triggers cell to turn on and then stay on
					}
					else if (strstr(json_name,"[OFF]"))
					{
						AppendStatusMessage("[OFF] detected");
						s2c_trigger_power_off();  // this triggers cell to turn off
					}
					else
					{
						// terminate with a \r
						// flush the response buffer
						// send the string to the cell modem
						if (json_name_cnt < JSON_NAME_SIZE)
						{
							json_name[json_name_cnt++] = '\r';
							json_name[json_name_cnt] = 0;  // always maintain a null-terminated string
						}
						// Clear the response buffer and send the text to the cell
						uart1_flush_inbuffer();
						cell_send_string(json_name);
					}
				}
			}
			else
			{
				// accumulate characters in the json_name buffer.  Note this is limited to 50 characters
				if (json_name_cnt < JSON_NAME_SIZE)
				{
					json_name[json_name_cnt++] = ch;
					json_name[json_name_cnt] = 0;  // always maintain a null-terminated string
				}
				else
				{
					json_name[0] = 0;  // discard command
					AppendStatusMessage("Error: [] command exceeds json_name buffer size");
				}
			}
			prev_ch = ch;  // always maintain the previous character
			break;
		default:
			json_step = 0;  // Unexpected state, return to start state
			break;
	};

	return JSON_INCOMPLETE;
}

void wifi_parse_incoming_JSON(void)
{
	uint8_t ch;

	//while (uart2_getchar(&ch))
    if (uart2_getchar(&ch))
	{

	    if (ch == 0)
	    {
	    	trace_AppendMsg("Should not happen");
	    }
		// Process the extracted character.  Complete name/value pairs are executed from right to left.
		// However, certain commands will only be performed at a higher level once the JSON closing bracket is detected.
		if (add_char_to_JSON_string((char) ch) == JSON_COMPLETE)
		{
			//Extend WiFi timeout since a complete JSON was received
			main_extend_timeout();
			// leave other characters in the queue for processing next round
			//break;
		}
	}

}


int8_t cell_parse_incoming_JSON(const char *p)  // the cell JSON string is null-terminated
{
	char ch;

	while (*p)
	{
		// Extract the next character to be processed
		ch = *p;

		//zg the spy should have already taken place before this point
		//if (sd_UART_5SPY1) lpuart1_putchar((uint8_t *)&ch);   // Echo incoming characters to UART4

		// Advance to next character in buffer (if any)
		p++;

		// Process the extracted character
		if (add_char_to_JSON_string(ch) == JSON_COMPLETE)
		{
			break;  // leave any other received characters in the buffer for next pass
		}
	}

	// Note that commands will not be performed because there is no wifi connection open.
	// But, just to be safe, defeat any that might have been sent over
	json_complete = 0; // MAIN LOOP PROCESSING FOR USER PAGE CLICKS.

	// Note that changes to configuration values (saveMainConfig, saveCellConfig) will be saved elsewhere

	return 0;
}
