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
#include "zentri.h"
#include "WiFi.h"
#include "usart.h"
#include "uart2.h"
#include "WiFi_Socket.h"

#include "trace.h"
#include "factory.h"

//#include "webapp.h"   // 5-13-2022 zg  removed.  Scan code for comments near symbol LOAD_ZENTRI_FILES

extern uint32_t WifiIsAwake;


// Workaround to strip incoming CR LF nulls
uint16_t zentri_inbuf_head = 0;
char zentri_inbuf[WIFI_BUF_SIZE];





// General zentri commands.https://docs.zentri.com/zentrios/wz/latest/cmd/apps/tcps-softap

static char ZENTRI_BREAK_SEQ[] __attribute__((unused)) = "$$$$$$\n";
static char ZENTRI_MODE_MACHINE[] __attribute__((unused)) = "set system.cmd.mode machine\n";
static char ZENTRI_MODE_HUMAN[] __attribute__((unused)) = "set system.cmd.mode human\n";
static char ZENTRI_BU_MODE_CMD[] __attribute__((unused)) = "set bu m command\n";// bus.mode command\n";
static char ZENTRI_BU_MODE_STREAM[] __attribute__((unused)) = "set bu m stream\n";// bus.mode stream\n";
static char ZENTRI_ECHO_OFF[] __attribute__((unused)) = "set sy c e 0\n"; // ECHO
static char ZENTRI_MAC[] __attribute__((unused)) = "get wl m\n"; // MAC
static char ZENTRI_SAVE[] __attribute__((unused)) = "save\n";
static char ZENTRI_SLEEP[] __attribute__((unused)) = "sleep\n";
static char ZENTRI_REBOOT[] __attribute__((unused)) = "reboot\n";
static char ZENTRI_FAC_RST[] __attribute__((unused)) = "fac 4C:55:CC:10:09:42\n";

// Buffer, flush, streaming setup.
static char ZENTRI_BU_UART_1[] __attribute__((unused)) = "set bus.log_bus uart1\n";
static char ZENTRI_BU_FLOW_ON[] __attribute__((unused)) = "set uart.flow 0 on\n";
static char ZENTRI_BU_FLUSH_TIME[] __attribute__((unused)) = "set bus.stream.flush_time 10\n";
static char ZENTRI_BU_FLUSH_COUNT[] __attribute__((unused)) = "set bus.stream.flush_count 1024\n";
static char ZENTRI_BU_FLUSH_RESET[] __attribute__((unused)) = "set bu s r 0\n";

// Softap config
static char ZENTRI_SO_SSID[] __attribute__((unused)) = "set so s";  // zg - this is used in an sprintf statement, no cr lf needed
static char ZENTRI_SO_PASSKEY[] __attribute__((unused)) = "set so p"; // zg - this is used in an sprintf statement, no cr lf needed
static char ZENTRI_NETWORK_RESTART[] __attribute__((unused)) = "nre\n";


static char ZENTRI_SO_DNS_URL[] __attribute__((unused)) = "set so n u eastechiq,eastechiq.com,www.eastechiq.com\n";
static char ZENTRI_SO_DNS_ENABLED[] __attribute__((unused)) = "set so n e 1\n";
static char ZENTRI_SO_DHCP_ENABLED[] __attribute__((unused)) = "set so d e 1\n";

static char ZENTRI_SO_STATIC_IP[] __attribute__((unused)) = "set so s i 10.10.10.1\n";
static char ZENTRI_SO_RATE_PROTOCOL[] __attribute__((unused)) = "set so r p auto\n";
static char ZENTRI_SO_AUTOSTART[] __attribute__((unused)) = "set softap.auto_start 1\n";
static char ZENTRI_SO_INTERFACE[] __attribute__((unused)) = "set ne f softap\n";
static char ZENTRI_SO_TIMEOUT[] __attribute__((unused)) = "set so t 600\n";

