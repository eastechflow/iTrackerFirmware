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

//#define ENABLE_SLEEP_DEBUG_MODE     // enables the debugger to work across sleep events and to go to sleep as soon as possible at power-on
//#define STRESS_TEST_ESP32_WAKEUP
//#define MEASURE_SLEEP_CURRENT
// see debug_helper.h for LOG_SLEEP_WAKE

//#define TEST_LIGHT_SLEEP



//DEVID   this symbol can be used to determine which MCU the code is running on.  See ST documentation and forums

/**
 * main.c
 *
 * Main unit, initialization, clocks, and loop control.

OPERATION LED SIGNALS (ORDER OF IMPORTANCE)
	- INITIAL WIFI CONFIG - LED ON (continuously, up to 60 seconds or until connected)
	- WIFI WAKEUP - LED ON (continuously - not the initial config, should happen faster)
	- WIFI CONNECTED - 2 Flickers LED - 2 short pulses (5ms)
	- NO ACTIONS, AWAKE - 1 Flicker LED - 1 short pulse (5ms) - a heartbeat.
	- NO LED - The unit is asleep.

PROPER USAGE - HEX FILES
	- Pull the latest code from GitLab
	- Close Atollic.
	- Start the STM32 Cube Programmer and erase the target iTracker flash completely. Disconnect. Close.
	- Open Atollic, select Project/Clean, then Project/Build All.
	- (noting that Atollic must be configured for a HEX output and/or ELF)
	- Click "DEBUG" to program the erased board.
	- When the breakpoint hits the "doInit()" first debugger stop, click run.
	- Wait until the board is operating in wifi mode, then slick "stop".
	- The programming is complete. Power down the board and it will begin in "initial config" mode.

Wi-Fi RULES
    1. Power on with one long LED blink (approximately one to two seconds) indicating power applied.
    2. Once the Wi-Fi is initialized and broadcasting the SSID, blink the LED 4 to 5 times.
    3. On connect prompt for password.
    4. After connecting and entering 10.10.10.1 or eastechiq.com load the setup page.
    5. Once connected with the browser displaying the setup page, keep the Wi-Fi on for the first 10 minutes to provide enough time to complete the setup.
    6. The Setup page should have:
       - Site ID (Should be Site Name) box for entering the site name.
       - New button for resting to defaults.
       - Pipe ID box for entering the inside diameter of the pipe.
       - Dropdown selection for damping.
       - Dropdown selection for log intervals.
       - Drop down selection for US Imperial or Metric units.
       - Population ( should be Total Households) text box for entering the number of households serviced.
       - Save button to save the entered configuration.
       - Display current set time and date.
       - Sync button to synchronize the time to the mobile device.
       - Level box and send button for interning the initial level. On pressing send the four distance readings will averaged and to the entered
       	 level to calculate and store a vertical mount.
       - Command box and send button for sending commands to the MCU and Zentri module.
       - Tabs for Live, History, Logs and Settings pages.
       - Display the iTrackers serial number, firmware revision, sewerwatch revision and Zentri firmware revision
    7. The Live button should display the level and units, the number of logged results, the gain, the temperature, the battery remaining (in percent)
    	and the current distance
    8. The History button should display a hydrograph of the levels for one day initially with an option to see the hydrograph in Delta Q or Flow. Flow
    	will be in either US empirical or metric units from the users selection in the setup page. Duration options are one day, one week or one month.
    9. The Log button should have a start date (the date in the logger was last started), the last download date, the number of logged results and the
    	current logging interval. The page should also have a download button all button and a Download from last button.
    10. After the 10-minute configuration timeout check for browser activity. Keep alive for 1 min if activity is detected in previous minute.
    11. After 10-minute startup time-out and one minute of no activity put in sleep mode until the next five-minute mark of the clock (ie 10:05, 10:10).
    	On the fives between 7:00AM and 5:00PM on week days wake up the Wi-Fi and broadcast the SSID.
    12. Take a time, distance, temperature, and gain reading every X minutes (X is the selected log interval value) store them in the logger. If no distance
    	echo is detected make up to five attempts until a result is return.  Each acceptable distance measurement will be an average of three readings plus
    	the number in the damping selection of the setup.
    13. On download transmit the record numbers, times, calculate the levels (by subtracting distance from the vertical mount), distances, temperatures and
    	gains for each recorded interval.

// wifi wake first time (setup) - 10 minutes w/keepalive
// (wifi_browser_val) based on browser activity, after 1 minutes, go to disconnect/sleep
//  	with "keep alive message"
// if downloading, don't go to sleep.
// Wifi wakes up (on the 5th minutes...), waits for a period (1 minute), and then disconnects/sleeps


NOTE: THE BELOW CALCULATIONS ARE BASED ON AN AVERAGE CURRENT DRAW IN DIFFERENT STAGES OF OPERATION
NOTE: THESE MEASUREMENTS MAY CHANGE AS MORE ACCURATE TESTS OF CURRENT DRAW ARE PERFORMED
NOTE: THIS BATTERY LIFE MODEL MAY CHANGE OR BE REPLACED TO MATCH THE RESULTS OF LONG TERM TESTING OR FIELD RESULTS

CAPACITIES:

   Tadrian TL-5930
   Omnicel ER34615

  	 Battery total capacity: 16,000 mAh
               Wifi cost ea: 40 mA
            Reading cost ea: 10 mA
Low Power Mode (STOP1) cost: 4 mA

CONVERSIONS:
	 1 Amp = 1 Coulomb / Sec   so 1mA = 1mC /Sec
	 1 mAh = 3.6 Coulombs

TEST CONFIGURATION:

	      Wifi wakeup: 5 mins
	Reading intervals: 5 mins

TIMINGS (WAKEUPS):

8am - 6pm:
	120 total wakeups: WIFI and READINGS
	5days * 240 minutes awake = 1,200 minutes  @ 40mA + 10mA = 50 mA
	5days * 360 minutes sleep = 1,800 minutes @  4 mA

6pm - 8am:
	168 total wakeups: READINGS ONLY
	168 seconds awake = 2.8 minutes awake ...
	2.8 minutes awake * 5days = 14 minutes @ 10 mA
	837.2 minutes awake * 5days = 4186 minutes @ 4 mA

Sat - Sun:
	576 total wakeups: READINGS ONLY
	576 seconds awake = 9.6 minutes awake @ 10 mA
	(48hours * 60 minutes -9.6 minutes) 2870.4 minutes asleep @ 4 mA


AVERAGE WEEKLY CURRENT DRAW:
	[1200min * 50mA + 1800min * 4mA + 14min * 10mA + 4186min * 4mA + 9.6min * 10mA + 2870min * 4mA]  = 95,660 mA*min

	[1200min + 1800min + 14min + 4186min + 9.6min + 2870min] = 10,079.6 min

	Average = (95,660 mA*min)/(10,079.6 min) = 9.491 mA

BATTERY LIFE EXPECTANCY:

	Convert the battery capacity from mAh to Coulombs:
		16,000 mAh * 3.6 Coulomb/ mAh = 57,600 Coulombs

	Divide the Coulomb capacity by the Coulomb discharge rate:

		57,600 Coulomb * (1sec / 9.49 e-3 Coulomb) = 6,068,907.38 seconds

		= 1,685.80 hours
		= 70.24 days
		= 2.34 months


 */
#include "main.h"
#include "debug_helper.h" // For debugging and hacking around when testing, etc.
#include "rtc.h"
#include "adc.h"
#include "i2c.h"
#include "iwdg.h"
#include "quadspi.h"
#include "rtc.h"
#include "usart.h"
#include "uart1.h"
#include "uart2.h"
#include "uart3.h"
#include "uart4.h"
#include "lpuart1.h"
#include "gpio.h"
#include "Common.h"
#include "Gen4_max5418.h"
#include "WiFi.h"
#include "s25fl.h"
#include "Sensor.h"
#include "Log.h"
#include "DeltaQ.h"
#include "dual_boot.h"
#include "Sample.h"
#include "s2c.h"
#include "trace.h"
#include "factory.h"
#include "init.h"
#include "Hydro.h"
#include "testalert.h"
#include "pass_thru.h"
#include "usb_device.h"
#include "Gen5_AD5270.h"

//extern HAL_StatusTypeDef FLASH_WaitForLastOperation(uint32_t Timeout);

__IO uint32_t led_countdown_timer_ms = 0;
__IO uint32_t hal_led_delay_timer_ms = 0;
__IO uint32_t generic_delay_timer_ms = 0;
__IO uint32_t cell_delay_timer_ms = 0;
__IO uint32_t wifi_delay_timer_ms = 0;


__IO uint32_t UTC_countdown_timer_ms = 0;
__IO uint32_t UTC_time_s = 0;

int UTC_timezone_offset_hh = 0;


// Readings SCHEDULING
uint32_t TimeToTakeReading = 0;		// Time to take the next reading
uint32_t TimeAtPowerOn;  // Used for battery change reminder
uint32_t TimeAtSleepStart;
uint32_t SecondsAsleep;
uint32_t TimeAtWakeUp;
uint32_t PrevTimeAtWakeUp;
uint32_t TimeWiFiPoweredOn = 0;
uint32_t TimeScheduledToWakeUp;
uint32_t IsFirstWakeCycle;
uint32_t LightSleepCount;

uint16_t AccumulatorJustBeforeSleep;

// The  ENABLE_PERIODIC_WIFI_WAKEUPS flag will enable periodic WiFi wakeups (~every 5 min) after the initial power-on time.
// It is disabled due to high power consumption and customer frustration with having to wait to connect, missing connections and waiting again, etc.
// Customers simply power-cycle the device to connect and they don't have to wait.
//#define ENABLE_PERIODIC_WIFI_WAKEUPS
// 7-16-2021 zg An alternative is to wake up in "listen" mode and if a known AP is sensed, then to wake up and broadcast.  Essentially the
// user carries a "wake-up dongle" that is only used to wake-up units in the vicinity.

// Wifi SCHEDULING
uint32_t WifiIsAwake = 0;				// Tracking Wifi awake status.

__IO uint32_t WakeUpFrom_EXTI = 0;		    // Force wakeup from external source

uint32_t WifiWakeTime = 0;				// Time to wake up



