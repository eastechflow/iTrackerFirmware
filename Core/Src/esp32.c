/*
 * esp32.c
 *
 *  Created on: Mar 23, 2022
 *      Author: Zachary
 */

#include "common.h"
#include "gpio.h"
#include "uart2.h"
#include "esp32.h"
#include "tim.h"
#include "s2c.h"

#include "usart.h"
#include "uart4.h"
#include "factory.h"
#include "debug_helper.h"
#include "trace.h"
#include "ltc2943.h"

#if 0
static void consume_power_down_chars(int delay_ms)
{
	uint8_t ch;
	hal_led_delay_timer_ms = delay_ms + 1;  // to ensure at least the number of requested ms...
	while (hal_led_delay_timer_ms)
	{
		led_heartbeat();
		uart2_getchar(&ch));
	}
}
#endif


static int wait_for_BT_Name(int delay_ms)
{
	uint8_t ch;
	char msg[80];
	int count = 0;

	// The ESP32 will echo the BT Name followed by \r\n so wait for it
	// Apparently on some units, this can take more than 4 seconds ???
	hal_led_delay_timer_ms = delay_ms + 1;  // to ensure at least the number of requested ms...
	while (hal_led_delay_timer_ms)
	{
		if (uart2_getchar(&ch))
		{
			if (count < (sizeof(msg)-1))
			{
				msg[count++] = (char) ch;
				msg[count] = 0;  // null-terminate as we go
			}
			if (ch == '\n')
			{
				if (count > 1) // Strip the CR LF
			    {
					if (msg[count-2] == '\r') msg[count-2] = 0 ;
				}
				if (IsFirstWakeCycle) AppendStatusMessage(msg);   // For diagnostics, record what the ESP32 is sending
				return 1;   // a complete line was received
			}
		}
		led_heartbeat();
	}
	return 0;
}

//                                          01234567890123456
//	The ESP32 MAC message looks like this:  34:86:5d:1d:64:f6\r\n"
static int has_MAC_colons(const char *msg)
{
  if (msg[ 2] != ':') return 0;
  if (msg[ 5] != ':') return 0;
  if (msg[ 8] != ':') return 0;
  if (msg[11] != ':') return 0;
  if (msg[14] != ':') return 0;
  return 1;
}

static void log_msg(const char *logmsg)
{
	AppendStatusMessage(logmsg);

	if (sd_UART_4SPY2)
	{
		uart4_tx_string("\n");
		uart4_tx_string(logmsg);
		uart4_tx_string("\n");
	}
	if (sd_WiFiCmdLogEnabled)
	{
		trace_AppendMsg("\n");
		trace_AppendMsg(logmsg);
		trace_AppendMsg("\n");
	}
}


static int parse_MAC_address(int delay_ms)
{
	uint8_t ch;
	char msg[80];
	char logmsg[80];
	int count = 0;

	hal_led_delay_timer_ms = delay_ms + 1;  // to ensure at least the number of requested ms...
	while (hal_led_delay_timer_ms)
	{
		if (uart2_getchar(&ch))
		{
			if (count < (sizeof(msg)-1))
			{
				msg[count++] = (char) ch;
				msg[count] = 0;  // null-terminate as we go
			}

			if (ch == '\n') // a complete line was received
			{

				if (count > 1) // Strip the CR LF
			    {
					if (msg[count-2] == '\r') msg[count-2] = 0 ;
				}

				if (IsFirstWakeCycle) log_msg(msg);   // For diagnostics, record what the ESP32 is sending


				// Parse MAC address from line
                if ((strlen(msg) == 17) && has_MAC_colons(msg))
                {
                	// Verify a MAC was indeed received?  or is len and colons enough?

    				sprintf(logmsg,"ESP32 MAC %s Saved MAC %s", msg, MainConfig.MAC);
    				if (IsFirstWakeCycle) log_msg(logmsg);

                	// Compare with previously saved value
                	if (strstr(MainConfig.MAC, msg))
                	{
                		// same MAC, nothing to do
                		sprintf(logmsg, "ESP32 skipped save to MainConfig");
                		if (IsFirstWakeCycle) log_msg(logmsg);
                	}
                	else
                	{
                		// Copy new MAC to config and save
                		int i;
                		int j = 0;
                		// MainConfig.MAC is mm:MM:mm:MM:mm:MM  (18 including null term)
                		for (i = 0; i < 17; i++)
                		{
                			MainConfig.MAC[j++] = msg[i];  // copy with colons
                		}
                		MainConfig.MAC[j] = 0;  // null-terminate
                		saveMainConfig = 1;
                		sprintf(logmsg, "ESP32 saved new MAC");
                		if (IsFirstWakeCycle) log_msg(logmsg);
                	}

                	return 1; // Found MAC, protocol OK so far
                }
  			    else
  			    {
  				    // Throw this line away and wait for another one
  				    count = 0;
  				    msg[count] = 0;  // null-terminate as we go
  			    }
			}
		}
		led_heartbeat();
	}

	if (IsFirstWakeCycle) log_msg("MAC not detected.");

	return 0;  // MAC not detected, protocol failure
}