// HTTP Server
static char ZENTRI_HTTP_SERVER_ENABLED[] __attribute__((unused)) = "set ht s e 1\n";
static char ZENTRI_HTTP_CORS_ORIGIN[] __attribute__((unused)) = "set ht s c *\n";
static char ZENTRI_HTTP_ROOT_FILENAME[] __attribute__((unused)) = "set ht s r index.html\n";

// GPIO
// 18
static char ZENTRI_GP18_INIT[] __attribute__((unused)) = "set gp i 18 none\n";
static char ZENTRI_GP18_DIR[] __attribute__((unused)) = "gdi 18 -1\n";
static char ZENTRI_GP18_BU_GPIO[] __attribute__((unused)) = "set bu s g 18\n";
// 20
static char ZENTRI_GP20_INIT[] __attribute__((unused)) = "set gp i 20 none\n";
static char ZENTRI_GP20_DIR[] __attribute__((unused)) = "gdi 20 -1\n";
static char ZENTRI_GP20_WS_CONNECTED[] __attribute__((unused)) = "set ht s x 20\n"; // Indicates websocket is connected pin
static char ZENTRI_GP20_SLEEP_LOW[] __attribute__((unused)) = "set gp s 20 output_low\n";
// 22
static char ZENTRI_GP22_INIT[] __attribute__((unused)) = "set gp i 22 none\n";  // zg - sets gpio.init variable so setting is saved and used at boot; deregisters gpio22
static char ZENTRI_GP22_DIR[] __attribute__((unused)) = "gdi 22 -1\n";   // zg - This deregisters pin 22
static char ZENTRI_GP22_WAKEUP_EVENT[] __attribute__((unused)) = "set sy w e gpio22\n";  // zg - This assigns wake up event to gpio22
// 23
static char ZENTRI_GP23_INIT[] __attribute__((unused)) = "set gp i 23 none\n";
static char ZENTRI_GP23_DIR[] __attribute__((unused)) = "gdi 23 -1\n";
static char ZENTRI_GP23_INDICATOR_GPIO[] __attribute__((unused)) = "set sy i g softap 23\n";
static char ZENTRI_GP23_INDICATOR_STATE[] __attribute__((unused)) = "set sy i s softap static_off|static_off|static_off|static_on\n";
static char ZENTRI_GP23_SLEEP_LOW[] __attribute__((unused)) = "set gp s 23 output_low\n";

// Wifi radio
static char ZENTRI_WL_ANTENNA_INT[] __attribute__((unused)) = "set wlan.antenna.select 1\n";
static char ZENTRI_WL_ANTENNA_EXT[] __attribute__((unused)) = "set wlan.antenna.select 2\n";
static char ZENTRI_WL_AUTOJOIN[] __attribute__((unused)) = "set wlan.auto_join.enabled 0\n";
static char ZENTRI_WL_TX_POWER[] __attribute__((unused)) = "set wlan.tx_power 18\n";



int  zentri_response_code = 0;
int  zentri_response_len = 0;
int  zentri_response_offset = 0;

int  zentri_extra_bytes_code = 0;
int  zentri_extra_bytes_len = 0;






#if 0
void zentri_get_extra_bytes_response(char * text, int maxlen)  // use sizeof(text) for maxlen
{
	int i;
	for (i = 0; i < (zentri_extra_bytes_len-2); i++)
	{
		if (i < maxlen)
		{
		  text[i] = zentri_inbuf[i + 7];  // note that the ISR code strips \r and \n
		}
	}
	if (i > maxlen) i = maxlen;
	text[i] = '\0';
}
#endif

void zentri_get_response(char * text, int maxlen)  // use sizeof(text) for maxlen
{
	int i;
	for (i = 0; i < (zentri_response_len-2); i++)
	{
		if (i < maxlen)
		{
		  text[i] = zentri_inbuf[i + 7 + zentri_response_offset];  // note that the ISR code strips \r and \n
		}
	}
	if (i > maxlen) i = maxlen;
	text[i] = '\0';
}