//#ifdef ENABLE_SLEEP_DEBUG_MODE
#if 0
uint32_t Wifi_Sleep_Time = 0;				// Time to expire user's interactions when connected.
uint32_t WifiIdleTimeValue = 10;		// Time to allow user to be idle before disconnect.
uint32_t WifiIdleTimeExtendValue = 10;	// Time to extend Wifi idle time during user interaction.
uint32_t WifiInitialIdleTimeValue = 15;// zg for faster testing of sleep related timing
#else
uint32_t Wifi_Sleep_Time = 0;		    // Time to expire user's interactions when connected.
uint32_t WifiIdleTimeValue = 60;		// Time to allow user to be idle before disconnect.
uint32_t WifiInitialIdleTimeValue = 540;// Time to allow user to be idle before disconnect ON INITIAL SETUP ONLY.  Creates a 10 minute ON time.
uint32_t Wifi_Initial_Wake_Time = 0;
uint32_t Wifi_Initial_Sleep_Time = 0;
#endif






int g_Enable_RTC_Charging = 1;   // work around to odd problem at power on
int g_StayAwake = 0;








// Cellular SCHEDULING
//uint32_t CellWakeTime = 0;			// Time to wake up the cellular

// Private Forward Declarations
static void initialize_spy_uarts(void);
//static void de_initialize_spy_uarts(void);

static int perform_delayed_power_on_activities(void);  // returns 1 if activities remain; 0 if all activities are complete

void doSleep(void);

// Update the epoch timers for the scheduled actions.

void calc_wifi_wakeup(void)
{
#ifdef ENABLE_AUTO_WAKEUP
    // Unconditional, auto wake up to allow user easy access to iTracker via WiFi.  No need for magnet or removal of batteries.
    // App scan for BLE devices will automatically connect and send a "keep alive" packet and then disconnect.
    // This will cause unit to stay awake for 1 minute and appear in the BLE iTracker list.
	// It takes 2 sec to turn on ESP32; give App 5 sec to scan and connect; 7 sec total on time.
	// This is just an estimate.  The actual Wifi_Sleep_Time is determined by the actual esp32 power on time when the esp32 wakes up.
    WifiWakeTime  = get_next_wifi_wakeup();
	Wifi_Sleep_Time = WifiWakeTime + 1;
#else

    // original
	if ((MainConfig.wifi_wakeup_interval) && (ltc2943_get_battery_percentage() >= 5))
	{
	  WifiWakeTime  = get_next_wifi_wakeup();
	  //WifiWakeTime  = TimeToTakeReading;  // Kludge for testing on weekends, evenings, and at fast intervals
	  Wifi_Sleep_Time = WifiWakeTime + WifiIdleTimeValue;
	}
	else
	{
		WifiWakeTime = 0;
		Wifi_Sleep_Time = 0;
	}
#endif
}

void calc_wakeup_times(int IsInitializing)
{
	// Update current epoch seconds.
	//CurrSeconds = rtc_get_time();

	// Grab the current clock and scheduler settings.
	TimeToTakeReading = get_next_reading();   // zg - uses I2C to access battery info ?

	calc_wifi_wakeup();



	//TODO calculate the cell wake time


#ifdef LOG_SLEEP_WAKE
    WifiWakeTime  = get_next_wifi_wakeup();
	Wifi_Sleep_Time = WifiWakeTime + 1;
#endif

	if (SleepDelayInMinutes)
	{
        // Configure similar to initial power-on WiFi behavior
		WifiWakeTime = rtc_get_time() + 30;
		Wifi_Sleep_Time = WifiWakeTime + WifiIdleTimeValue + WifiInitialIdleTimeValue;
	}


	if (IsInitializing)
	{
        // Configure initial power-on WiFi behavior
		WifiWakeTime  = rtc_get_time();


		if (sd_SleepAtPowerOnEnabled)
		{
		  // quick sleep after 60 seconds for testing
		  Wifi_Sleep_Time = WifiWakeTime + WifiIdleTimeValue;
		}
		else
		{
			// normal behavior
			Wifi_Sleep_Time = WifiWakeTime + WifiIdleTimeValue + WifiInitialIdleTimeValue;
		}


		if (IsFirstWakeCycle)
		{
		  Wifi_Initial_Wake_Time = WifiWakeTime;
	 	  Wifi_Initial_Sleep_Time = Wifi_Sleep_Time;
		}
	}

}

void main_rtc_changed(void)
{
	// must update all time-based values to assure correct wake/sleep operation
	// Possible time shifts:
	// a. rtc is advancing in time (relative to when device was powered on)
	// b. rtc is going backwards in time (relative to when device was powered on)

	// might have to assume that changing the clock acts like a power cycle ??


	// initialize time variables related to when power-on occurred
	init_power_on_time();

	TimeWiFiPoweredOn = rtc_get_time();

	// initialize wake/sleep time variables
	calc_wakeup_times(1);

}

void main_extend_timeout(void)
{

  // Extend WiFiIdleTime by 90 seconds into the future.
  // It is tempting to detect if a WiFi connection or BT connection exists and stay awake.
  // However, if there is no activity on the connection after 10 minutes, it makes sense to
  // go to sleep (to save the battery).  This will force the user to wake up
  // the iTracker manually, but that's always the trade off.  In Gen5, the magnetic wake-up
  // feature may make this a bit easier in that you don't have to unscrew anything.

  CurrSeconds = rtc_get_time();
  WifiWakeTime  = CurrSeconds;   // just to be safe in case the RTC was changed by the UI
  Wifi_Sleep_Time = CurrSeconds +  WifiIdleTimeValue;  // Sleep after 60 seconds of idle time

  if (IsFirstWakeCycle)
  {
	  // Avoid situations where the clock has somehow gone backwards in time
	  if (Wifi_Sleep_Time > Wifi_Initial_Wake_Time) // The clock has not gone backwards
	  {
	    // Keep the "hard edge" of ten minutes
	    if (Wifi_Sleep_Time < Wifi_Initial_Sleep_Time)  Wifi_Sleep_Time = Wifi_Initial_Sleep_Time;
	  }
  }

  // the sleep command will set Wifi_Sleep_Time to zero, which occurs AFTER this function

}




// Take a live reading and send back to websocket/website.
void doTakeLiveReading()
{
	// Send back live value.
	get_sensor_measurement_0();
	wifi_send_live();
}

static void perform_hardware_reading(void)
{
	// Use simulated data if doing Cell Phone test
	if (sd_TestCellData || sd_TestCellAlerts)
	{
	    LastGoodMeasure = testalert_get_next_sim_value();

		uint8_t sim;
	    sim = (uint8_t)LastGoodMeasure;

	    //gain = 255 - (log_rec.gain & 0xff);

	    // TODO - fix this for Gen5
	    Actual_Gain = ((int)255 - ((int)sim)) & 0xff;  // try and vary gain in a repeatable way

	}
	else
	{
		// Take a measurement
		get_sensor_measurement_0();  // This logs to the SD card

	}

}

static void confirm_alert()
{

	int try_count = 0;

	if (!IsCellEnabled()) return; // alerts not active, confirmation is not required

TryAgain:

    // Check for a possible new alert
	s2c_check_for_alert(LastGoodMeasure);

	if (s2c_current_alarm_state == NO_ALARM) return;    // no alarm is confirmed, exit


	// Perform additional readings to confirm the alert is real.
	// Space the readings over a relatively large interval, say 30s to 1 min to allow potential debris to be cleared.
	// Do not allow other activities (ble or cell) during this time.
	//
	// Discard intermediate data and report only the confirmed reading in the log file.
	// For the timestamp, assign the start of the intended log interval.
    // Note that the SD card trace file may contain a record of the intermediate readings for factory use.
	//
	// Gather readings at 10, 20, 30 seconds after the first alert.  A total of four readings is needed to trigger an alert.
	// Any single reading that is not an alert will terminate the series.
	if (try_count < 3)
	{
	  HAL_led_delay(10000);
	  perform_hardware_reading();
	  try_count++;
	  goto TryAgain;
	}

	// ASSERT:  five readings spaced ten seconds apart have each reported an alert, so it must be real.
	// Continue by logging the very last reading obtained.

}


static void TakeReading(void)
{
	// Get the current time to assign as the logged datapoint timestamp, consider truncating seconds to maintain a consistent log interval
	CurrSeconds = rtc_get_time();

	if (sd_TestCellAlerts)
	{
		AppendStatusMessage("Alert start...");  // To verify, scan log for all lines containing "Alert".  It should show a sequence of readings, and what is finally logged...
	}

	// Perform a hardware reading
	perform_hardware_reading();

	// Confirm each alert by gathering additional hardware readings
	confirm_alert();

	if (sd_TestCellAlerts)
	{
		float level;

	    level = MainConfig.vertical_mount - LastGoodMeasure;
		s2c_log_alert("Alert Logged: ", LastGoodMeasure, level);  // This ends the data gathering process
	}

	// Write the log entry
	log_write(CurrSeconds, LastGoodDegF, LastGoodMeasure, Actual_Gain, MainConfig.vertical_mount, LOG_WRITE_LOG_TRACK);

	// Trigger cellular transmissions
	if (IsCellEnabled())
	{
		if (S2C_CELLULAR_MAKE_CALL())   // determine if it is time to make a cell call
		{
			// trigger a cell call here, but do not actually perform it (non-blocking)
			S2C_TRIGGER_CELLULAR_SEND();
		}

	}

}


int doTakeReading()  // returns 0 if no reading was taken, 1 if a reading was taken
{
	int result = 0;
	if (rtc_get_time() >= TimeToTakeReading)
	{
		if (sd_DataLoggingEnabled)  // Disable logging data for CSV parsing and Hydrograph Development
		{
			if (IsFirstWakeCycle)
			{
				// postpone taking and logging data points during the first wake cycle.  This includes the initial power on ping time which could be up to 30 minutes if registration fails.
			}
			else
			{
				TakeReading();  // This will log data and make cell phone calls as needed
				result = 1;
			}
		}
		TimeToTakeReading = get_next_reading();   // zg - uses I2C to access battery info ?
	}
	return result;
}



