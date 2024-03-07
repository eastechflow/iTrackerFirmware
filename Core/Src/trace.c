/*
 * trace.c
 *
 *  Created on: Mar 12, 2020
 *      Author: Zachary
 */
#include "string.h"
#include "stdio.h"
#include "Common.h"
#include "WiFi_Socket.h"
#include "fatfs.h" // SD Card
#include "sdmmc.h" // SD Card
#include "rtc.h" // SD Card

#include "trace.h"
#include "factory.h"
#include "debug_helper.h"
#include "lpuart1.h"
#include "uart4.h"

char TraceBuffer[TRACE_BUFFER_SIZE];




void trace_Init(void)
{
	TraceBuffer[0]=0;
}


static void save_trace_buffer(void)
{
	FIL sdFileName;
	FRESULT sd_res;
	UINT sdCardBytes;
	int len;

	len = strlen(TraceBuffer);

	if (len == 0) return;  // nothing to save

	//zg if (sd_RunningFromBank2) return;  // known problems when running from Bank 2, so just don't create the Trace file...

	//return;  // Bug Hunt esp32

	sd_res = sd_init();
	if (sd_res != FR_OK) return;

#if 0
	// This was not as useful as a single trace file

	uint32_t tmpsec;
	struct tm tm_st;
	char path[_MAX_LFN + 1];

	// Create file name.
	tmpsec = rtc_get_time();
	sec_to_dt(&tmpsec, &tm_st);

	// LONG FILENAME (up to 255)
	// For fast traces (less than 1 sec apart), must generate a unique name so the file is not overwritten.  Use the tick counter.
	sprintf(path, "TRACE_%04d-%02d-%02d_%02d-%02d-%02d-%lu.txt", tm_st.tm_year, tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec, HAL_GetTick());

	// Open the file for writing
	sd_res = f_open(&sdFileName, path, FA_WRITE | FA_CREATE_ALWAYS);
	if (sd_res != FR_OK) return;
#else
	sd_res = f_open(&sdFileName, "Trace.txt", FA_OPEN_APPEND | FA_WRITE);
	if (sd_res != FR_OK)
	{
		return;
	}
#endif

	f_write(&sdFileName, TraceBuffer, len, &sdCardBytes);

	// Close the file handle.
	f_close(&sdFileName);
}


void trace_AppendChar(char ch)
{
	int space_available;
	int space_needed;
	int trace_len;

	trace_len = strlen(TraceBuffer);

	space_available = sizeof(TraceBuffer) - trace_len - 1;
	space_needed = 2;  // for null-terminator

    if (space_needed > space_available)
    {
    	save_trace_buffer();
    	trace_Init();  // reset trace buffer
    	trace_len = 0;
    }
    // Store the character and the null terminator
    TraceBuffer[trace_len] = ch;
    TraceBuffer[trace_len+1] = '\0';


}
void trace_AppendMsg(const char *Msg)
{
	int space_available;
	int space_needed;

	space_available = sizeof(TraceBuffer) - strlen(TraceBuffer) - 1;
	space_needed = strlen(Msg) + 1;

    if (space_needed > space_available)
    {
    	save_trace_buffer();
    	trace_Init();  // reset trace buffer
    }
	strcat(TraceBuffer, Msg);  // store this trace message
}

void trace_AppendCharAsHex(char ch)
{
  char msg[5];
  //sprintf(msg,"%02X,", ch);
  sprintf(msg,"%3d,", ch);
  trace_AppendMsg(msg);
}

// AppendStatusMessage
// "Running
//
void trace_TimestampMsg(const char *Msg)  // This uses the rtc
{


	uint32_t tmpsec;
	struct tm tm_st;
	char status_msg[256];

	// Get time stamp
	tmpsec = rtc_get_time();
	sec_to_dt(&tmpsec, &tm_st);

#if 0
	static int just_once = 1;
	if (just_once)  // TODO is this OK?  There is a similar power_on_msg in FirmwareLog.txt
	{
		// Note when power on occurs
		get_power_on_msg(status_msg);

		trace_AppendMsg(status_msg);

		just_once = 0;
	}
#endif

	// Timestamped message (up to 100)

	sprintf(status_msg, "%02d-%02d-%04d %02d:%02d:%02d %s\n", tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year, tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec, Msg);
	trace_AppendMsg(status_msg);
}





void trace_sleep(void)
{
  char msg[128];
  int dec1, dec2;

  convfloatstr_4dec(ltc2943_last_voltage_reported, &dec1, &dec2);

  sprintf(msg, "Sleep: v %d.%04d", dec1, dec2);

  trace_TimestampMsg(msg);

  AppendStatusMessage(msg);
}

void trace_wakeup(void)
{
  char msg[128];
  int dec1, dec2;

  //extern uint32_t TimeToTakeReading;
  //extern uint32_t TimeAtPowerOn;
  extern uint32_t TimeAtSleepStart;
  //extern uint32_t SecondsAsleep;
  extern uint32_t PrevTimeAtWakeUp;
  extern uint32_t TimeAtWakeUp;
  extern uint32_t TimeScheduledToWakeUp;
  extern uint32_t LightSleepCount;

  convfloatstr_4dec(ltc2943_last_voltage_reported, &dec1, &dec2);


  sprintf(msg, "Wakeup: Slp %lu Sch %lu Act %lu Prv %lu Shf %lu Int %lu Cnt %lu v %d.%04d", TimeAtSleepStart,
	  TimeScheduledToWakeUp, TimeAtWakeUp, PrevTimeAtWakeUp,
	  TimeAtWakeUp - TimeScheduledToWakeUp, TimeAtWakeUp - PrevTimeAtWakeUp,
	  LightSleepCount, dec1, dec2);

  trace_AppendMsg("\n");
  trace_TimestampMsg(msg);


  AppendStatusMessage(msg);
}

void trace_SaveBuffer(void)
{
	save_trace_buffer();
	trace_Init();  // reset trace buffer
}


void TraceSensor_AppendMsg(const char *Msg)  // Specialized trace function
{
#ifdef TEST_SENSOR_MAIN_LOOP   //TestSensorMainLoop
  // Allow vectoring output to lpuart1 (Vel2) for real-time monitoring
    //lpuart1_tx_string(Msg);  // Vel2
    //uart4_tx_string(Msg);  //Vel1
    TESTSENSOR_UART_tx_string(Msg);
#else
    trace_AppendMsg(Msg);
#endif
}