static int parse_extra_bytes_and_response(void)
{
    // convert response code digit to integer
	zentri_extra_bytes_code = zentri_inbuf[1] - '0';

    // convert length to an integer
	zentri_extra_bytes_len  =  (zentri_inbuf[2] - '0') * 10000;
	zentri_extra_bytes_len +=  (zentri_inbuf[3] - '0') *  1000;
	zentri_extra_bytes_len +=  (zentri_inbuf[4] - '0') *   100;
	zentri_extra_bytes_len +=  (zentri_inbuf[5] - '0') *    10;
	zentri_extra_bytes_len +=  (zentri_inbuf[6] - '0') *     1;

    //if (zentri_extra_bytes_len == 0) return RESPONSE_COMPLETE;

	// zg - note: the ISR RX code strips \r and \n

	// Compute where the response header "R" should start

	zentri_response_offset = 7 + zentri_extra_bytes_len - 2;

	if (zentri_inbuf_head < zentri_response_offset) return WIFI_RESPONSE_WAITING_FOR_RX;      // all data not yet received

	if (zentri_inbuf[zentri_response_offset] != 'R')
	{
		return WIFI_RESPONSE_ERROR;    // not a valid header
	}

    // convert response code digit to integer
    zentri_response_code = zentri_inbuf[zentri_response_offset + 1] - '0';

    // convert length to an integer
    zentri_response_len  =  (zentri_inbuf[zentri_response_offset + 2] - '0') * 10000;
    zentri_response_len +=  (zentri_inbuf[zentri_response_offset + 3] - '0') *  1000;
    zentri_response_len +=  (zentri_inbuf[zentri_response_offset + 4] - '0') *   100;
    zentri_response_len +=  (zentri_inbuf[zentri_response_offset + 5] - '0') *    10;
    zentri_response_len +=  (zentri_inbuf[zentri_response_offset + 6] - '0') *     1;

    if (zentri_response_len == 0) return WIFI_RESPONSE_COMPLETE;

	// zg - note: the ISR RX code strips \r and \n

	if (zentri_inbuf_head < (zentri_response_offset+7+zentri_response_len-2)) return WIFI_RESPONSE_WAITING_FOR_RX;      // all data not yet received

	return WIFI_RESPONSE_COMPLETE;  // data can be found starting at inbuf2[zentri_response_offset+7] to [zentri_response_offset+7+zentri_response_len-2]
}

static int parse_response(void)
{

    // convert response code digit to integer
    zentri_response_code = zentri_inbuf[1] - '0';

    // convert length to an integer
    zentri_response_len  =  (zentri_inbuf[2] - '0') * 10000;
    zentri_response_len +=  (zentri_inbuf[3] - '0') *  1000;
    zentri_response_len +=  (zentri_inbuf[4] - '0') *   100;
    zentri_response_len +=  (zentri_inbuf[5] - '0') *    10;
    zentri_response_len +=  (zentri_inbuf[6] - '0') *     1;

    if (zentri_response_len == 0) return WIFI_RESPONSE_COMPLETE;

    // note zentri_response_len will be 3 or greater at this point

	// zg - note: the ISR RX code strips \r and \n

	if (zentri_inbuf_head < (7+zentri_response_len-2)) return WIFI_RESPONSE_WAITING_FOR_RX;      // all data not yet received

	return WIFI_RESPONSE_COMPLETE;  // data can be found starting at inbuf2[7] to [7+zentri_response_len-2]

}



static void copy_and_strip(void)
{
	int i;
	char ch;
	zentri_inbuf_head = 0;
	for (i=0; i < uart2_inbuf_head; i++)
	{
		ch = uart2_inbuf[i];

		// Strip carriage returns, nulls, etc.
		if ((ch != '\r') && (ch != '\n') && (ch != '\0'))
		{
			zentri_inbuf[zentri_inbuf_head] = ch;  // save the incoming character in the array
			zentri_inbuf_head++;             // increment to next position
		}
	}
}