// Process user interactions in SewerWatch via WebSocket.
void wifi_process_json()
{
	// Process any Wifi actions from the user via the Web page.
	wifi_parse_incoming_JSON();  // When complete JSON strings are received, the timeout is extended

	// Process the pages clicked through the index.html (Live, History, Logs, Settings)
	if (json_complete)
	{
		int code;

		//wifi_start_transmission();

		switch (json_cmd_id)
		{

		case CMD_LIVE_VALUES:  // 1
			doTakeLiveReading();
			break;
		case CMD_AUTO_READINGS_LIVE_ONLY:  // 2
			doTakeLiveReading();
			break;
		case CMD_SLEEP:  // 3
			g_StayAwake = 0;
			WifiWakeTime = 0;
			Wifi_Sleep_Time = 0;  // go to sleep normally
			break;
		case CMD_ID:  // 4
			wifi_send_id();
			break;

		case CMD_SEND_CLOUD_PKT: // 5
			wifi_send_cloud_packet(json_cloud_packet_type, json_cloud_record_number);
			break;

#if 0
		case CMD_DEBUG:
			wifi_send_debug();
			break;
#endif

		case CMD_SETTINGS:  // 6
			wifi_send_settings();
			break;

		case CMD_STAY_AWAKE:  // 7
			g_StayAwake = 1;
			break;

		case CMD_DOWNLOAD_DIAG:  // 8
			wifi_send_download_diag(Download_type);
			break;
		case CMD_DOWNLOAD:  // 9
			code = wifi_send_download(Download_type, Download_startTime);
			if (code != 0)
			{
				wifi_send_download_error_code(code);  // zg - this sends a JSON error message.
			}
			else
			{
				wifi_send_logs();
			}
			break;
		case CMD_LOGS_PAGE:  // 11
			wifi_send_logs();
			break;
			//case CMD_999:  // zg - this appears to be a "keep alive" artifact in the UI code from Jason

		case CMD_ALARM_PAGE:  // 13
			wifi_send_alarm();
			break;

		case CMD_HISTORY_PAGE: // 14
			wifi_send_history(CurrentGraphType, CurrentGraphDays);
			break;

		case CMD_LED2_ON:  // 15
			//STATUS2_LED_ON;
			break;

		case CMD_LED2_OFF: // 16
			//STATUS2_LED_OFF;
			break;

		case CMD_RPT_ACCUM_ON:  // 17
			wifi_report_seconds = 1;
			break;

		case CMD_RPT_ACCUM_OFF: // 18
			wifi_report_seconds = 0;
			break;

		case CMD_SET_ACCUM_PCT:  // 19,pct
			ltc2943_set_battery_percentage(json_sub_cmd_id);
			// Resend the settings.
			wifi_send_settings();
			break;


		case CMD_WIFI_CONFIG:  // 21
			// Reconfigure Wifi now -- does this actually work?
			wifi_config();
			// Resend the settings.
			//wifi_send_settings();  // zg - this makes no sense, the websocket was destroyed.  The client has to re-open.
			break;
		case CMD_RESET_DEFAULTS:  // 22

			// Reset main config defaults.
			//Reset_MainConfig(DO_NOT_RESET_SERIAL_NUMBER);
			reset_to_factory_defaults();
			saveMainConfig = 1;
			saveCellConfig = 1;
			// Resend the settings.
			wifi_send_settings();
			break;

		case CMD_RTC_BATT_RECHARGE_OFF:  // 23
			SetRTCBatt(RTC_BATT_RECHARGE_OFF);
			saveMainConfig = 1;
			HAL_PWREx_DisableBatteryCharging();
			rtc_report_battery_charging_bit();
			// Resend the settings.
			wifi_send_settings();
			break;

		case CMD_RTC_BATT_RECHARGE_ON_1_5K:  // 24
			SetRTCBatt(RTC_BATT_RECHARGE_ON_1_5);
			saveMainConfig = 1;
		    HAL_PWREx_EnableBatteryCharging(PWR_BATTERY_CHARGING_RESISTOR_1_5);
			rtc_report_battery_charging_bit();
			// Resend the settings.
			wifi_send_settings();
			break;

		case CMD_CELL_NONE:  // 30 Sets cell module to NONE
			ClearCellEnabled();
			SetCellModule(CELL_MOD_NONE);
			saveMainConfig = 1;
			s2c_CellModemFound = S2C_NO_MODEM;
			break;

		case CMD_CELL_TELIT: // 31 Sets cell module to TELIT and performs a factory reset.  Also powers on cell and leaves on.
			telit_reset();  // This takes about 10 seconds
			break;

		case CMD_CELL_XBEE3: // 32 Sets cell module to XBEE3 and performs a factory reset.  Also powers on cell and leaves on.
			xbee3_reset();
			break;

		case CMD_CELL_LAIRD: // 33 {"comm":33} Sets cell module to LAIRD and performs a factory reset.  Powers cell on and then off.
			s2c_trigger_factory_config_from_ui();
			break;

		case CMD_CELL_LAIRD_ISIM: // 35 {"comm":35}  Configures internal SIM for Laird
		    SetCellEnabled();
			s2c_trigger_internal_SIM_from_ui();
			wifi_send_settings();
			break;

		case CMD_CELL_LAIRD_ESIM: // 36 {"comm":36}  Configures external SIM for Laird
		    SetCellEnabled();
			s2c_trigger_external_SIM_from_ui();
			wifi_send_settings();
			break;

		case CMD_CELL_BAND: // 38 Performs a laird cell band configure {"cell_band":n} where n = 2, 4, 12, 13, 17, etc  9001 means all USA bands, 9061 means all AU bands, 9064 means all NZ bands
		    SetCellEnabled();
			s2c_trigger_cell_band_from_ui(json_sub_cmd_id);
			wifi_send_settings();
			break;

		case CMD_CELL_PING: // 40 {"comm":40} Performs a power-on ping.
		    SetCellEnabled();
			s2c_trigger_power_on_ping_from_ui();
			wifi_send_settings();
			break;


		case CMD_BOOT_ESP32_UART4:  // 224

			// Puts ESP32 into bootload mode and bridges UART2 to UART4 (VEL1), must power-cycle to exit
			esp32_enter_bootload_mode(1);
			break;


		default:
			break;
		};

		//wifi_end_transmission();

		// Reset this flags
		json_complete = 0;
		json_cmd_id = 0;
		json_sub_cmd_id = 0;
		json_cloud_packet_type = 0;

	}

	// Wait ten seconds beyond WiFi power up time before saving config changes to flash.
	// Intended to prevent flash corruption due to user "bouncing" the wakeup magnet or batteries

	if ((saveMainConfig == 1) || (saveCellConfig == 1) || (saveSensorConfig == 1))   // most of the time these are zero
	{
		if (wifi_delay_timer_ms == 0)  // hold off until N seconds after wifi power on
		{
			// Save changes to MainConfig and CellConfig here
			if (saveMainConfig == 1)
			{
				saveMainConfig = 0;
				Save_MainConfig();
			}
			if (saveCellConfig == 1)
			{
				saveCellConfig = 0;
				Save_CellConfig();
			}
			if (saveSensorConfig == 1)
			{
				saveSensorConfig = 0;
				Save_SensorConfig();
			}
		}
	}
}

static int perform_delayed_power_on_activities(void)  // returns 1 if activities remain; 0 if all activities are complete
{
//return 0;  // Bug Hunt

	if (g_Enable_RTC_Charging == 0) return 0; // no background activities to perform

	if (wifi_delay_timer_ms) return 1; // Wait N seconds beyond WiFi power up time before starting power on background activities

	if (sd_TestCellAlerts)
	{
	  testalert_init(1);  // 0 = do not test alerts; 1 = test alerts
	}


	// For some unknown reason, enabling the RTC Battery charger early in the power-on sequence messes up the microprocessor --- badly,
	// as in can brick some units.  So... wait for all peripherals including  wifi/BLE to be up and active before turning on.
	//
	if (g_Enable_RTC_Charging == 1)
	{

#if 0  // maybe in the future all 511 boards will have rechargeables and this can be used instead of {"comm":24} in production
		  if ((BoardId == BOARD_ID_511) && (GetRTCBatt() == RTC_BATT_RECHARGE_OFF))
		  {
			  // Force battery recharging to be on, since ALL 511 boards have rechargeable batteries
			  SetRTCBatt(RTC_BATT_RECHARGE_ON_1_5);
			  saveMainConfig = 1;
			  AppendStatusMessage("BoardId requires RTC battery charging, but config not set.  Setting config.");
		  }
#endif
		  if (GetRTCBatt() == RTC_BATT_RECHARGE_ON_1_5)
		  {
			  if ((BoardId == BOARD_ID_510) || (BoardId == BOARD_ID_511) || (BoardId == BOARD_ID_SEWERWATCH_1))
			  {
				  HAL_PWREx_EnableBatteryCharging(PWR_BATTERY_CHARGING_RESISTOR_1_5);
				  AppendStatusMessage("RTC battery charging circuit is ON");
			  }
			  else
			  {
				  AppendStatusMessage("BoardId does not support RTC battery charging.  Ignoring configuration.");
			  }
		  }
		  else
		  {
			  AppendStatusMessage("RTC battery charging circuit is OFF");
		  }
		  g_Enable_RTC_Charging = 0;  // only do this once per power-on

	}

	// Indicate if any power-on activities still remain

	if (g_Enable_RTC_Charging) return 1;

    return 0;

}

//#define MEASURE_WIFI_WAKE_TIME_BATTERY_USAGE

#ifdef MEASURE_WIFI_WAKE_TIME_BATTERY_USAGE
static uint32_t debug_wifi_accumulator_uptick_counter = 0;
#endif


#if 0
static void process_wifi(void)
{
	// Process WiFi Until Idle Time expires or the sleep command is executed

	while (rtc_get_time() <= Wifi_Sleep_Time)
	{
		led_heartbeat();

		wifi_process_json();  // When cmds are received, the timeout is extended.  The sleep command will set Wifi_Sleep_Time to zero. This can take live data readings and make test cell calls.

		perform_delayed_power_on_activities();

		ltc2943_timeslice();

		doTakeReading();  // This will log data but will not make any automatic cell calls because the WiFi is active

		s2c_timeslice();  // perform cellular activities

		if (g_StayAwake)
		{
			Wifi_Sleep_Time = rtc_get_time() + WifiIdleTimeValue;
		}
	}

	// If a sleep command was executed prior to all power on activities completing, give them time to complete here
	while (perform_delayed_power_on_activities())
	{
		s2c_timeslice();  // perform cellular activities
	}


#ifdef MEASURE_WIFI_WAKE_TIME_BATTERY_USAGE
	if (LogTrack.accumulator != ltc2943_get_accumulator())
	{
		debug_wifi_accumulator_uptick_counter++;

		LogTrack.accumulator = ltc2943_get_accumulator();

		//AppendStatusMessage("Updating logtrack.accumulator before sleep.");
		//BSP_Log_Track_Write(1);  // update battery accumulator value here, just before sleeping, so the power-cycle wakeup value is as close as possible.
		//  Note that sleep current cannot be saved, consequently the power-cycle wake up accumulator value will ALWAYS BE INACCURATE.  Hopefully not by much.
	}
#endif


}
#endif


