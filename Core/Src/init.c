/*
 * init.c
 *
 *  Created on: Apr 10, 2020
 *      Author: Zachary
 */


#include "main.h"
#include "debug_helper.h" // For debugging and hacking around when testing, etc.
//#include "boot_helper.h"  // For field updates to the firmware.
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
#include "version.h"
#include "zentri.h"
#include "tim.h"

#include "spi.h"
#include "Gen5_AD5270.h"


void SystemClock_Config(void);


uint32_t stm32_DEVID;

static void power_on_msg(void)
{
	char msg[256];

	led_power_on_banner();

	// Note when power on occurs
	get_power_on_msg(msg);


	trace_AppendMsg("\n");
	trace_AppendMsg(msg);

	//AppendStatusMessage("\n");
	AppendStatusMessage(msg);
}



// MAIN INTIALIZATION
void init_hw(void)
{
	// Initialize RAM based tracing functions (no hardware)
	trace_Init();
	trace_AppendMsg("\ninit_hw\n");

	// Reset of all peripherals, Initializes the Flash interface and the Systick.  This calls HAL_MspInit()
	HAL_Init();

	// Wait here for main capacitor to charge to 95% (3 Tau = 660 ms), using systick based on MSI clock  (1 Tau = 220 ms)
	// Ok to flash led here to provide visual indication power is on.
	HAL_led_delay(FOUR_TAU);

	stm32_DEVID = HAL_GetDEVID();

	// Configure the system clock and also the Systick
	SystemClock_Config();

	// Initialize all configured peripherals
	MX_GPIO_Init();

	// Turn on LED until done initializing.
	STATUS_LED_ON;

	get_board_id();   // also initializes some board-specific gpio pins

    // SPI for FLASH and LOGS
	if (BSP_QSPI_Init() != QSPI_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	// Initialize standard peripherals.


    // zg ===================================

	if (MX_I2C2_Init() != 0)  // zg - for some reason, this function sets a HAL error flag that screws up bootloading.  Maybe fixed on 1/30/2020 by removing de-init function call?
	{
		_Error_Handler(__FILE__, __LINE__);
	}
	__HAL_FLASH_CLEAR_FLAG(FLASH_SR_PGSERR); // zg - clear the HAL error flag here to allow for bootloading

	// zg ===================================


	if (MX_SPI2_Init() != 0)  //maw
	{
		_Error_Handler(__FILE__, __LINE__);
	}
	__HAL_FLASH_CLEAR_FLAG(FLASH_SR_PGSERR); // zg - clear the HAL error flag here to allow for bootloading

	// Set the real-time clock and epoch.
	// https://www.epochconverter.com/
	if (MX_RTC_Init() != 0)
	{
		// Kludge - remove error check for bad-board repair, note that no LED pattern will occur as the tick counter is not running.
		_Error_Handler(__FILE__, __LINE__);
	}

	// Save the time at which power-on occurred
	init_power_on_time();

	power_on_msg();

	// Init LTC2943 as close to power on as possible to set pre-scaler (M) correctly for sense resistor on the board.  Uses i2c.
	// Note: this will set the accumulator value to mid-scale, provided the LTC2943 is not already running and initialized from a previous power-on.
	// The magnet wake up does not disable the LTC2943 (which is held up by the main capacitor), just disrupts the main 3V3 supply (not a good design).
	ltc2943_init_hw();

	// Debounce power-on wakeup --- wait for person to remove magnet, wait for power supplies to stabilize, batteries to settle, etc
	// Original code was 3000, but this has been distributed above (880 ms) and below (1000 ms just before main loop)
	// Definitely want to flash the led here to provide visual feedback the system has power
	HAL_led_delay(1120);

	ltc2943_Enforce_HARDSTOP_Voltage_Limit(1);  // Read voltage

    if (isGen5())
    {

	  /* Initialize microsecond delay subsystem used by both iTracker5 and SewerWatch boards */
	  us_Timer_Init();

	  if ((BoardId == BOARD_ID_510) || (BoardId == BOARD_ID_511))
	  {
	    // Initialize OneWire Temperature Sensor (uses microsecond timer)
	    Gen5_MAX31820_init();

	    // Initialize the digital gain pot
	    Gen5_AD5270_Init();
	  }

    }

#ifdef ENABLE_USB
	MX_USB_DEVICE_Init();
#endif
}


void init_power_on_time(void)
{
	// Save the time at which power-on occurred
	TimeAtPowerOn = rtc_get_time();
	TimeAtWakeUp = TimeAtPowerOn;
	PrevTimeAtWakeUp = TimeAtPowerOn;
}




void init_iTracker(void)
{
	int empty_or_corrupt_flash_detected;

	// Save the time at which power-on occurred
	//init_power_on_time(); // moved to init_hw


	// zg FOR TESTING =============
	//BSP_QSPI_Erase_Chip();  // takes about 48 seconds, or is that just the timeout value ??
	//while(1);
	// zg FOR TESTING =============

	// Load the Main configuration from flash.
	empty_or_corrupt_flash_detected = Load_MainConfig();

	UNUSED(empty_or_corrupt_flash_detected);



#if 0  // For early Gen5 skywired prototypes
	if (isGen5())
	{
	  // Turn on WiFi last so that it is not broadcasting and allowing connections until the firmware is ready.
	  // This will get the MAC address from the ESP32 and compare to the previous stored one (if any).
	  // Note: early Gen5 protoytpe boards did not actually have control over power, so (provided no SD card is present)
      // This was hoisted up temporarily to allow the STM32 to capture the output from the ESP32 as they both power on at the same time,
	  // provided there is no SD card present.
	  // Note that having access to the serial number and site name is used to set the BT name, so this should
	  // occur after the Load_MainConfig();
	  wifi_wakeup();   // Just leave it on...
	}
#endif

	// Load the Sensor configuration from flash - this did not exist on legacy firmware
	Load_SensorConfig();

	// Load the Cellular configuration from flash - this did not exist on legacy firmware
	Load_CellConfig();

	if (rtc_BackupBatteryDepleted)
	{
	  trace_AppendMsg("RTC backup battery depleted.\n");
	}



#if 0 // Kludge --- set and initialize the serial number and cell module
	// SN        ESP32 MAC       Owner       Description
	// 1710000   34865d1e50a6    Max         SN? Gen5 Prototype; does not control power to ESP32; kludged SD reader; otherwise functional
	// 1710003                   Zachary     SN3 Gen5 Prototype; does not control power to ESP32; kludged SD reader; otherwise functional
	// 1710004                   Zachary     SN4 Gen5 Prototype; MISSING LED; does not control power to ESP32; kludged SD reader; otherwise functional
	// 1710005                   Mark W.
	// 1710006                   Mark W.
	if (isGen4())
	{
	  //strcpy(MainConfig.serial_number,"1700217"); // for unit in Eastech Flow Lab with bad Telit Cell Modem; won't complete the AT#SD command...  1,0,2222,"54.24.219.107"
	  strcpy(MainConfig.serial_number,"1710003");
	  strcpy(MainConfig.site_name, "Sheridan");

	  //telit_reset();
	  xbee3_reset();
	}
	if (isGen5())
	{

	  strcpy(MainConfig.serial_number,"1710003");
	  strcpy(MainConfig.site_name, "Sheridan");
	  //strcpy(MainConfig.site_name, "Unit7");


	  // ===== induce ESP32 stack smashing error From Matt no-epoxy unit 11-04-2022
	  //strcpy(MainConfig.serial_number,"1700378 ");    // Note: SN has a space at the end
	  //strcpy(MainConfig.site_name, "TestingTesting");
	  // ===== end stack smashing error

	  SetCellModule(CELL_MOD_LAIRD);
	  SetCellEnabled();
	  s2c_CellModemFound = S2C_LAIRD_MODEM;

	}
	ClearFirstResponse();
	set_default_cell_ip_and_port();
	saveMainConfig = 1;
	saveCellConfig = 1;
#endif


#if 0

	// Due to problems with the batteries and the ESP32 power-on "brownouts", do NOT power on the cell phone here,
	// do it later after the ESP32 is up and running

	// If configured, power on the Cell modem and start the confirm hardware process.
	// Powerup for Laird and Telit is 10 seconds, XBee3 is 3 seconds.
	// However, don't wait here, just continue.  The cell confirmation process will
	// conclude in the WiFi polling loop.
    s2c_ConfirmCellModemHardware();  // This powers on the cell modem, if configured.
#endif


	// Initialize the data logging mechanism in flash.
	log_init();  // uses rtc, also initializes CurrentLog

	//TODO Initialize sensor Gen4 or Gen5 here?  hoisted to main...


	// Initialize the battery meter
	// LogTrack.accumulator  - updated per data point, and other times, such as replacing a battery
	// MainConfig.val        - updated once per day (at first data point past midnight)
	// MainConfig.we_current - updated on Sun and Mon (at first data point past midnight)
	// MainConfig.wd_current - updated on Tues - Sat (at first data point past midnight)

#if 0  // Begin: inject accumulator values here for battery capacity testing

	//LogTrack.accumulator = MAX_BATTERY * 0.051;  // 5.1%
	//LogTrack.accumulator = MAX_BATTERY * 0.05;   // 5.0%   This actually results in 4.998 when the division is performed later so it acts like less than 5
	//LogTrack.accumulator = MAX_BATTERY * 0.049;  // 4.9%
	//LogTrack.accumulator = MAX_BATTERY * 0.04;   // 4.0%
	//LogTrack.accumulator = MAX_BATTERY * 0.021;  // 2.1%
	LogTrack.accumulator = MAX_BATTERY * 0.02;   // 2.0%
	//LogTrack.accumulator = MAX_BATTERY * 0.019;  // 1.9%
	//LogTrack.accumulator = MAX_BATTERY * 0.01;  // 1.0%   tested, went into HardStop 3 ok

	BSP_Log_Track_Write(0);  // Do not update battery accumulator, leave it alone.

	MainConfig.val = LogTrack.accumulator;

#endif  // End: inject accumulator values for battery capacity testing

	ltc2943_init_battery_accumulator(1); // uses global LogTrack.accumulator

	// To force a known config for fixing boards
	//SetWiFiInternalAnt();
	//ClearWiFiConfigured();
	//ClearCellEnabled();
	//SetCellEnabled();
#ifdef FORCE_1_MINUTE_DATA_POINTS
	MainConfig.log_interval = 60;  // 60 = every minute; 300 = every 5 min
	saveMainConfig = 1;
#endif
	// End force known config

	// Perform bootload from SD card, if needed.   This will reboot the hardware if needed.
	Bootload();

	// Firmware is OK at this point, perform other factory actions indicated on SD card
	factory_perform_sd_actions();

	if (isGen4())
	{
		// 5-13-2022 zg
		// The WebApp was discontinued when the App was able to connect directly to the socket.
		// Therefore, the WebApp files are no longer updated or maintained or even shipped on the SD card.
		// The ability to load zentri web server files has therefore been removed from this codebase.
		// The files removed were webapp.h and webapp.c and an if statement in the zentri_config() function.
		// If needed in the future they can be retrieved and the LOAD_ZENTRI_FILES used below.
		//zentri_config(LOAD_ZENTRI_FILES); // zg - this now takes 3.824 seconds (with no Zentri file loads) as opposed to 84 seconds or more.  Hence, always do it at power on.
		zentri_config(DO_NOT_LOAD_ZENTRI_FILES); // zg - this now takes 3.824 seconds (with no Zentri file loads) as opposed to 84 seconds or more.  Hence, always do it at power on.
	}




#ifdef TEST_CURRENT_DRAW   // for measuring current draw
	sd_WiFiStayAwakeAtPowerOn = 0;  // go to sleep immediately, do not stay awake at power on
	ClearCellEnabled();   // disable power-on-ping
    //SetCellEnabled();
	MainConfig.log_interval = 120;
	saveMainConfig = 1;
#endif


	// NO LONGER TRUE At this point CELL power is on (if configured) and WiFi/BT is off
	// At this point both CELL and WiFi/BT are OFF

	if (isGen4())
	{
		if (IsWiFiInternalAnt())
		{
			AppendStatusMessage("WiFi set to internal antenna.");
		}
		else
		{
			AppendStatusMessage("WiFi set to external antenna.");
		}
	}

#if 0
	// Report some info to the SD card

	if (IsFirstResponse())
	{
		AppendStatusMessage("Configured as FirstResponseIQ.");
	}
	else
	{
		AppendStatusMessage("Configured as iTracker.");
	}


	if (IsBLENameOverride())
	{
		AppendStatusMessage("BLE name override: iTracker.");
	}
	else
	{
		if (BoardId == BOARD_ID_SEWERWATCH_1)
		{
			AppendStatusMessage("BLE name: SewerWatch.");
		}
		else
		{
			AppendStatusMessage("BLE name: iTracker.");
		}
	}
#endif

}


void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInit;

	/** Configure LSE Drive Capability */
	HAL_PWR_EnableBkUpAccess();

	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

	/** Initializes the CPU, AHB and APB busses clocks */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI | RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
	RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_7;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 20; // was 12
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		// Kludge - remove error check for bad-board repair, note that no LED pattern will occur as the tick counter is not running.
		_Error_Handler(__FILE__, __LINE__);
	}

	/** Initializes the CPU, AHB and APB busses clocks */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)	{
		// Kludge - remove error check for bad-board repair, note that no LED pattern will occur as the tick counter is not running.
		_Error_Handler(__FILE__, __LINE__);
	}

	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_USART3
		| RCC_PERIPHCLK_I2C2 | RCC_PERIPHCLK_USB | RCC_PERIPHCLK_SDMMC1 | RCC_PERIPHCLK_ADC;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
	PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
	PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
	PeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
	PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
	PeriphClkInit.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_PLL;
	PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
	PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
	PeriphClkInit.PLLSAI1.PLLSAI1N = 16;
	PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
	PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV4;
	PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV4;
	PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_ADC1CLK;

	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		// Kludge - remove error check for bad-board repair, note that no LED pattern will occur as the tick counter is not running.
		_Error_Handler(__FILE__, __LINE__);
	}

	/** Configure the main internal regulator output voltage */
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
	{
		// Kludge - remove error check for bad-board repair, note that no LED pattern will occur as the tick counter is not running.
		_Error_Handler(__FILE__, __LINE__);
	}

	/** Enable MSI Auto calibration */
	HAL_RCCEx_EnableMSIPLLMode();

	/* Enable Power Clock */
	__HAL_RCC_PWR_CLK_ENABLE();

	/* Ensure that MSI is wake-up system clock */
	__HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_MSI);

	/** Configure the Systick interrupt time	*/
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

	/** Configure the Systick */
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);


	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