static int parse_zentri_response(void)
{
	// A Zentri header is of the form:  RcLLLLL[\r\n<data>]
	// where c = result code
	//       L = length of bytes to follow
	// if L is non-zero, the first two bytes are: \r\n


	// Note: the original ISR RX code striped the \r\n so an actual response looks like:
	// For example: R000008Set OK

	copy_and_strip();


	// Note that a file_delete command returns ...  zg I think this is a documentation error, the actual device may not return the L...
	// fde myfile.txt
	// L000014
	// File deleted
	// R000009
	// Success
	//
	// Or, if the file does not exist:
	// R100016Command failed
	//
	// the file_create command apparently returns "R000014File created\r\n"

	if (zentri_inbuf_head < 7) return WIFI_RESPONSE_WAITING_FOR_RX;  // no complete header yet
    if (zentri_inbuf[0] == 'L')
    {
    	// There are additional bytes that precede the response header
    	return parse_extra_bytes_and_response();
   	}
    else if (zentri_inbuf[0] == 'R')
    {
    	zentri_response_offset = 0;
    	return parse_response();
   	}
    else if (zentri_inbuf[0] == 'S')
    {
    	zentri_response_offset = 0;
    	return parse_response();
   	}
    return WIFI_RESPONSE_ERROR;    // not a valid header
}



int zentri_wait_for_response(uint32_t timeout_ms)
{
	int response;

	response = parse_zentri_response();

	uart2_timeout = timeout_ms;  // RX should end within requested timeout

	while (response == WIFI_RESPONSE_WAITING_FOR_RX)
	{
		led_heartbeat();
		if (uart2_timeout)
		{
			response = parse_zentri_response();
		}
		else
		{

			response = WIFI_RESPONSE_TIMEOUT;
		}
	}
	if (sd_WiFiCmdLogEnabled)
	{
		int i;
		if (response == WIFI_RESPONSE_TIMEOUT) trace_AppendMsg("\nRX Error: timeout\n");
		for (i=0; i < zentri_inbuf_head; i++)
		{
		    trace_AppendChar(zentri_inbuf[i]);
		}
	}

	return response;
}




void zentri_send_command_no_wait(char * txBuff)
{
	uint32_t txSize;

	uart2_flush_inbuffer();  // Prepare RX buffer for parsing command response
	zentri_response_len = 0;
	zentri_response_offset = 0;
	zentri_extra_bytes_len = 0;

	// Send the command.
	txSize = strlen(txBuff);

	if (txSize == 0) return; // nothing to send, all done

	strcpy((char *)uart2_outbuf, txBuff);  // Copy the string into the ISR output buffer.

	uart2_tx(txSize, 0);  // send the command bytes, no logging, they are logged below

	if (sd_WiFiCmdLogEnabled)
	{
	  trace_AppendMsg("\r\n\r\nTX");  // need CR LF to move from previous RX, then add a blank line
	  trace_AppendMsg(txBuff);  // this ends with CR LF already, so...
	}

}

static void log_RX(int result, uint32_t Duration)
{
	char duration_msg[35];

	sprintf(duration_msg,"R%d %luX\r\n", result, Duration);

	trace_AppendMsg(duration_msg);
	//trace_AppendMsg(inbuf2);  // This technique requires inbuf2 to always be null-terminated, not good for sockets
}

int zentri_send_command_timeout(char * txBuff, uint32_t timeout_ms)
{
	int result;
	uint32_t tickstart;  // When the command TX was started
	uint32_t duration;   // Duration of the last command (time from TX to RX complete)

	tickstart = HAL_GetTick();

	zentri_send_command_no_wait(txBuff);

    // Wait for a response
	result = zentri_wait_for_response(timeout_ms);

	duration = HAL_GetTick() - tickstart;

	if (sd_WiFiCmdLogEnabled) log_RX(result, duration);

	return result;  // returns 0 for success, non-zero if failure
}