// FOR DEBUGGING - simulates the __sleep in timing and wakeups
void __haldelay(uint32_t wakecount)
{

	if (wakecount < 3) wakecount = 3;

	// Convert wakecount seconds to milliseconds
	int xx = (wakecount)*1000;
	if (xx < 0) {
		xx = 1000; // 1ms
	}
	// SIMULATE SLEEP with exact delay that the __sleep() uses.
	HAL_Delay(xx);
}

// LOW POWER DEEP SLEEP MODE
void __sleep(uint32_t wakecount)
{

	if (wakecount < 3) wakecount = 3;

	// Suspend the ticks.
	HAL_SuspendTick();

	// Deactivate the current timer.
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

    // Set the RTC alarm
	//set_rtc_alarm(rtcepoch);

	// Clear wakeup flags.
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);

	// Set the timer to wakeup.
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, wakecount, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);

	// SLEEP - BLOCKING;
	HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFI);  // original
	//HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFE);  // Test, flashed LED

	// After stop exit, reconfigure clocks.
	extern void SYSCLKConfig_STOP(void);
	SYSCLKConfig_STOP();

	// Resume ticks.
	HAL_ResumeTick();

	// Wait for shadow register updates before continuing.
	if (HAL_RTC_WaitForSynchro(&hrtc) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}
}



static void deep_sleep(uint32_t TimeToWakeUp)
{

	uint32_t sleep_ticks;

	AccumulatorJustBeforeSleep = ltc2943_get_raw_accumulator();

	TimeAtSleepStart = rtc_get_time();

#ifdef STRESS_TEST_ESP32_WAKEUP
	TimeToWakeUp = TimeAtSleepStart + 20;  // Force a known sleep interval; 10 seemed to not cause issue as often?  ESP32 not fully discharged ?
#endif

	// Assert: TimeScheduledToWakeUp and TimeAtSleepStart are correct

	if (TimeToWakeUp <= (TimeAtSleepStart+3))  // prevent wake up times in the past and enforce a minimum of 3 seconds sleep time to avoid knife edge situations
		sleep_ticks = 3;
	else
		sleep_ticks = TimeToWakeUp - TimeAtSleepStart;

#ifdef ENABLE_SLEEP_DEBUG_MODE
	// NO SLEEP HACK FOR DEBUGGING
	// FULLY SIMULATES THE __sleep() low power mode
	__haldelay(sleep_ticks);
#else
	// USE RTC ALARM AND PWR STOP INSTEAD
	// BLOCKING!!!!!
	__sleep(sleep_ticks);
#endif

	//STATUS2_LED_ON;

	// Save the time at which wake-up occurred and compute the seconds asleep
	PrevTimeAtWakeUp = TimeAtWakeUp;
	TimeAtWakeUp = rtc_get_time();
	SecondsAsleep = TimeAtWakeUp - TimeAtSleepStart;

	// Estimate the amount of energy used during deep sleep (see comment in LTC2943.h)
	ltc2943_correct_for_sleep_current(AccumulatorJustBeforeSleep, SecondsAsleep);
}


	/* NOTES FROM light_sleep() development 02-28-2023 zg
	 *
	 * 1. The LTC2943 does not power on when only USB power is applied.  Don't know why.  I2C reports HAL_ERROR.
	 * 2. When powering on from battery or bench supply, chip is initialized.  Subsequently adding USB external power and then
	 *    removing the battery keeps the chip talking at all times.
	 * 3. Power on with battery, wait until deep sleep is entered, then remove batteries.  Chip is held up by something, possibly capacitance or I2C lines back-feeding.
	 *    In any case, the chip talks just fine.  Can use that fact to trigger voltage conversions and monitor if voltage drops.
	 *
	 * The energy consumed by light_sleep() appears to be on the order of deep_sleep().  That is, in a 5 hour test the accumulator
	 * only changed by 2 counts.  Since deep_sleep() is estimated to consume one count every 2.2 hrs, the energy consumed by light_sleep() was not measurable
	 * with the iTracker and LTC2943 circuit I had.
	 */


//#define ENABLE_VISUAL_INDICATORS

static int light_sleep(uint32_t TimeToWakeUp)
{
	int count = 0;
	float battery_voltage_at_light_sleep_start;
	float battery_voltage;

	uint32_t time_to_wake_from_light_sleep;

	// Note:  a previous method tried to overlap the adc conversions with deep_sleep().  However, that is counterproductive
	// as when it wakes up, the last voltage measurement is 3 seconds in the past, which tends to produce a 6-second (2-cycle) lag
	// in detecting the voltage drop.

	// Determine the voltage right now, before going to sleep
	battery_voltage_at_light_sleep_start = ltc2943_get_voltage(0);  // if ltc2943 is not on-line, zero voltage is reported

	sleep_again:

	time_to_wake_from_light_sleep =  rtc_get_time() + 3;

	// ENTER DEEP SLEEP HERE

	deep_sleep(time_to_wake_from_light_sleep);   // perform deep_sleep() adjusting accumulator as needed

	// AWAKE FROM DEEP SLEEP HERE

	if (WakeUpFrom_EXTI) return count;   // exit light sleep when the EXTI is triggered

    // ASSERT:  TimeAtWakeUp has been set by deep_sleep().  However, due to possible delays caused by ltc2943 functions retrieving battery voltage, it is not used here

	battery_voltage = ltc2943_get_voltage(1);  // Blink a single led once, either red or yellow depending on battery capacity

	ltc2943_Enforce_HARDSTOP_Voltage_Limit(0);  // do not read voltage again, use last value


	// Visually indicate some battery capacity info


    if (ltc2943_last_percentage_reported <= 2)  // Hard Stop 3. No data logging.  Must power cycle to wake up.
    {
    	led_hard_stop_3();

		count++;
		goto sleep_again;

    }

	// End of visual indicators for battery capacity


	if (battery_voltage > battery_voltage_at_light_sleep_start) battery_voltage_at_light_sleep_start = battery_voltage;  // Just in case, to keep things in sync

	if (battery_voltage < battery_voltage_at_light_sleep_start)
	{
		float voltage_drop;
		voltage_drop =  battery_voltage_at_light_sleep_start - battery_voltage;
		//if (voltage_drop > 0.6)  // worked in about 2 cycles
		if (voltage_drop > 0.1)  // worked in about 2 cycles
		{
#ifdef ENABLE_VISUAL_INDICATORS
			STATUS_LED_ON;
			STATUS2_LED_ON;
			HAL_Delay(0);
			STATUS_LED_OFF;
			STATUS2_LED_OFF;
#else
			STATUS_LED_ON;
			STATUS2_LED_ON;
			//HAL_Delay(0);
			HAL_Delay(3000);  // give the eye time to see it
			STATUS_LED_OFF;   // leave in OFF state for wake up
			STATUS2_LED_OFF;  // leave in OFF state for wake up
			WakeUpFrom_EXTI = 1;  // force same wake up behavior as EXTI (e.g. turn on WiFi for 1 minute or so)
			return count;  // wake up to drain the capacitors, the batteries have probably been removed
#endif
		}
		else
		{
#ifdef ENABLE_VISUAL_INDICATORS
			STATUS2_LED_ON;
			HAL_Delay(0);
			STATUS2_LED_OFF;
#endif
		}
	}

#ifdef ENABLE_VISUAL_INDICATORS
	HAL_Delay(500);
#endif


	CurrSeconds = rtc_get_time();

	//if (TimeScheduledToWakeUp <= (CurrSeconds+3))  // prevent wake up times in the past and enforce a minimum of 3 seconds sleep time to avoid knife edge situations
	if (CurrSeconds < TimeToWakeUp)  // This will always wait until at least TimeToWakeUp and up to 3 seconds past that due to roundup in _sleep() function
	{
		count++;

#ifdef ENABLE_VISUAL_INDICATORS
		// This sequence yielded nine blinks before stopping when power was removed (including the 500 ms delay above)
		STATUS_LED_ON;
		HAL_Delay(0);
		STATUS_LED_OFF;
#endif

		goto sleep_again;
	}

    return count;  // to allow for debugger to monitor sleep behavior
}


#if defined( LOG_SLEEP_WAKE)  || defined (TEST_LIGHT_SLEEP)
static void log_accumulator_data_point(uint16_t SleepOrWake)  // 0 = going to sleep; 1 = waking up
{
  uint32_t time;
  float degF;
  float distance;
  uint16_t gain;
  float vertical_mount;

  //CSV File output is:
  //Record #,DateTime,Level,Distance,Gain,Temp,Batt
  //so put SleepWake next to Batt  for easy reference
  //can put other info in other fields as well, for now just use 1

  time = rtc_get_time();
  degF = SleepOrWake;    // <<<<=== Sleep or Wake Flag
  distance = 1;
  gain = 1;
  vertical_mount = 1;
  log_write(time, degF, distance, gain, vertical_mount);
}
#endif

static void update_log_track_battery_accumulator_before_sleep(void)
{
	// Note: DO NOT update the LogTrack when running on USB power, or if the LTC2943 is not initialized,
	if (ltc2943_update_log_track())
	{
		// Always check here if the battery accumulator needs to be updated before sleeping (to allow for more accurate power-cycling wake ups).
		// During the initial "awake" period, data points are not usually logged, which means the logtrack battery accumulator value is not updated.
		// If the unit goes to sleep without saving the accumulator, a power-cycle will use an old logtrack value and essentially be wrong at power on.
		//
		// Under normal data logging, the accumulator is updated with each data point, which is usually the same when we get here.
		// The if (ltc2943_update_log_track()) prevents unneeded writes because the LogTrack.accumulator value was not changed.

		AppendStatusMessage("Updating logtrack.accumulator before sleep.");
		BSP_Log_Track_Write(0);  // update battery accumulator value here, just before sleeping, so the power-cycle wakeup value is as close as possible.
		// Note that sleep current cannot be saved, consequently a power-cycle wake up accumulator value will ALWAYS BE INACCURATE.  Hopefully not by much.
		// Note that the MAGNET reset wake-up is NOT a power-cycle wake up and the LTC2943 is not affected.
		// Note this can take about 15 seconds to complete !
	}
}