void SYSCLKConfig_STOP(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit;
	uint32_t pFLatency = 0;

	/* Enable Power Control clock */
	__HAL_RCC_PWR_CLK_ENABLE();

	/* Get the Oscillators configuration according to the internal RCC registers */
	HAL_RCC_GetOscConfig(&RCC_OscInitStruct);

	/* Enable PLL */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI|RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;

	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	/* Get the Clocks configuration according to the internal RCC registers */
	HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &pFLatency);

	/* Select PLL as system clock source and keep HCLK, PCLK1 and PCLK2 clocks dividers as before */
	RCC_ClkInitStruct.SYSCLKSource  = RCC_SYSCLKSOURCE_PLLCLK;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USART1
		|RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_USART3
		|RCC_PERIPHCLK_I2C2|RCC_PERIPHCLK_USB
		|RCC_PERIPHCLK_SDMMC1|RCC_PERIPHCLK_ADC;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
	PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
	PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
	PeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
	PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
	PeriphClkInit.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_PLL;
	PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
	PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
	PeriphClkInit.PLLSAI1.PLLSAI1N = 16;
	PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
	PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV4;
	PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV4;
	PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_ADC1CLK;

	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}
}

void HAL_IncTick(void)
{
  extern __IO uint32_t uwTick;

  uwTick++;

  if (uart1_timeout) uart1_timeout--;
  if (uart2_timeout) uart2_timeout--;
  if (uart2_previous_tx_wait_time_ms) uart2_previous_tx_wait_time_ms--;
  if (uart3_timeout) uart3_timeout--;
  if (uart4_timeout) uart4_timeout--;
  if (lpuart1_timeout) lpuart1_timeout--;
  if (led_countdown_timer_ms) led_countdown_timer_ms--;
  if (hal_led_delay_timer_ms) hal_led_delay_timer_ms--;
  if (generic_delay_timer_ms) generic_delay_timer_ms--;
  if (cell_delay_timer_ms) cell_delay_timer_ms--;
  if (wifi_delay_timer_ms) wifi_delay_timer_ms--;

  // Maintain the estimated UTC time - this is not used yet.  Intended for when enabling the Cell from inside SewerWatch, but no such auto-sync occurs yet.
  if (UTC_countdown_timer_ms)
  {
	  UTC_countdown_timer_ms--;
	  if (UTC_countdown_timer_ms == 0)
	  {
		  UTC_time_s++;
		  UTC_countdown_timer_ms = 1000;  // keep going, this is turned off at first sleep transition (e.g. WiFi wakeup time is over)
	  }
  }

}

void _Error_Handler(char *file, int line)
{
	// Kludge - remove error check for bad-board repair, note that no LED pattern will occur as the tick counter is not running.
	//uint32_t tick_at_error = HAL_GetTick();
	while (1) {
		led_error_report();
		HAL_Delay(500);
	}
}