int zentri_send_command(char * txBuff)
{
	return zentri_send_command_timeout(txBuff, 2000);
}


#if 0  // INFO ON WAKE/SLEEP

https://www.silabs.com/community/wireless/wi-fi/knowledge-base.entry.html/2018/07/30/sleep_and_wake_opera-tAgx

The only way to wake-up the AMW037/AMW007 module is when the timer expires. Once module sleeps, it will wakeup once system.wakeup.timeout expires. More here:
https://docs.zentri.com/zentrios/wl/latest/cmd/commands#sleep
https://docs.zentri.com/zentrios/wl/latest/cmd/variables/system#system-wakeup-timeout

The WAKE pin is an output from the module, and have to be be used to wake the module from sleep state. Connect WAKE to RESET_N using a 1k resistor to enable sleep/wake functionality.

The operation of wakeup is that once sleep timer expires, it triggers the wake pin (which is tied to the reset pin) which resets the module. This is the only way AMW007 wakes itself up. There is no other modes of sleep.

This connection is not required if sleep/wake functionality is not used, or a host MCU has explicit control of the RESET_N pin.

Another topic in the forums:
https://www.silabs.com/community/wireless/wi-fi/knowledge-base.entry.html/2018/07/02/for_amwxxx_familyz-D45j

Ayman Grais
Employee
Level 7
Issuing 'reboot' command is very similar to resetting the module via RESET pin. Though, there is a couple of differences here:

1. RESET_N is tied to the MCU hardware signal. When the pin is assert the MCU is immediately reset, there is no software intervention.
While the 'reboot' command will bring down the network (close open stream and disassociate from WLAN/disconnect softap client) and cleanup
any running hardware (e.g. stop PWMs, ADCs) before performing a software reset of the MCU.

2. Toggling the RESET_N pin does not cause any text output on the serial bus, whereas issuing a 'reboot' command prints the version string.

Knowledge Base Articles AMW006 AMW106 Zentri Legacy Wi-FI Modules Wi-Fi

#endif


// Trigger the wifi GPIO.
void zentri_reset(void)
{
	HAL_GPIO_WritePin(GEN4_Zentri_Reset_Port, GEN4_Zentri_Reset_Pin, 0);
	HAL_Delay(1);  // zg - originally this was 200.  Probably just need an edge to trigger....
	HAL_GPIO_WritePin(GEN4_Zentri_Reset_Port, GEN4_Zentri_Reset_Pin, 1);
	HAL_led_delay(200);  // zg - give the unit time to wake up from reset ?
}





// This is "CMD" mode for command (AT commands),
// OR "STR" mode for sending/transferring strings/data.
void zentri_set_mode(uint8_t val)
{
	HAL_GPIO_WritePin(GEN4_Zentri_CMD_STR_Port, GEN4_Zentri_CMD_STR_Pin, val);

	// Must delay here so that sending a CMD immediately after will succeed.

	//HAL_Delay(5);     // zg - didn't work
	//HAL_Delay(100);   // zg - didn't work
	//HAL_Delay(500);   // zg - didn't work
	HAL_led_delay(750); // zg - works
	//HAL_Delay(1000);  // zg - works
	//HAL_Delay(3000);  // zg - works, but why wait 3 sec here?
}

int8_t zentri_connected(void)
{
	return (HAL_GPIO_ReadPin(GEN4_Zentri_OOB_Port, GEN4_Zentri_OOB_Pin));
}

int8_t zentri_socket_open(void)
{
	return (HAL_GPIO_ReadPin(GEN4_Zentri_SC_Port, GEN4_Zentri_SC_Pin));
}