#if 1
// Put the CPU to sleep
void doSleep()
{


	//STATUS2_LED_OFF;

	// Ignore any wakeup interrupts that may have occurred while awake.  The user can perform another one if needed.
	WakeUpFrom_EXTI = 0;

	if (sd_WiFiWakeLogEnabled)
	{
		trace_sleep();
	}

	// If the rtc has not been reset by connecting to the App, then:
	// 1) create a duplicate log record of the last entry (if any).
	// 2) sync the RTC to the last value in the log file
	// The RTC will continue to run normally on main batteries until the
	// next power-cycle.  At least the log file will have recoverable data
	// and a known indication that a power-cycled occurred without an RTC sync.
	if (rtc_RequiresSync)
	{
		if (LogTrack.log_counter > 0)
		{
			AppendStatusMessage("RTC battery depleted. Syncing RTC to last log and creating duplicate log record.");
			log_duplicate_last_entry();
			log_sync_rtc_from_last_entry();
			AppendStatusMessage("RTC sync complete.");
		}
		rtc_RequiresSync = 0; // At this point, the RTC is either synced to the last log entry, or to 1/1/2018.
	}



	// SleepDelayInMinutes
	// To support deployment in the field, the sleep command can be issued with a delay in minutes
	// to allow the manhole cover to be replaced before issuing the power-on-ping.
	// This feature allows confirmation of cell transmission before leaving the site.
	// If the power-on-ping is not observed in the cloud, the manhole cover will be removed and
	// the cell antenna re-positioned.  The unit then must be woken up so that WiFi can be used to
	// issue the sleep command again.  To avoid having to remove the unit to wake it up, the
	// sleep delay will also schedule the unit to wake up in WiFi mode.  If everything worked ok
	// the unit will fall asleep normally.

	if (SleepDelayInMinutes)
	{
		uint32_t delay_ms;

		if (sd_WiFiWakeLogEnabled)
		{
			char msg[128];
			sprintf(msg, "\nSleepDelayInMinutes %u", SleepDelayInMinutes);
			trace_AppendMsg(msg);
			sprintf(msg, "\nStartDelay %lu", rtc_get_time());
			trace_AppendMsg(msg);
		}

		// wait a few minutes before going to sleep
		delay_ms = SleepDelayInMinutes * 60L * 1000L;

		HAL_led_delay(delay_ms);      //  <==================== This delay occurs with Cell powered ON

		if (sd_WiFiWakeLogEnabled)
		{
			char msg[128];
			sprintf(msg, "\nStopDelay %lu", rtc_get_time());
			trace_AppendMsg(msg);
		}
	}


	// Always make sure the cell modem is powered off here
	//S2C_POWERDOWN();

	if (sd_UART_5SPY1)  // VEL2 spies on CELL
	{
		char msg[80];
		sprintf(msg,"\nSleep.  log_counter = %lu\n", LogTrack.log_counter);
		lpuart1_tx_string(msg);
	}

	// Always make sure to save any config settings to flash if pending
	if (saveMainConfig == 1)
	{
		saveMainConfig = 0;
		Save_MainConfig();
	}
	if (saveCellConfig == 1)
	{
		saveCellConfig = 0;
		Save_CellConfig();
	}
	if (saveSensorConfig == 1)
	{
		saveSensorConfig = 0;
		Save_SensorConfig();
	}


	// Report UART buffer sizes here
	check_and_report_uart_buffer_overflow();  // Errors are unconditionally reported

	if (sd_UartBufferSizeLogEnabled)  // Actual sizes are reported only when asked
	{
		char msg[75];
		sprintf(msg, "U1(%d,%d) U2(%d,%d) U3(%d,%d) U4(%d,%d) U5(%d,%d)",
				uart1_inbuf_max, uart1_outbuf_max,
				uart2_inbuf_max, uart2_outbuf_max,
				uart4_inbuf_max, uart4_outbuf_max,
				uart4_inbuf_max, uart4_outbuf_max,
				lpuart1_inbuf_max, lpuart1_outbuf_max);
		AppendStatusMessage(msg);
	}


#ifdef MEASURE_SLEEP_CURRENT
	//MainConfig.log_interval = 10800;  // Once every 3 hrs
	//MainConfig.log_interval = 36000;  // Once every 10 hrs
    MainConfig.log_interval = 86400;    // Once every 24 hrs
#endif

#ifdef LOG_SLEEP_WAKE
	//MainConfig.log_interval = 10800;  // Once every 3 hrs
	//MainConfig.log_interval = 36000;  // Once every 10 hrs
    MainConfig.log_interval = 86400;    // Once every 24 hrs
#endif



	// Update the schedule. Should be the next wake up(s)
	calc_wakeup_times(0);  // Side effects:  sets globals: TimeToTakeReading, WifiWakeTime, WiFi_Sleep_Time, s2c_Wake_Time

	// Determine which item needs to wake up first and set an alarm.
	// reading -> cellular -> wifi
	long t = TimeToTakeReading;


	// Note: SleepDelay will always precede taking a data point.

	if (MainConfig.wifi_wakeup_interval || SleepDelayInMinutes)
	{
		if (WifiWakeTime < t)
		{
			t = WifiWakeTime;
		}
	}
	else
	{
		WifiWakeTime = 0;
	}

	if (s2c_Wake_Time > 0)
	{
		if (s2c_Wake_Time < t)
		{
			t = s2c_Wake_Time;
		}
	}


	TimeScheduledToWakeUp = t;

	if (sd_WiFiWakeLogEnabled)
	{
		char msg[128];
		sprintf(msg, "log %lu wifi %lu cell %lu Sched %lu", TimeToTakeReading, WifiWakeTime, s2c_Wake_Time, TimeScheduledToWakeUp);
		AppendStatusMessage(msg);
	}

	trace_SaveBuffer();

	uart3_enter_low_power_mode();

	//de_initialize_spy_uarts();  // not quite right yet

	//flash_enter_low_power_mode();

	STATUS_LED_OFF;  // make sure both LEDs are off before going to sleep
	STATUS2_LED_OFF;

	UTC_countdown_timer_ms = 0;  // disable estimated UTC

    //ltc2943_get_voltage();  // attempt to read a bunch of info from the chip (chip may not be online)

	update_log_track_battery_accumulator_before_sleep();  // This calls ltc2943_get_voltage()



#ifdef MEASURE_WIFI_WAKE_TIME_BATTERY_USAGE
	// Get the current time
	CurrSeconds = rtc_get_time();

	// Write a diagnostic log entry
	LastGoodDegF = 1.0;
	LastGoodMeasure = debug_wifi_accumulator_uptick_counter;
	log_write(CurrSeconds, LastGoodDegF, LastGoodMeasure, Actual_Gain, MainConfig.vertical_mount);
#endif



#ifdef LOG_SLEEP_WAKE
	log_accumulator_data_point(0);  // 0 = going to sleep
#endif


	if (0)
	{
		deep_sleep(TimeScheduledToWakeUp);   // see if PA8 will wake it up
	}
	else
	{
	  LightSleepCount = light_sleep(TimeScheduledToWakeUp);
	}

	//led_red_and_yellow_blink();  // awake indicator here ?

	IsFirstWakeCycle = 0;  // might hoist this to the new main loop?  The initial power on ping could extend over a lot of data points...


#ifdef LOG_SLEEP_WAKE
	log_accumulator_data_point(1);  // 1 = awake
#endif

	//initialize_spy_uarts();

	if (sd_UART_5SPY1)  // VEL2 spies on CELL
	{
		char msg[80];
		sprintf(msg,"Wakeup.  log_counter = %lu\n", LogTrack.log_counter);
		lpuart1_tx_string(msg);
	}

	if (sd_WiFiWakeLogEnabled) trace_wakeup();

	//if (sd_WiFiWakeLogEnabled) trace_AppendMsg("\nExit doSleep");


	if (WakeUpFrom_EXTI)
	{
		WakeUpFrom_EXTI = 0;
        // Configure initial power-on WiFi behavior?
		// Or just a 1 minute on time ?
		WifiWakeTime  = TimeAtWakeUp;
		Wifi_Sleep_Time = WifiWakeTime + WifiIdleTimeValue + WifiInitialIdleTimeValue;
		//STATUS2_LED_ON;

		AppendStatusMessage("EXTI Wakeup.");
	}

	trace_SaveBuffer();

}
#endif



#if 0
void time_it()
{

	uint32_t elapsed_start;
	uint32_t elapsed_stop;
	uint32_t elapsed_time;

	elapsed_start = rtc_get_time();

	//BSP_QSPI_Erase_Chip();  // took 48 seconds

#if 0
	for (int i=0; i < 100; i++)
	{
	  // put function here
	  get_sensor_measurement_0();  // 100 tries took 172 seconds which is 1.72 sec/meas

	}
#endif

#if 1
	uint32_t readAddr;
	readAddr = LOG_BOTTOM;

	// Graphs request data by days:  1, 7, or 30
	// If 15 min/data point
	//  1 day  = xxx records
	//  7 days = xxx records
	// 30 days= 2,880 records  about 9 seconds.

	//for (int i=0; i < 1000; i++)
	while (readAddr < LOG_TOP)
	{
	  uint8_t sector_data[640];

	  // read contents of first log data sector
	  //log_readstream_ptr((log_t *)&sector_data, readAddr);  // 1000 tries took 3 seconds.  0.03 s/record

	  BSP_QSPI_Read((uint8_t *)sector_data, readAddr, sizeof(sector_data));

	  readAddr += sizeof(sector_data);

	}
#endif

	elapsed_stop = rtc_get_time();
	elapsed_time = elapsed_stop - elapsed_start;

}
#endif