#if 0
// TODO work in progress
int get_configuration(void)
{
	if (IsBLENameOverride())   // see JSON {"Command":"b1"}
	{
	    return CONFIGURED_AS_ITRACKER;
	}
	else
	{
	  if (BoardId == BOARD_ID_SEWERWATCH_1)
	  {
		  return CONFIGURED_AS_SEWERWATCH;
	  }
	  else
	  {
		  return CONFIGURED_AS_ITRACKER;
	  }
	}
	return CONFIGURED_AS_ITRACKER;
}
#endif

static void send_BT_Name(void)
{
	char prefix[20];
#if 0 // Not fully tested.  Most likely would not cause issues, but who knows?
	// The BLE Uart code on the ESP32 will wait up to 5 seconds to receive the BT Name.
	// This time can be used to allow the capacitor to fully recharge before starting the
	// actual BLE current-heavy start up.
	// The capacitor time-constant is 0.220 sec.  Five Tau is 1.1 seconds.  Use 2 to be safe.
	// Wait for capacitor to be fully charged
	HAL_led_delay(2000);
#endif

	// Send the name to use for the BTName:  "iTracker_EntireSN_SiteName\r\n".  Minimum is "iTracker_".  Default is "iTracker_"
	if (MainConfig.serial_number[0] == 0)  // check if a serial number string exists, if not, use 0
	{
		MainConfig.serial_number[0] = '0';  // assign the character zero to represent the serial number
		MainConfig.serial_number[1] = 0;    // null-terminate the string
	}

	//if (get_configuration() == CONFIGURED_AS_SEWERWATCH)
    if (ProductType == PRODUCT_TYPE_SEWERWATCH)
	{
		sprintf(prefix,"SewerWatch");
	}
	else
	{
		 sprintf(prefix,"iTracker");
	}


	sprintf((char *)uart2_outbuf,"%s_%s_%s\r\n", prefix, MainConfig.serial_number, MainConfig.site_name);  // sn max is 10, site name max is 20, so overall max is 40 + null

	// There is apparently a limit on the BLE name.
	// The name "iTracker_1700378__TestingTesting" triggers a "Stack smashing protect failure!" failure on the ESP32.  Only 29 chars plus null works.
	// Prevent long names here as a precaution against older BLE firmware.
	if (strlen((char *)uart2_outbuf) > 29) uart2_outbuf[29] = 0;

	uart2_tx_outbuf();
}

static void parse_esp32_version_string(const char *msg)
{
	if (strstr(msg,"Eastech BLE UART 1.6"))
	{
		uart2_esp32_version = ESP32_VERSION_1_6;
	}
	if (strstr(msg,"Eastech BLE UART 1.5"))
	{
		uart2_esp32_version = ESP32_VERSION_1_5;
	}
	else if (strstr(msg,"Eastech BLE UART 1.4"))
	{
		uart2_esp32_version = ESP32_VERSION_1_4;
	}
	else if (strstr(msg,"Eastech BLE UART 1.3"))
	{
		uart2_esp32_version = ESP32_VERSION_1_3;
	}

}

#if 0
static int unexpected_lines_detected(int delay_ms, int ExpectedLines)
{
	uint8_t ch;
	char msg[80];
	int count = 0;
	int result = 0;
	int lines_received = 0;

	hal_led_delay_timer_ms = delay_ms + 1;  // to ensure at least the number of requested ms...
	while (hal_led_delay_timer_ms)
	{
		if (uart2_getchar(&ch))   // grab one character and process it
		{
			if (count < (sizeof(msg)-1))
			{
				msg[count++] = (char) ch;
				msg[count]=0;   // null terminate as we go
			}

			if (ch == '\n') // a complete line was received
			{
			  lines_received++;   // keep count of lines received

			  if (count > 1) // Strip the CR LF
			  {
				if (msg[count-2] == '\r') msg[count-2] = 0 ;
			  }

			  log_msg(msg);   // For diagnostics, record what the ESP32 is sending

			  // Throw this line away and wait for another one
			  count = 0;

			  //result = 1;  // some unexpected characters were detected
			}
		}

		led_heartbeat();  // Due to power on issues, the led is normally NOT used when ESP32 is powering up
	}
	if (lines_received > ExpectedLines) result = 1;
	return result;  // 0 if no characters detected, 1 if extra characters detected
}
#endif