void zentri_enter_command_mode(void)
{
	// Wakeup
	MX_USART2_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_RTS_CTS);  // This is here because at power on and during some SD card activated functions a call to wifi_wakeup() is bypassed.
	WifiIsAwake = 1;

    // The hardware reset is required, so that we can talk to the modem via AT commands.
	zentri_reset();  // zg - this is a hardware reset, not a controlled shutdown.

	// Set command mode for programming
	zentri_set_mode(ZENTRI_CMD_MODE);

	zentri_send_command(ZENTRI_BU_MODE_CMD);

	// Turn off the echo.
	zentri_send_command(ZENTRI_ECHO_OFF);

	// Enter "machine mode".
	zentri_send_command(ZENTRI_MODE_MACHINE);
}

// Zentri Factory Reset
void zentri_factory_reset(char *mac_address, uint8_t bErase)
{
	char cmd_buf[75];
	char *pMAC;

	zentri_enter_command_mode();

	if (strlen(mac_address) == 0)
	{
		// Get the MAC, if possible

		zentri_send_command(ZENTRI_MAC);
		// Save the MAC address if received.
		// zg - Note this depends on CR LF being stripped by RX ISR
		// zg - Note also that it is subject to race conditions with the ISR, even though
		// zg - the low-level routines have built-in wait cycles.
		for (int i = 0; i < 17; i++) {
			MainConfig.MAC[i] = zentri_inbuf[i + 7];
		}

		pMAC = MainConfig.MAC;
	}
	else
	{
		pMAC = mac_address;
	}

	if (bErase)
	{
		// Tell the Zentri to perform a factory erase (must include the MAC to avoid accidental erase)
		sprintf(cmd_buf, "format extended %s\r\n", pMAC);
	}
	else
	{
		// Tell the Zentri to perform a factory reset (must include the MAC to avoid accidental reset)
		sprintf(cmd_buf, "factory_reset %s\r\n", pMAC);
	}

	//wifi_send_command(cmd_buf);  // this times out in about 2 seconds which is too short

	zentri_send_command_timeout(cmd_buf, 60000); // try one full minute

}

// Initial Zentri config.