#if 0
static void time_rtc(void)
{
	uint32_t tickstart;
	uint32_t duration;
	tickstart = HAL_GetTick();

	rtc_get_time();

	// kludge for breakpoint to read duration
	duration = HAL_GetTick() - tickstart;
	if (duration > 0) duration = 1;
}
#endif
#if 0
static void test_byte(void)
{
	int i;
	int8_t  i8;
	uint8_t u8;
	uint8_t u8b;
	char msg1[80];
	char msg2[80];
	for (u8 = 0x00; u8 < 0xFF; u8++)
	{
		i8 = u8;
		sprintf(msg1,"u8  = %u, u8  = %u, i8 = %d\n", u8, (uint16_t) u8, i8);
		trace_AppendMsg(msg1);
		u8b = i8;
		sprintf(msg2,"u8b = %u, u8b = %u, i8 = %d\n\n", u8b, (uint16_t) u8b, i8);
		trace_AppendMsg(msg2);
	}
	i8 = u8;
	sprintf(msg1,"u8  = %u, u8  = %u, i8 = %d\n", u8, (uint16_t) u8, i8);
	trace_AppendMsg(msg1);
	u8b = i8;
	sprintf(msg2,"u8b = %u, u8b = %u, i8 = %d\n\n", u8b, (uint16_t) u8b, i8);
	trace_AppendMsg(msg2);
	trace_SaveBuffer();

	i8 = 9;
}
#endif

#if 0
static void explain_tso(void)
{
	union {
		char c[4];
		uint32_t value;
	} test_tso;

	union {
		char c[4];
		uint32_t value;
	} test_tso2;

	char msg[50];
	char msg2[50];

	test_tso.value = 4627636L;
	sprintf(msg, "Expected: %lu, %2x %2x %2x %2x\n", test_tso.value, test_tso.c[0], test_tso.c[1], test_tso.c[2], test_tso.c[3]);
	//trace_AppendMsg(msg);

	test_tso2.value = 1943415L;
	sprintf(msg2, "Actual: %lu, %2x %2x %2x %2x\n", test_tso2.value, test_tso2.c[0], test_tso2.c[1], test_tso2.c[2], test_tso2.c[3]);
	//trace_AppendMsg(msg);

	// adjustment to tso:  add 2684221

}
#endif

#ifdef TEST_SENSOR_MAIN_LOOP   //TestSensorMainLoop

// See Sensor.c line 42 for a table relating gain to voltage

//#define	TEST_GAIN_MIN   3  // original value for iTracker
#define	TEST_GAIN_MIN    0      // for Mark Watson testing
#define TEST_GAIN_MAX 1023     // 251
#define TEST_GAIN_MID   40     // use 300 for the longer distances; midpoint of 8.255 volts, wiper pos index of 598




uint16_t test_gain = TEST_GAIN_MID;
static int perform_auto = 0;
static int run = 1;


static void process_keystrokes(void)
{
	int new_gain = test_gain;
	while (TESTSENSOR_UART_inbuf_tail != TESTSENSOR_UART_inbuf_head)
	{
		uint8_t ch;

		// ASSERT:  there is at least one rx char in the buffer

		// Extract the next character to be processed
		ch = TESTSENSOR_UART_inbuf[TESTSENSOR_UART_inbuf_tail];

		// Advance to next character in buffer (if any)
		TESTSENSOR_UART_inbuf_tail++;

		// Wrap (circular buffer)
		if (TESTSENSOR_UART_inbuf_tail == sizeof(TESTSENSOR_UART_inbuf))
			TESTSENSOR_UART_inbuf_tail = 0;

		// Consume the character1
		if      (ch == '1') new_gain -= 1;
		else if (ch == '2') new_gain += 1;
		else if (ch == '3') new_gain -= 5;
		else if (ch == '4') new_gain += 5;
		else if (ch == '5') new_gain -= 10;
		else if (ch == '6') new_gain += 10;
		else if (ch == '7') new_gain -= 50;
		else if (ch == '8') new_gain += 50;
		else if (ch == 'a') perform_auto = 1;
		else if (ch == 'r') run = 1;

		if (new_gain < TEST_GAIN_MIN) new_gain = TEST_GAIN_MIN;
		if (new_gain > TEST_GAIN_MAX) new_gain = TEST_GAIN_MAX;

	}

	test_gain = new_gain;
}

static void delay_and_watch_for_keystrokes(int delay_ms)
{
	hal_led_delay_timer_ms = delay_ms + 1;  // to ensure at least the number of requested ms...
	while (hal_led_delay_timer_ms)
	{
		led_heartbeat();
		process_keystrokes();
	}
}

static void manual_gain_loop(void)
{
	sensor_power_en(ON);

    while (1)
    {

      test_sensor_at_gain(test_gain);

      //HAL_led_delay(500);
      delay_and_watch_for_keystrokes(500);

    }
}


static void auto_gain_loop(void)
{
	sd_TraceSensor = 1; // Insert SD card with file named Enable_TraceSensor.txt to turn this flag on/off

	while (1)
	{
		get_sensor_measurement_0();

	    HAL_led_delay(1000);
	}
}

static void manual_and_auto_loop(void)
{
	while (1)
	{
		if (run)
		{
			if (perform_auto)
			{
				sensor_power_en(OFF);
				HAL_led_delay(100);

				sd_TraceSensor = 1;
				get_sensor_measurement_0();
				sd_TraceSensor = 0;

				perform_auto = 0;

				run = 0;   // pause until an 'r' is pressed to allow screen to be captured
				TESTSENSOR_UART_tx_string("\nPaused.   Press r to continue...\n");
			}
			else
			{
				// Perform manual readings as before
				sensor_power_en(ON);
				test_sensor_at_gain(test_gain);
			}
		}

	    delay_and_watch_for_keystrokes(500);
	}
}

static void TestSensorMainLoop(void)
{


	//MX_UART1_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_NONE);  // This initializes uart1 for the initial talking to the PC
	TESTSENSOR_MX_UART_Init(115200, UART_STOPBITS_1);  // This initializes uart for talking to the PC
    HAL_led_delay(5);  // give the uart a little time
    TESTSENSOR_UART_flush_inbuffer();
	{
		char msg[80];
		sprintf(msg, "\nTestSensorMainLoop: FW Version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);
		TESTSENSOR_UART_tx_string(msg);
	}

    TESTSENSOR_UART_tx_string("Hello from TestSensorMainLoop\n");

	UNUSED(manual_gain_loop);
	UNUSED(auto_gain_loop);
	UNUSED(manual_and_auto_loop);

	//manual_gain_loop();
	//auto_gain_loop();
	manual_and_auto_loop();

}
#endif


#if 0
static void TestLaird(void)
{
	MX_USART1_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_RTS_CTS);  // This initializes uart1 for the initial talking to the Laird Modem
    HAL_led_delay(5);  // give the uart a little time
	uart1_flush_inbuffer();
	while (1)
	{
		while (uart1_inbuf_tail != uart1_inbuf_head)
		{
			int i;

			i = uart1_inbuf[uart1_inbuf_head];
			UNUSED(i);
		}
	}
}
#endif



/* Factory Test Main Loop
 *
 */
static void FactoryTestMainLoop(void)  // 5-17-2022 zg I don't think this is ever used by Production
{
	// The info from the factory test file has already been parsed by the time we get here and Zentri has the DUT SSID.
	int i;
	int led_pattern;

	// Must power-on the wifi module here  --- TODO what happens when using Bluetooth ?
    wifi_wakeup();

	for (i=0; i < FactoryTest.number_of_readings; i++)
	{
		TakeReading();

		if (FactoryTest.status == FACTORY_TEST_FAILED) break;

	    doSleep();
	}

	// Save the log to a CSV file based on MAC address
	wifi_send_download(DL_TYPE_SDCARD, 0);  // note: filename is created in a lower level routine


	if (FactoryTest.status == FACTORY_TEST_FAILED)
	{
		led_pattern = LED_PATTERN_FAST_RED;
		strcat(MainConfig.SSID,"%s_FAILED");  // SSID is only 20 characters long
	}
	else
	{
		led_pattern = LED_PATTERN_SLOW_RED;
	    strcat(MainConfig.SSID,"%s_PASSED");
	}

	saveMainConfig = 1;

	// Re-config WiFi to match DUT Status
	wifi_config();  // Extracts MAC address and assigns SSID from config.  Leaves in STREAM mode (socket connected)

	// Set led pattern here because reconfig_wifi leaves it in the WiFi active state...
	led_set_pattern(led_pattern);

	while (1)
	{
		led_heartbeat();

		wifi_process_json();  // When bytes are received, the timeout is extended.  The sleep command will set Wifi_Sleep_Time to zero.
	}

}

static void initialize_spy_uarts(void)
{
  if (isGen4()) return;   // Gen4 cannot spy

  if (sd_UART_1SPY5)
  {
    MX_USART1_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_NONE);
    sd_UART_5SPY1 = 0;  // mutually exclusive
	HAL_led_delay(5);  // give the uart a little time
	uart1_tx_string("Hello from UART1 (CELL).  CELL spies on VEL2\n");
  }
  if (sd_UART_5SPY1)
  {
    MX_LPUART1_UART_Init(115200, UART_STOPBITS_1);
    sd_UART_1SPY5 = 0;  // mutually exclusive
	HAL_led_delay(5);  // give the uart a little time
	lpuart1_tx_string("Hello from LPUART1 (VEL2).  VEL2 spies on CELL\n");
  }
  if (sd_UART_5SPY2)
  {
    MX_LPUART1_UART_Init(115200, UART_STOPBITS_1);
	HAL_led_delay(5);  // give the uart a little time
	lpuart1_tx_string("Hello from LPUART1 (VEL2).  VEL2 spies on WiFi\n");
  }
  if (sd_UART_2SPY4)
  {
    MX_USART2_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_NONE);
    sd_UART_4SPY2 = 0;  // mutually exclusive
	HAL_led_delay(5);  // give the uart a little time
	uart2_tx_string("Hello from UART2 (WIFI).  WIFI spies on VEL1\n");
  }
  if (sd_UART_4SPY2)
  {
    MX_UART4_UART_Init(115200, UART_STOPBITS_1);
    sd_UART_2SPY4 = 0;  // mutually exclusive
	HAL_led_delay(5);  // give the uart a little time
	uart4_tx_string("Hello from UART4 (VEL1).  VEL1 spies on WIFI\n");
  }
}