static int parse_banner_message(int delay_ms)
{

//	The ESP32 power-on banner looks like this:
#if 0
	ets Jul 29 2019 12:21:46

	rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
	configsip: 0, SPIWP:0xee
	clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
	mode:DIO, clock div:1
	load:0x3fff0030,len:1240
	load:0x40078000,len:13012
	load:0x40080400,len:3648
	entry 0x400805f8
	Eastech BLE UART 1.5a
	BTName?
	iTracker_1710003_Unit4
	34:86:5d:1d:e8:4e
#endif

	uint8_t ch;
	char msg[80];
	int count = 0;
	int result = 1;  // assume correct protocol

	//trace_AppendMsg("GotHere1");

	hal_led_delay_timer_ms = delay_ms + 1;  // to ensure at least the number of requested ms...
	while (hal_led_delay_timer_ms)
	{
		if (uart2_getchar(&ch))   // grab one character and process it
		{
			if (count < (sizeof(msg)-1))
			{
				msg[count++] = (char) ch;
				msg[count]=0;   // null terminate as we go
			}

			if (ch == '\n') // a complete line was received
			{

			  if (count > 1) // Strip the CR LF
			  {
				if (msg[count-2] == '\r') msg[count-2] = 0 ;
			  }

			  if (IsFirstWakeCycle)
			  {
			    AppendStatusMessage(msg);   // For diagnostics, record what the ESP32 is sending
			  }

			  //trace_AppendMsg("GotHere2");
			  parse_esp32_version_string(msg);
			  //trace_AppendMsg("GotHere3");
			  if (strstr(msg,"BTName?"))
			  {
				send_BT_Name();
				goto step2;
			  }
			  else
			  {
				  // Throw this line away and wait for another one
				  count = 0;
			  }
			}
		}

		led_heartbeat();  // Due to power on issues, the led is normally NOT used when ESP32 is powering up
	}

	log_msg("ESP32 banner message not detected.");

	return 0;  // protocol failure, ESP32 not functioning correctly

	step2:
	//trace_AppendMsg("GotHere4");
	if (wait_for_BT_Name(10000))   // Some units take longer than 4 seconds to reply, possibly due to BLE startup
	{
		//trace_AppendMsg("GotHere5");
		// The ESP32 turns on BLE to get the MAC.
		// The BLE init process can apparently take 2 or more seconds on some units.
		// Turning on BLE causes ANOTHER inrush of current and can draw down the iTracker power rail.
		// If the ESP32 power rail drops to 2.8 volts, the brownout detector is triggered and a message is sent via the UART.
		//
		if (!parse_MAC_address(10000))
		{
			result = 0;
		}
	}
	else
	{
	  log_msg("ESP32 echo of BTName message not detected.");
	  result = 0;
	}

	// In at least one captured trace, the ESP32 completed the entire power on and messaging exchange, and yet,
	// later entered a brownout state.  That may imply that the BLE turn on process can extend past the sending of
	// the MAC address...


#if 0
	// Check for unexpected lines.  For example, if the ESP32 experienced a brownout and reboots, there will be a bunch more lines here.
	// For diagnostics, echo a few of them here but don't wait forever
	// Note that two more lines are echoed from the ESP32 and are expected:  The BLE name and the MAC address.  Ignore those.
	if (unexpected_lines_detected(500, 2))
	{
		log_msg("ESP32 unexpected lines detected.");
		result = 0;  // protocol not correct, unexpected lines detected
	}
#endif

	return result;
}

#if 0
static void print_comm_4_as_hex(void)
{
	char cmd[50];
	char msg[50];
	sprintf(cmd,"{\"comm\":\"4\"}");
	for (int i = 0; i < 12; i++)
	{
	  sprintf(msg,"%.2X ", cmd[i]);
	  uart4_tx_string(msg);
	}

}
#endif