void zentri_config(int LoadZentriFiles)
{
	char cmd_buf[75];


	zentri_enter_command_mode();

#if 0 // 5-13-2022 zg  removed.  Scan code for comments near symbol LOAD_ZENTRI_FILES
	// Upload any new Zentri files here, then proceed with initialization
	if (LoadZentriFiles == LOAD_ZENTRI_FILES)
	{
	  PerformFactoryLoadWebAppFiles();
	}
#endif

	// Get the MAC.

	zentri_send_command(ZENTRI_MAC);
	// Save the MAC address if received.
	// zg - Note this depends on CR LF being stripped by RX ISR
	// zg - Note also that it is subject to race conditions with the ISR, even though
	// zg - the low-level routines have built-in wait cycles.
	for (int i = 0; i < 17; i++) {
		MainConfig.MAC[i] = zentri_inbuf[i + 7];
	}

	// Tell the Zentri the new SSID
	sprintf(cmd_buf, "%s %s\r\n", ZENTRI_SO_SSID, MainConfig.SSID);
	zentri_send_command(cmd_buf);


	// Set the PASSKEY next.
	sprintf(cmd_buf, "%s %s\r\n", ZENTRI_SO_PASSKEY, WIFI_SO_PASSKEY_VAL);
	zentri_send_command(cmd_buf);

	// zg - why is this here?  Is there any chance the user wants to assign a password ?
	strcpy(MainConfig.PassKey, WIFI_SO_PASSKEY_VAL);  // this will erase any user-entered values

	// Buffer, flush, streaming setup.
	zentri_send_command(ZENTRI_BU_UART_1);
	zentri_send_command(ZENTRI_BU_FLOW_ON);
	zentri_send_command(ZENTRI_BU_FLUSH_TIME);
	zentri_send_command(ZENTRI_BU_FLUSH_COUNT);

	// Server
	zentri_send_command(ZENTRI_SO_DNS_URL);
	zentri_send_command(ZENTRI_SO_DNS_ENABLED);
	zentri_send_command(ZENTRI_HTTP_SERVER_ENABLED);
	zentri_send_command(ZENTRI_HTTP_CORS_ORIGIN);
	zentri_send_command(ZENTRI_HTTP_ROOT_FILENAME);

	// Softap config (webapp)
	zentri_send_command(ZENTRI_WL_AUTOJOIN);
	zentri_send_command(ZENTRI_SO_DHCP_ENABLED);
	zentri_send_command(ZENTRI_SO_STATIC_IP);
	zentri_send_command(ZENTRI_SO_RATE_PROTOCOL);
	zentri_send_command(ZENTRI_SO_AUTOSTART);
	zentri_send_command(ZENTRI_SO_INTERFACE);
	zentri_send_command(ZENTRI_SO_TIMEOUT);

	// Wifi radio
	if (IsWiFiInternalAnt())
	{
		zentri_send_command(ZENTRI_WL_ANTENNA_INT);
	}
	else
	{
		zentri_send_command(ZENTRI_WL_ANTENNA_EXT);
	}
	zentri_send_command(ZENTRI_WL_TX_POWER);

	// GPIO CONFIGURATION
	// 18 (Wifi Cmd Str)
	zentri_send_command(ZENTRI_GP18_INIT);
	zentri_send_command(ZENTRI_GP18_DIR);
	zentri_send_command(ZENTRI_GP18_BU_GPIO);
	// 20 (Wifi SC - Socket Connected)
	zentri_send_command(ZENTRI_GP20_INIT);
	zentri_send_command(ZENTRI_GP20_DIR);
	zentri_send_command(ZENTRI_GP20_WS_CONNECTED);
	zentri_send_command(ZENTRI_GP20_SLEEP_LOW);

	// 22 (Wifi WU - Wakeup)  zg - I am not sure GPIO22 is needed at all, especially as a wakeup event
	zentri_send_command(ZENTRI_GP22_INIT);
	zentri_send_command(ZENTRI_GP22_DIR);
	//wifi_send_command(WIFI_GP22_WAKEUP_EVENT);  // zg - I don't think this is needed.  Wakeup is performed via RESET_N pin.

	// 23 (Wifi OOB - Connected)
	zentri_send_command(ZENTRI_GP23_INIT);
	zentri_send_command(ZENTRI_GP23_DIR);
	zentri_send_command(ZENTRI_GP23_INDICATOR_GPIO);
	zentri_send_command(ZENTRI_GP23_INDICATOR_STATE);
	zentri_send_command(ZENTRI_GP23_SLEEP_LOW);

	// Leave in stream mode.
	zentri_send_command(ZENTRI_BU_MODE_STREAM);

	// Save and reboot.
	zentri_send_command(ZENTRI_SAVE);  // zg - measured 875 ms to get response
	//HAL_Delay(500); // Save takes a bit.
	zentri_send_command(ZENTRI_REBOOT); // zg - measured 788 ms to get response
	//HAL_Delay(11000); // Must receive response "ZENTRI-AMW106" after reboot completes. About 10s.

	// Set GPIO for STREAM mode for data and websockets.
	zentri_set_mode(ZENTRI_STR_MODE);
}

#if 0  // not tested yet...
void zentri_update_SSID(void)
{

char cmd_buf[75];

zentri_enter_command_mode();

// Tell the Zentri the new SSID
sprintf(cmd_buf, "%s %s\r\n", WIFI_SO_SSID, MainConfig.SSID);
zentri_send_command(cmd_buf);

zentri_send_command_no_wait(ZENTRI_NETWORK_RESTART);

}
#endif

void zentri_wakeup(void)
{

	zentri_reset();   // zg - this is a hardware reset.  Why didn't pin 22 (gpio22) wake it up?

	zentri_set_mode(ZENTRI_STR_MODE);
}

void zentri_sleep(void)
{
	// Put into command mode and put to to sleep.
	zentri_set_mode(ZENTRI_CMD_MODE);

	zentri_send_command_no_wait(ZENTRI_SLEEP);  // zg - there is no response to the sleep command, so no sense waiting for one.
}