#if 0
static void de_initialize_spy_uarts(void)
{
	if (sd_UART_1SPY5)
	{
		HAL_UART_MspDeInit(&huart1);
	}
	if (sd_UART_5SPY1)  // VEL2 spies on CELL
	{
		char msg[80];
		sprintf(msg,"Bye from LPUART1 (VEL2).  log_counter = %lu\n", LogTrack.log_counter);
		lpuart1_tx_string(msg);
		HAL_UART_MspDeInit(&hlpuart1);
	}
	if (sd_UART_2SPY4)
	{
		HAL_UART_MspDeInit(&huart2);
	}
	if (sd_UART_4SPY2)  // VEL1 spies on WIFI
	{
		HAL_UART_MspDeInit(&huart4);
	}
}
#endif




int wifi_timeslice(void)   //returns 1 if busy, 0 if idle
{
	uint32_t now;
	now = rtc_get_time();
	if ((now >= WifiWakeTime) && (now <= Wifi_Sleep_Time))
	{
		if (wifi_wakeup())   // If power is already on, this does nothing.  Otherwise, blocks when powering on.
		{

		  wifi_process_json();  // When cmds are received, the timeout is extended.  The sleep command will set Wifi_Sleep_Time to zero. This can take live data readings and make test cell calls.
		}
        return 1; // busy
	}
	else
	{
		wifi_sleep();   // If left powered on, esp32 draws about 20 mA during sleep

		calc_wifi_wakeup();  // Schedule next wifi wakeup

		return 0; // idle
	}
}

void manage_leds(void)
{
	// Manage leds here.
	// Want to indicate: WiFi Active, Cell Active, and when Battery Capacity is less than 5%.
	//
	// o....o....o....o     WiFi only          Single
	// o....oo...o....oo    WiFi and Cell      Single,Double alternating
	// oo...oo...oo...oo    Cell only          Double
	// ooo..ooo..ooo..ooo   Data log only      Triple
	//
	// Red = battery capacity > 5%
	// Yellow = battery capacity <5%

	// data logging is always active (except for 10 minutes after power on) and has no specific light pattern,
	// in fact, both leds are dark during a measurement

	if (wifi_is_active() && s2c_is_cell_active())
	{
		if (ltc2943_last_percentage_reported > 5)
		  led_set_pattern(LED_PATTERN_DOUBLE_RED);   // wifi active and cell call in progress.   LED_PATTERN_SINGLE_DOUBLE_RED
		else
		  led_set_pattern(LED_PATTERN_DOUBLE_YEL);   // wifi active and cell call in progress.   LED_PATTERN_SINGLE_DOUBLE_YEL
	}
	else if (s2c_is_cell_active())
	{
		if (ltc2943_last_percentage_reported > 5)
		  led_set_pattern(LED_PATTERN_TRIPLE_RED);   // no wifi, cell call in progress
		else
		  led_set_pattern(LED_PATTERN_TRIPLE_YEL);
	}
	else
	{
		if (ltc2943_last_percentage_reported > 5)
		  led_set_pattern(LED_PATTERN_SINGLE_RED);  // wifi active
		else
		  led_set_pattern(LED_PATTERN_SINGLE_YEL);
	}

}

/**
 * MAIN LOOP
 *
 * OVERVIEW
 *
 * There a (3) basic behaviors:
 *
 * 1. Take periodic readings and log them.
 * 2. Check for a Wifi user connection for configuring the iTracker and downloading logs.
 * 3. Send the readings to the cellular radio.
 *
 * Each function depends on a schedule which is as follows:
 *
 * 1. Take periodic readings are according to the configured frequency in minutes
 * 2. Transmit the logs one time per day at midnight.
 * 3. Wifi wakes up on every 5th minute for a maximum of 10 minutes.
 * 4. Wifi stays connected as long as user is connected
 * 5. Go into a low-power mode until the next wakeup.
 *
 *
 */

#if 0
static void kluge_set_sn_and_site(void)
{
  strcpy(MainConfig.serial_number, "1710004");
  strcpy(MainConfig.site_name, "Sheridan");
  saveMainConfig = 1;
}
#endif

#if 0
static void measure_power_usage(void)
{
	int i;

	// To investigate power usage of various parts of the system

	led_set_pattern(LED_PATTERN_OFF);

	trace_SaveBuffer();
	trace_TimestampMsg("MCU");
	for (i=0; i < 5; i++) ltc2943_trace_measurements("");
	trace_TimestampMsg("");

	trace_SaveBuffer();
	trace_TimestampMsg("MCU+LED1");
	STATUS_LED_ON;
	for (i=0; i < 5; i++) ltc2943_trace_measurements("");
	STATUS_LED_OFF;
	trace_TimestampMsg("");

	trace_SaveBuffer();
	trace_TimestampMsg("MCU+LED2");
	STATUS2_LED_ON;
	for (i=0; i < 5; i++) ltc2943_trace_measurements("");
	STATUS2_LED_OFF;
	trace_TimestampMsg("");

	trace_SaveBuffer();
	trace_TimestampMsg("MCU+LED1+LED2");
	STATUS_LED_ON;
	STATUS2_LED_ON;
	for (i=0; i < 5; i++) ltc2943_trace_measurements("");
	STATUS_LED_OFF;
	STATUS2_LED_OFF;
	trace_TimestampMsg("");

	trace_SaveBuffer();
	trace_TimestampMsg("MCU+BLE");
	wifi_wakeup();   // This can take many seconds
	for (i=0; i < 5; i++) ltc2943_trace_measurements("");
	trace_TimestampMsg("");
	for (i=0; i < 30; i++) { HAL_led_delay(1000); ltc2943_trace_measurements("");}
	trace_TimestampMsg("");

	trace_SaveBuffer();
	trace_TimestampMsg("MCU");
	wifi_sleep();   // If left powered on, esp32 draws about 20 mA during sleep
	for (i=0; i < 5; i++) ltc2943_trace_measurements("");
	trace_TimestampMsg("");

	trace_SaveBuffer();
	trace_TimestampMsg("MCU+CELL");
	S2C_POWERUP();
	for (i=0; i < 5; i++) ltc2943_trace_measurements("");
	trace_TimestampMsg("");
	for (i=0; i < 30; i++) { HAL_led_delay(1000); ltc2943_trace_measurements("");}   // Need to give Cell time to fully power on
	trace_TimestampMsg("");

	trace_SaveBuffer();
	trace_TimestampMsg("MCU");
	S2C_POWERDOWN();
	for (i=0; i < 5; i++) ltc2943_trace_measurements("");
	trace_TimestampMsg("");


	trace_SaveBuffer();

	if (BoardId == BOARD_ID_SEWERWATCH_1)
	{
		extern void pga460_turn_on(void);
		extern void pga460_turn_off(void);

		trace_TimestampMsg("MCU+460");
		pga460_turn_on();

		for (i=0; i < 5; i++) ltc2943_trace_measurements("");  // this can take 64 ms to complete
		trace_TimestampMsg("");
		for (i=0; i < 30; i++) { HAL_led_delay(1000); ltc2943_trace_measurements("");}
		trace_TimestampMsg("");

		trace_SaveBuffer();
		trace_TimestampMsg("MCU");
		pga460_turn_off();
		for (i=0; i < 5; i++) ltc2943_trace_measurements("");  // this can take 64 ms to complete
		trace_TimestampMsg("");
	}
	else
	{
		trace_TimestampMsg("MCU+Sen");
		sensor_power_en(ON);  // Note: this calls fully_charge_capacitor()  which takes 1 s

		for (i=0; i < 5; i++) ltc2943_trace_measurements("");  // this can take 64 ms to complete
		trace_TimestampMsg("");
		for (i=0; i < 30; i++) { HAL_led_delay(1000); ltc2943_trace_measurements("");}
		trace_TimestampMsg("");

		trace_SaveBuffer();
		trace_TimestampMsg("MCU");
		sensor_power_en(OFF);
		for (i=0; i < 5; i++) ltc2943_trace_measurements("");  // this can take 64 ms to complete
		trace_TimestampMsg("");
	}

	trace_SaveBuffer();

	led_enable_pattern = 1;
	led_set_pattern(LED_PATTERN_WIFI_ON);
	//s2c_ConfirmCellModem = 1;
	g_Enable_RTC_Charging = 1;
}
#endif