#if 0
static void original_loop(void)
{
	// 5-18-2022 Mark Watson discovered the ESP32 causes a power dip lasting about 10 us that can trigger the brownout detector.
	// This is a poor-man's PWM to try and avoid that.  See comments in code.

	// The io pin may be responding, but the FET switch has a 1 M ohm pull up (R87)
	// so it will be very slow due to the gate capacitance of Q15.
	// On future builds, a 100 K resistor from the drain of Q14 to the gate of Q15 would help slow the turn on.
#if 0
	1
	0
	00000000000000000000000000000000000000000000000000
	1
	0000000000000000000000000000000000000000
	1
	000000000000000000000000000000
	1
	00000000000000000000
	1
	0000000000
	1
#endif

	//NVIC_DisableIRQ(USART2_IRQn); // This is critical.  Must not interrupt the power-up sequence.
	//if (s2c_cell_power_is_on) NVIC_DisableIRQ(USART1_IRQn);
	//TODO be careful about the other interrupts as well (RTC, etc)
	uint32_t prim;
	prim = __get_PRIMASK();  // (0 if enabled; 1 if disabled) check if interrupts are already off so we can leave them off when exiting, not strictly necessary here, but good practice.
	__disable_irq();

	GEN5_WIFI_PWR_EN_Port->BSRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;  // sets the bit
	GEN5_WIFI_PWR_EN_Port->BRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // resets the bit
	for (int i = 0; i < 5; i++)
	{
		for (int j =  0; j < (50 - (10*i)); j++)
		{
			GEN5_WIFI_PWR_EN_Port->BRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // resets the bit
		}
		GEN5_WIFI_PWR_EN_Port->BSRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // sets the bit
	}

	if (!prim)
	{
    	__enable_irq();
	}

	//if (s2c_cell_power_is_on) NVIC_EnableIRQ(USART1_IRQn);
	//NVIC_EnableIRQ(USART2_IRQn);
}
#endif

#if 0
	__weak void HAL_SuspendTick(void)
	__weak void HAL_ResumeTick(void)
#endif


#if 0
static void replacement_loop(void)
{
	// 5-18-2022 Mark Watson discovered the ESP32 causes a power dip lasting about 10 us that can trigger the brownout detector.
	// This is a poor-man's PWM to try and avoid that.  See comments in code.

	// The io pin may be responding, but the FET switch has a 1 M ohm pull up (R87)
	// so it will be very slow due to the gate capacitance of Q15.
	// On future builds, a 100 K resistor from the drain of Q14 to the gate of Q15 would help slow the turn on.

	GEN5_WIFI_PWR_EN_Port->BSRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;  // sets the bit
	GEN5_WIFI_PWR_EN_Port->BRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // resets the bit

	for (int i = 0; i < 10; i++)
	{
		for (int j =  0; j < (200 - (20*i)); j++)
		{
			GEN5_WIFI_PWR_EN_Port->BRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // resets the bit
		}
		GEN5_WIFI_PWR_EN_Port->BSRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // sets the bit
	}

}
#endif


#if 0
static void replacement_loop2(void)  // produces "double dip" on scope about 20 us wide.  Keeps voltage above 3. Works
{
	int i,j,k;
	// 5-18-2022 Mark Watson discovered the ESP32 causes a power dip lasting about 10 us that can trigger the brownout detector.
	// This is a poor-man's PWM to try and avoid that.  See comments in code.

	// The io pin may be responding, but the FET switch has a 1 M ohm pull up (R87)
	// so it will be very slow due to the gate capacitance of Q15.
	// On future builds, a 100 K resistor from the drain of Q14 to the gate of Q15 would help slow the turn on.

	__disable_irq();
	GEN5_WIFI_PWR_EN_Port->BSRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;  // sets the bit
	GEN5_WIFI_PWR_EN_Port->BRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // resets the bit
	__enable_irq();
#if 0
#define LOOP2  100
	for (i = 0; i < LOOP2; i++)
	{
		for (j = 0; j < i; j++)
		{
			for (k =  0; k < (LOOP2 - i); k++)
			{
				GEN5_WIFI_PWR_EN_Port->BRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // resets the bit
				//__enable_irq();
			}
			//__disable_irq();
			GEN5_WIFI_PWR_EN_Port->BSRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // sets the bit
		}
	}
#endif
}
#endif


