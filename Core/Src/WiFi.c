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

#include "stm32l4xx_hal.h"
#include "debug_helper.h"
#include "main.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include <ctype.h>
#include "string.h"
#include "rtc.h"
#include "Common.h"
#include "WiFi.h"
#include "esp32.h"
#include "zentri.h"
#include "usart.h"
#include "uart2.h"
#include "WiFi_Socket.h"
#include "s2c.h"

#include "trace.h"
#include "factory.h"

extern uint32_t WifiIsAwake;





void wifi_config(void)
{
	if (isGen4()) zentri_config(DO_NOT_LOAD_ZENTRI_FILES);   // Extracts MAC address and assigns SSID from config.  Leaves in STREAM mode (socket connected)

	if (sd_WiFiCmdLogEnabled) trace_SaveBuffer();  // flush any logged info to disk

	WifiIsAwake = 1;

}

int wifi_is_active(void)
{
	return WifiIsAwake;
}

// Wifi Wakeup
int wifi_wakeup(void)
{
	if (WifiIsAwake) return 1;  // already awake, nothing to do, no time passed

	WifiIsAwake = 1;

    //TODO maybe someday use a configuration variable for the WiFi module.  For now, use the Gen5 vs Gen4 test.
	if (isGen5())
	{

	  // Enable the WiFi uart for ESP32
	  MX_USART2_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_NONE);

	  HAL_led_delay(1);  // give the UART a little time to stabilize

	  esp32_wakeup();

	}
	else
	{

	  // Enable the WiFi uart for Zentri
	  MX_USART2_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_RTS_CTS);
	  zentri_wakeup();
	}

	TimeWiFiPoweredOn = rtc_get_time();
	wifi_delay_timer_ms = 5000;   //  start wifi power on countdown delay.  Original was 5000.  2000 was tried but caused problems in field, probably due to insufficient cap charge up time.

#ifdef LOG_SLEEP_WAKE
// This is just a placeholder to indicate the other point in the code that is involved in the sleep/wake cycle.  Minimum 4 seconds ON time.
#endif
	// Enforce a minimum WiFi on time of 4 seconds from this point in time.  The ESP32 can have a variable amount of power-on time.
	if (Wifi_Sleep_Time < (TimeWiFiPoweredOn + 4))  Wifi_Sleep_Time = TimeWiFiPoweredOn + 4;

  return 1;
}



void wifi_sleep(void)
{
	if (WifiIsAwake == 0) return;  // already powered off, nothing to do

	if (isGen5())
	{
		esp32_sleep();
	}
	else
	{
		zentri_sleep();
	}

	// Deinit UART.  Must be careful to ensure no further characters are incoming on the interrupt-driven RX channel...
	HAL_UART_MspDeInit(&huart2);
	//HAL_UART_DeInit(&huart2);
	HAL_led_delay(100);  // give the UART a little time to stabilize


	//led_enable_pattern = 0;   // LED is enabled only during WiFi (or at power on)
	led_set_pattern(LED_PATTERN_OFF);

	WifiIsAwake = 0;
}

int8_t wifi_chkkey(char * key)
{
	int i = 0;
	uint8_t tmp = 0;

	for (i = 0; i < 8; i++) {
		if ((key[i] < '0') || (key[i] > '9'))
			tmp = 1;
	}

	if (key[8] != 0)
		tmp = 1;

	if (tmp) {
		strcpy(key, "00000000\0");
		return 1;
	}
	return 0;
}