int main(void)
{
#if 0
	uint8_t buffer[]="Hello World";
	"Bank"
#endif

	// Check if booting from Bank 2
	if  (GetBFB2() == OB_BFB2_ENABLE)
	{
		// Relocate the Interrupt Vector Table
		SCB->VTOR = BANK2_SWAPPED_ADDRESS;
		sd_RunningFromBank2 = 1;
	}

	// Perform initialization.
    init_hw();   // After this function, isGen4() and isGen5() are functional.  This has a 2 second "debounce"

    // kludge to test date reset from app, make sure wake/sleep values update; simulates bad RTC backup battery
    //rtc_set_factory_date(); // Default RTC date is:  1/1/2018  00:00:00

	//extern void test(void);
	//test();  // Used for specific testing during development

    EnableTraceFunctions();   // This accesses the SD card and takes just a bit of time

    // ============= Kludge for Developers: bypass SD card ======================

    if (isGen5())
    {
	  //sd_UART_2SPY4 = 1;  // WIFI spies on VELOCITY 1
	  //sd_UART_1SPY5 = 1;  // CELL spies on VELOCITY 2
#ifdef ENABLE_SPY_UARTS
	  //sd_UART_4SPY2 = 1;  // For Max Turner:  BT development VEL1 spies on WIFI
	  //sd_UART_5SPY1 = 1;  // For future CELL development VEL2 spies on CELL
#endif

#ifdef ESP32_RECORD_FILE
      sd_UART_5SPY2 = 1;
#endif

    }

    sd_WiFiWakeLogEnabled = 1;
	//sd_UartBufferSizeLogEnabled = 1;
	//sd_WiFiCmdLogEnabled = 1;
	//sd_WiFiDataLogEnabled = 1;
	//sd_CellCmdLogEnabled = 1;
	//sd_CellDataLogEnabled = 1;


	// ============= End Kludge for Developers: bypass SD card ==================

	initialize_spy_uarts();

//#ifdef ENABLE_USB
#if 0
    while (1)
    {
      CDC_Transmit_FS(buffer, sizeof(buffer));
      CDC_Transmit_FS(uart1_inbuf, sizeof(uart1_inbuf));
      HAL_led_delay(2000);
    }
#endif

#if 0
    // Pass-thru VEL1 (UART4) to ESP32 (UART2) for bootloading and direct access to ESP32
	esp32_enter_bootload_mode(0); // 0 to put in bootload mode; 1 for simple uart pass-thru for debug messages from ESP32
#endif


#if 0  // SEE BELOW FOR PGA460 PASS THRU TESTING WITH SEWER WATCH BOARDS FULLY INITIALIZED
    // For Mark Watson cable testing to Velocity boards
	// Pass-Thru modes for testing UARTS, modems, and velocity boards using pc console
    // Use this to program ESP32 at 115200 baud via VEL1 (uart4) port
	PassThruMainLoop();
#endif

#if 0   // Operating modes for testing PGA460

    extern void main3(void);
	main3();  // PGA460

#endif

	// Kludge, initialize sensor here for testing.  Normally done inside init_iTracker.
	// Note, during development some Gen4 boards were hacked to explore Gen5 options, hence the ability to hack it here...
	if (isGen4())
	{
	  sensor_init(4);  // 4 = Gen4, 5 = Gen5
	}
	if (isGen5())
	{
	  sensor_init(5);  // 4 = Gen4, 5 = Gen5
	}

#ifdef TEST_SENSOR_MAIN_LOOP   //TestSensorMainLoop   EDIT COMPILE FLAG IN debug_helper.h

	TestSensorMainLoop();  // This is for testing the iTracker analog performance and gain both manually and via automatic search
    //TestLaird();
#endif


    init_iTracker();

#if 0 // Kludge for performing fast log test with cell modem
	//wifi_report_seconds = 1;
    MainConfig.log_interval = 60;  // Once per minute
	  strcpy(MainConfig.serial_number,"1700228");
	  strcpy(MainConfig.site_name, "Unit 7");
    saveMainConfig = 1;
	log_start(0);
#endif

#if 0  // Use this to set the clock to a known date/time
	{
	struct tm tm_st;
	extern void parse_datetime(char * strdate, struct tm * tm_st);  // Parses a string of the form "8/27/2014 13:15"  or "8/27/2014 13:15:32"
	parse_datetime("9/16/2022 14:45:30", &tm_st);
	rtc_sync_time(&tm_st);   // Cellular also syncs to the network UTC at power on.
	}
#endif



#if 0 // test vbat code

	HAL_Delay(1);
	Gen4_MX_ADC1_Init();
	HAL_led_delay(100);
    for (int i=0; i < 3; i++)
    {
    	float vbat = 1.0;

    	if (read_rtc_batt(&vbat) == 0)
    	{
    		vbat = 0.0;
    	}
    	HAL_led_delay(1000);
    }
	HAL_ADC_DeInit(&hadc1);
	// Mark - does the current go down again?
	vbat = 0.0;
#endif

    //kluge_set_sn_and_site();
	
	//test();  // Used for specific testing during development



	if (sd_FactoryTest)
	{
		// Enter the factory test main loop
		FactoryTestMainLoop();
	}

    trace_SaveBuffer(); // save all startup messages to SD card

	// Queue the first schedule.
	IsFirstWakeCycle = 1;
	calc_wakeup_times(1);

	TimeAtWakeUp = rtc_get_time();

// zack
	//if (sd_TestCellAlerts)
	//{
	//Wifi_Sleep_Time = WifiWakeTime + 45;  // Kludge. Bug Hunt. Speed up initial sleep time.
	//}


#ifdef STRESS_TEST_ESP32_WAKEUP



	// Establish a known configuration
	MainConfig.wifi_wakeup_interval = 1;
	MainConfig.log_interval = 60;

	ClearCellEnabled();

	//SetCellModule(CELL_MOD_NONE);
	//s2c_CellModemFound = S2C_NO_MODEM;
	//saveMainConfig = 1;

	//sd_DataLoggingEnabled = 0;  // Bug Hunt
	WifiWakeTime = TimeAtWakeUp;
	Wifi_Sleep_Time = WifiWakeTime + 20;  // go to sleep in 20 seconds after power on to allow time to connect via app

	s2c_ConfirmCellModem = 0;
	g_Enable_RTC_Charging = 0;

#if 0
	  // Enable the WiFi uart for ESP32
	  MX_USART2_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_NONE);

	  HAL_led_delay(100);  // give the UART a little time to stabilize; this appears to be CRITICAL to avoiding power on issues with the ESP32
#endif
	while (1)
	{
		wifi_wakeup();   // This can take many seconds

		STATUS2_LED_ON;

		process_wifi();  // This loop will log data and make cell calls as needed after esp32 power is stable

		STATUS2_LED_OFF;

		wifi_sleep();   // If left powered on, esp32 draws about 20 mA during sleep

		doSleep(); // BLOCKING

		//HAL_led_delay(5000);
		// Force WiFi on each cycle
		WifiWakeTime  = rtc_get_time();
		Wifi_Sleep_Time = WifiWakeTime + 10;  // only stay awake for 10 seconds
		TimeToTakeReading = WifiWakeTime;  // Force data point collision with wifi wakeup

#if 0
        // This works
		CurrSeconds = rtc_get_time();   // CRITICAL: MUST UPDATE CurrSeconds GLOBAL HERE !! SEE __sleep() below
		WifiWakeTime = CurrSeconds + 5;  // prevent wake up times in the past and enforce a minimum of 3 seconds sleep time to avoid knife edge situations
		__sleep(WifiWakeTime);  // Uses global variable CurrSeconds! Poor design...
		WifiWakeTime  = rtc_get_time();
		Wifi_Sleep_Time = WifiWakeTime + 20;
#endif

#if 0
		// this works
		HAL_led_delay(5000);
		WifiWakeTime  = rtc_get_time();
		Wifi_Sleep_Time = WifiWakeTime + 20;
#endif


	}
#endif

#ifdef MEASURE_SLEEP_CURRENT

	ClearCellEnabled();  // prevent cell phone calls
	s2c_ConfirmCellModem = 0;  // bypass cell modem hw confirmation
	g_Enable_RTC_Charging = 0; // bypass RTC charging

	// Use App to Clear the log file and go to sleep

#endif

#ifdef TEST_LIGHT_SLEEP
	//ClearCellEnabled();         // prevent cell phone calls
	//s2c_ConfirmCellModem = 0;   // bypass cell modem hw confirmation
	//g_Enable_RTC_Charging = 0;  // bypass RTC charging
	//sd_DataLoggingEnabled = 0;  // disable data logging but log wake/sleep on the data log schedule

	//WifiWakeTime = 0;    // Use App to clear logs
	//Wifi_Sleep_Time = 0;

	//test_undervoltage_detect();

	while (1)
	{
		log_accumulator_data_point(0);  // 0 = going to sleep
	    TimeScheduledToWakeUp = rtc_get_time() + (5 * 60 * 60); // light sleep for 5 hrs
	    LightSleepCount = light_sleep(TimeScheduledToWakeUp);

	    log_accumulator_data_point(1);  // 1 = awake
	    TimeScheduledToWakeUp = rtc_get_time() + (5 * 60 * 60); // light sleep for 5 hrs
	    LightSleepCount = light_sleep(TimeScheduledToWakeUp);
	}
#endif

#if 0  // USE THIS TO CAPTURE TRACE OF RAW BYTES
    // For Mark Watson cable testing to Velocity boards
	// Pass-Thru modes for testing UARTS, modems, and velocity boards using pc console
    // Use this to program ESP32 at 115200 baud via VEL1 (uart4) port
	PassThruMainLoop();
#endif

#if 0
	// Kludge for pga460 - trace one live reading cycle
	sd_EVM_460_Trace = 1;
	if (sd_EVM_460_Trace)
	{
		char msg[80];
		int dec9, dec10 = 0;

		get_sensor_measurement_0();

		// Distance
		convfloatstr_2dec(LastGoodMeasure, &dec9, &dec10);  // Note use of global variable
		sprintf(msg,"\nDistance = %d.%02d in", dec9, dec10);

		trace_AppendMsg(msg);
		trace_SaveBuffer();
		sd_EVM_460_Trace = 0;
	}
	while (1){
      led_heartbeat();
	}
#endif

#if 0
	if (sd_TraceVoltageEnabled)
	{
		measure_power_usage();
	}
#endif

#if 0  // produced 0.741 on my voltmeter at room temp
	if (BoardId == BOARD_ID_SEWERWATCH_1)
	{
		HAL_GPIO_WritePin(GEN5_SEWERWATCH_TEMP_DRV_Port, GEN5_SEWERWATCH_TEMP_DRV_Pin, GPIO_PIN_SET);
		while(1);
	}
#endif

#if 0
	while (1)
	{
	  float degF;
      Gen45_get_tempF(&degF);
  	  HAL_led_delay(500);
	}
#endif


	//s2c_trigger_confirm_cell_hw();  // must be able to report s2c_CellModemFound to the App in the settings packet
	s2c_CellModemFound =  GetCellModule(); // kludge for App

	// Perform remaining "debounce" time here and fully charge the main capacitor.  Continue to flash led.
	HAL_led_delay(1000);

	//loadSampleData(SAMPLE_DATA_ONE_MONTH);  // Kludge for Hail Mary testing




#if 0 // for testing led patterns

    //led_set_pattern(LED_PATTERN_SINGLE_RED);
    led_set_pattern(LED_PATTERN_DOUBLE_RED);
    //led_set_pattern(LED_PATTERN_TRIPLE_RED);
	while (1)
	{
		led_heartbeat();
	}

#endif

    //ClearBLENameOverride();

	while (1)
	{
		int ok_to_sleep;

		ok_to_sleep = 1;

		manage_leds();

		led_heartbeat();

		ltc2943_timeslice();

		perform_delayed_power_on_activities();

		doTakeReading(); // This will log data and make automatic cell calls as needed

		if (wifi_timeslice()) ok_to_sleep=0;

		if (s2c_timeslice()) ok_to_sleep=0;

		if (ok_to_sleep)
		{
			if (!s2c_trigger_initial_power_on_ping())  // causes a power on ping to occur at the first sleep cycle (if cell is enabled).
			{
			  doSleep(); // BLOCKING
			}
		}

	}

} // MAIN()