#if 1
static void replacement_loop3(void)  // produces "double dip" on scope about 20 us wide.  Keeps voltage above 3. Works
{

	__disable_irq();
	GEN5_WIFI_PWR_EN_Port->BSRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;  // Sets the bit.
	GEN5_WIFI_PWR_EN_Port->BRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // Resets the bit.  It takes 5 ms before the power is actually switched off to the communication board !
	__enable_irq();
	// At this point the 3.3V rail has dropped to about 3.0V

	//delay_us(20);  // This worked, maybe add margin?
    delay_us(25);  // Let voltage supply rail recover to 3.3v

	__disable_irq();
	GEN5_WIFI_PWR_EN_Port->BSRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;  // sets the bit
	GEN5_WIFI_PWR_EN_Port->BRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // resets the bit
	__enable_irq();

	// At this point the 3.3V rail has dropped to about 3.1V again, however, recovery time is much quicker
    //delay_us(5);
    delay_us(20);

	GEN5_WIFI_PWR_EN_Port->BSRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;  // sets the bit

	// No noticeable dip on 3.3V rail at this point

}
#endif



// 5-18-2022 Mark Watson discovered the ESP32 causes a power dip lasting about 10 us that can trigger the brownout detector.
// This is a poor-man's PWM to try and avoid that.  See comments in code.
static void esp32_turn_on_slowly(void)
{
	if (!sd_ESP32_Enabled)
	{
		log_msg("ESP32 is disabled.");
		return;
	}


	if (isGen5())
	{
      /* Switch VDD_3V3 to PWM mode to ensure strongest power supply */
	  HAL_GPIO_WritePin(GEN5_VDD_3V3_SYNC_Port, GEN5_VDD_3V3_SYNC_Pin, GPIO_PIN_SET);   // Low noise mode (PWM)
	  HAL_Delay(1);    // Wait for 3V3 to stabilize
	}


	// Board id 511 and above uses a hardware method of turning on slowly

	//if (BoardId == BOARD_ID_510)
	{
		//esp32_pull_boot_pin_low();
		//original_loop();
#if 0  // For bench testing response of both 3V3 and 3V3_WiFi rails using scope
		GEN5_WIFI_PWR_EN_Port->BSRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;  // sets the bit
		GEN5_WIFI_PWR_EN_Port->BRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;   // resets the bit. It takes 5 ms before the power is actually switched off to the communication board !
		HAL_Delay(6);
		GEN5_WIFI_PWR_EN_Port->BSRR = (uint32_t)GEN5_WIFI_PWR_EN_Pin;  // sets the bit
#endif
		//replacement_loop();
		//replacement_loop2();
		replacement_loop3();
		//esp32_release_boot_pin();
	}
	//else if (BoardId == BOARD_ID_511)
	//{
      // Turn on power to the ESP32
	  //HAL_GPIO_WritePin(GEN5_WIFI_PWR_EN_Port, GEN5_WIFI_PWR_EN_Pin , GPIO_PIN_SET);
	//}

	// Do not log a message here. It takes time and power. Best to do later once the esp32 is fully on.


	// After esp32 is on, change the 3V3 supply back to low-power mode
	if (isGen5())
	{
	    /* Switch VDD_3V3 to low power mode */
		HAL_GPIO_WritePin(GEN5_VDD_3V3_SYNC_Port, GEN5_VDD_3V3_SYNC_Pin, GPIO_PIN_RESET);   // Low-power mode
	}


}

void set_tx_parameters(void)
{

}


#if 0
static int wait_for_message(uint32_t wait, const char * desired_msg)
{
	uint32_t tickstart;

	uint8_t ch;
	char msg[20];
	int count = 0;

	tickstart = HAL_GetTick();

	while ((HAL_GetTick() - tickstart) < wait)
	{
		if (uart2_getchar(&ch))   // grab one character and process it
		{
			if (count < (sizeof(msg)-1))
			{
				msg[count++] = (char) ch;
				msg[count]=0;   // null terminate as we go
			}

			if (strstr(msg, desired_msg))
			{
#if 0
				char msg[50];
				uint32_t time_to_brownout;
				time_to_brownout = HAL_GetTick() - tickstart;
				sprintf(msg, "Time to esp32 brownout %lu ms", time_to_brownout);   // This was measured at 2 ms
				AppendStatusMessage(msg);
#endif
				return 1;
			}
		}
	}

	return 0;
}
#endif

void esp32_turn_on(void)
{
  // ASSERT: under normal operation (waking from power on, or from sleep) the main capacitor is fully charged at this point.

  // TODO - what if cell modem is on at this point ?

  switch (BoardId)
  {
	default:
	case BOARD_ID_GEN4:
	case BOARD_ID_510:
	case BOARD_ID_511:
		// Turn on power to the ESP32
		esp32_turn_on_slowly();
		break;
	case BOARD_ID_SEWERWATCH_1:
	      // Turn on power to the ESP32.  New boards have a separate power supply chip
		  // This does not appear to work when using USB power supply ?
		  HAL_GPIO_WritePin(GEN5_WIFI_PWR_EN_Port, GEN5_WIFI_PWR_EN_Pin , GPIO_PIN_SET);
		break;
  }

  // Fully charge main capacitor prior to 2nd stage of ESP32 wake up (the BLE turn on uses a rush of current)
  // After measurements, fully charging the cap here has no effect.
  //ltc2943_charge_main_capacitor(THREE_TAU);  // takes 660 ms
}

void esp32_wakeup(void)
{
	int prev_pattern;

	prev_pattern = led_set_pattern(LED_PATTERN_OFF);  // Due to ESP32 power-on "brownout" issues, do NOT use the LED during power up

	uart2_flush_inbuffer();  // discard any lingering data

	log_msg("ESP32 turning power on...");

	ltc2943_trace_measurements("ESP32");  // this can take 64 ms to complete

	// prevent power supply issues ---cannot turn on esp32 if cell is on
	//if (s2c_cell_power_is_on) return;

	esp32_turn_on();

	// Note: the turn on now has about a 1 second delay after ESP32 has power to charge the main capacitor.
	// So, the ESP32 has about 4 more seconds before it will timeout waiting for BLE name


	// Wait for banner message and send BTName if requested.
	// The BLE Uart code waits a maximum of 5 seconds before timing out when it is waiting for the BLE name.
	// This can be used to allow the capacitor to fully re-charge.
	// give the ESP32 a little time to turn on and send the banner message.  1500 was too short for getting MAC from BLE.  Most timing exits early, so just use 4 seconds (4000)
	//parse_banner_message(4000);  // use when testing tryagain code
	if (!parse_banner_message(4000)) // use for production code
	{
		// At this point the ESP32 still has power.  It might possibly even be working correctly.
		// However, set an LED indicator for diagnostics.  An SD card will provide even more diagnostics.
		//STATUS2_LED_ON;

		// Alternatively, could power-on in boot mode, then turn off/on quickly to retain capacitance and reduce power draw ?
		// TODO could set a flag that indicates ESP32 is off and to use the Cell modem as a fallback
		// TODO log error message here?
		log_msg("ESP32 power up problem.");
	}

	uart2_set_tx_parameters();

	//TODO log power on message here?
	//HAL_led_delay(2000);  // wait for the esp32 power usage to stabilize?
	log_msg("ESP32 Power is on");

	led_set_pattern(prev_pattern);  // Restore previous led pattern

}

void esp32_sleep(void)
{

  // Turn off power to the ESP32
  HAL_GPIO_WritePin(GEN5_WIFI_PWR_EN_Port, GEN5_WIFI_PWR_EN_Pin , GPIO_PIN_RESET);


  // The ESP32 starts to emit a message "\r\nBrownout..." when power is cut off.
  // On the iTracker, it generally is able to send at least 3 letters (2 ms) worth of data before shutting down. "\r\nB"
  // As the iTracker has interrupts tied to a UART, it is important to prevent disabling the UART until
  // all letters have arrived, otherwise, the interrupt might fire during this process and the iTracker firmware goes haywire?
  HAL_led_delay(100);

  uart2_flush_inbuffer();  // discard any lingering data


  // Consume any characters that might be sent during the power-down process?
  // Any stray characters that might be sent from the ESP32 during power-down will be flushed at next power on
  //consume_power_down_chars(500);

  log_msg("ESP32 Power is OFF");

}


void esp32_pull_boot_pin_low(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	//PD7 WIFI AUX1 used for ESP32 boot pin
	GPIO_InitStruct.Pin   = GEN5_WIFI_AUX1_Pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_WritePin(GEN5_WIFI_AUX1_Port, GEN5_WIFI_AUX1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_Init(GEN5_WIFI_AUX1_Port, &GPIO_InitStruct);

}

void esp32_release_boot_pin(void)
{
	gpio_unused_pin(PD7);
}
