/*
 * s2c.c
 *
 *  Created on: OCT 20, 2019
 *  Updated LTE-CATM functions
 *  Added findall routine for find a substring inside a string to fix an issue with the find function
 *      Author: S2C
 */
#include "stm32l4xx_hal.h"
#include "uart1.h"
#include "lpuart1.h"
#include "debug_helper.h"

#include "stdlib.h"  // atoi


#include "Common.h"
#include "s2c.h"
#include "gpio.h"
#include "usart.h"
#include "uart1.h"
#include "uart2.h"
#include "uart3.h"
#include "uart4.h"
#include "lpuart1.h"
#include "string.h"
#include "stdio.h"
#include "Log.h"
#include "trace.h"
#include "factory.h"
#include "version.h"
#include "uart1.h"
#include "testalert.h"
#include "quadspi.h"
#include "rtc.h"
#include "LTC2943.h"

/*
 * SIGNAL STRENGTH - AT+CSQ?
 * Value	RSSI dBm	Condition
 *	2	-109	Marginal
 *	3	-107	Marginal
 *	4	-105	Marginal
 *	5	-103	Marginal
 *	6	-101	Marginal
 *	7	-99	Marginal
 *	8	-97	Marginal
 *	9	-95	Marginal
 *	10	-93	OK
 *	11	-91	OK
 *	12	-89	OK
 *	13	-87	OK
 *	14	-85	OK
 *	15	-83	Good
 *	16	-81	Good
 *	17	-79	Good
 *	18	-77	Good
 *	19	-75	Good
 *	20	-73	Excellent
 *	21	-71	Excellent
 *	22	-69	Excellent
 *	23	-67	Excellent
 *	24	-65	Excellent
 *	25	-63	Excellent
 *	26	-61	Excellent
 *	27	-59	Excellent
 *	28	-57	Excellent
 *	29	-55	Excellent
 *	30	-53	Excellent
 *
 *  Created on: APR 5, 2019
 *      Author: S2C
 */


// The target cell modem is:  Multitech mtsmc-mna1-sp
// Please refer to the data sheet and programming manuals for details.
// This code is specific to that device and model.
// Timeout values are taken from another manual, dated 2006.
// No other timeout values could be found.
// Timeouts were empirically adjusted.
//
// March 5, 2021 Found a reference guide for the older modems
// which listed timeouts and the minimum delay between commands (20 ms)
//     AT Commands Reference Guide
//         80407ST10116A Rev.12 � 2015-07-02
//


#define TELIT_POWER_ON_DELAY   10000   //  10 s - Telit modem warm up must be at least this long, otherwise it does not respond to commands.
#define LAIRD_POWER_ON_DELAY   20000   //  15 s test for marginal Laird Units.  Prefer to see +KRUP but....this tends to give more time for CEREG to complete

// Telit AT Commands Reference Guide 80000ST10025a Rev. 0 - 04/08/06

#define CGSN_TIMEOUT     20000   //  20 s
#define CGMM_TIMEOUT     20000   //  20 s --- this was originally  5 s and it failed on an actual modem...
#define CGDCONT_TIMEOUT  20000   //  20 s
//#define COPS_TIMEOUT    180000   // 180 s   --- this CMD is not needed for operation, just info.
#define CEREG_TIMEOUT    20000   //  20 s
//#define CGPADDR_TIMEOUT  20000   //  20 s
#define CIMI_TIMEOUT     20000   //  20 s
#define CPIN_TIMEOUT     20000   //  20 s
#define SCFG_TIMEOUT     10000   //  10 s
#define CCLK_TIMEOUT     10000   //  10 s
#define SGACT_TIMEOUT   150000   //  150 seconds per 2015 ref guide
#define SGACTQUERY_TIMEOUT   1000   //  seems to be very fast

#define ATE_TIMEOUT      10000   //  10 s
#define CSQ_TIMEOUT      10000   //  10 s
#define FWSWITCH_TIMEOUT 20000   //   20 s - this is a guess.  The command will perform a cell modem system reboot...
#define SD_TIMEOUT      140000   //  140 seconds per 2015 ref guide
#define BREAK_TIMEOUT    10000   //  not present in specs;
#define SH_TIMEOUT       10000   //  10 seconds per 2015 ref guide
#define SHDN_TIMEOUT     15000   //  15 s - note datasheet recommends waiting 30 seconds before powerdown


uint32_t s2c_HwFlowCtl = UART_HWCONTROL_NONE;

// retry




static uint32_t s2c_resend_record_number = 0;

//#define TEST_RESEND

#ifdef TEST_RESEND

#define TEST_RESEND_DISABLED  0
#define TEST_RESEND_ARMED     1
#define TEST_RESEND_TRIGGERED 2
int test_resend_state= TEST_RESEND_DISABLED;
int test_resend_ack = ACK_RECEIVED;
int test_resend_rec_number = 0;

#endif



int  s2c_CellModemFound = S2C_NO_MODEM;
char s2c_imei[15+1] = {0};  // 15 decimal digits plus null terminator

static uint16_t s2c_try_band = 0;

static uint32_t cell_powerup_time = 0;
static uint32_t cell_powerdown_time = 0;

uint32_t s2c_Wake_Time = 0;  // Time to wake up

// Variables that keep track of network registration state for the most recent power-on cycle.
// Must be set to zero when power goes off.

static int state_CGDCONT = 0;
static int state_KCNXCFG = 0;
static int state_CEREG = 0;
static int state_SCFG = 0;
static int state_SGACT_SET = 0;
static int state_SGACT_QUERY = 0;
static int state_NETWORK_CONNECTED = 0;
static int state_TCP_CONFIGURED = 0;
static int state_SOCKET_OPEN = 0;
static int state_LAIRD_MUX = 0;

static uint32_t last_successful_registration_time = 0; // This is the time the CEREG command was last successful
static uint32_t last_failed_registration_time = 0;     // This is the time the cell was powered off due to a 30 minute timeout
static uint32_t last_attempted_registration_time = 0;  // This is the time the CEREG command was last attempted
//static uint32_t last_attempted_open_socket_time = 0;  // This is the time the KTCPSTART command was last attempted


// Cell Modem States
#define CELL_OFF                               0 // cell modem power is OFF
#define CELL_WAIT_FOR_WIFI_HOLDOFF             1 // delay until WiFi holdoff has expired (if any)
#define CELL_WAITING_FOR_WARMUP                2 // delay until cell modem hardware is on
#define CELL_WAITING_FOR_CONFIRM_HW            3 // waiting for cell power up and (one time) confirm hw process to complete
#define CELL_WAITING_FOR_CONFIGURATION         4 // configuration that must be done each time cell is powered on (none at present)
#define CELL_WAITING_FOR_REGISTRATION          5 // wait up to 30 minutes to register on cell network
#define CELL_WAITING_FOR_OPEN_SOCKET           6
#define CELL_WAITING_FOR_TRIGGER               7 // waiting for trigger to send data over open socket, reconfig modem, etc
#define CELL_WAITING_FOR_CLOSE_SOCKET          8 // waiting for close socket to propogate through the network before power down?


static int cell_state = CELL_OFF;

#define TRIGGER_NONE            0
#define TRIGGER_CONFIRM_HW      1
#define TRIGGER_FACTORY_CONFIG  2    // {"comm":33}
#define TRIGGER_INTERNAL_SIM    3    // {"comm":35}
#define TRIGGER_EXTERNAL_SIM    4    // {"comm":36}
#define TRIGGER_CELL_BAND       5    // {"cell_band":n} where n = 2, 4, 12, 13, 17, 9001, 9064      9001 (all USA bands)   n=9064 (all NZ bands)  (9000 + Country Code)
#define TRIGGER_POWER_ON_PING   6
#define TRIGGER_CELLULAR_SEND   7
#define TRIGGER_POWER_ON        8
#define TRIGGER_POWER_OFF       9


static int cell_trigger = TRIGGER_NONE;


#define NO_COMMA   0
#define ADD_COMMA  1

int s2c_current_alarm_state = NO_ALARM;
int s2c_last_alarm_state_sent =  NO_ALARM;
log_t * s2c_pAlertLogRec = NULL;


// Forward references
static uint32_t s2c_get_power_on_duration_sec(void);
static uint32_t s2c_get_power_off_duration_sec(void);
static void update_error(char *msg);
void laird_report_radio_info(void);

// Original specification anticipated a float value of maximum 99.99 inches  nn.nn
// However, SewerWatch can go beyond that, for example nnn.nn
// If that happens, the string array bounds can apparently overflow depending on the number of elements in the array.
//
static void append_to_array(char * arry, float val, int ndigits, int addComma)
{
	int dec2;
	int dec1;
	char buff[10]; // zg is this big enough?

#if 1  // omit to induce failure
	if (val < -99.99)
	{
		if (ndigits > 0)
		{
			ndigits = 0;  // must make room for negative sign -nnn.  (but the decimal point is dropped)
		}
	}
	else if (val < 0)
	{
		if (ndigits > 1)
		{
			ndigits = 1;  // must make room for negative sign -nn.n
		}
	}
	else if (val > 99.99)
	{
		if (ndigits > 1)
		{
			ndigits = 1;  // can only print a max 4 significant digits nnn.n
		}
	}
#endif
	dec1 = (int) val;
	if (ndigits == 0)
	{
		sprintf(buff, "%d", dec1);
	}
	else if (ndigits == 1)
	{
		dec2 = (unsigned int) ((val - dec1) * 10);

		sprintf(buff, "%d.%01d", dec1, dec2);
	}
	else
	{
		dec2 = (unsigned int) ((val - dec1) * 100);

		sprintf(buff, "%d.%02d", dec1, dec2);
	}
	strcat(arry, buff);
	if (addComma == 1) strcat(arry,",");
}

#if 0

void test_fix(void)
{

	char gain_array_str[60];     // nnn,nnn       4*12 = 48 + null = 49
	char distance_array_str[80]; // nn.nn,nn.nn   6*12 = 72 + null = 73  For SewerWatch nnn.nn,
	char vertical_mount_array_str[80]; // nn.nn,nn.nn   6*12 = 72 + null = 73

	float gain;
	float distance;
	float vertical_mount;

	int16_t comma;

	comma = ADD_COMMA;

	gain_array_str[0] = '\0';
	distance_array_str[0] = '\0';
	vertical_mount_array_str[0] = '\0';

	gain = 254.09;
	distance = 254.09;
	vertical_mount = distance;

	append_to_array(gain_array_str, gain, 0, comma);

	append_to_array(distance_array_str, distance, 2, comma);

	append_to_array(vertical_mount_array_str, vertical_mount, 2, comma);
}

void test_fix(void)
{
	try(254.09, 48.68, 48.54, 48.95, 142.11);
}


#endif


static int getStatus(const char * cmdr)
{
	int output = 0;
	char status = '\0';
	for (int index = 0; index < strlen(cmdr); index++)
	{
		if (cmdr[index] == ',')
		{
			status = cmdr[index + 1];
			break;
		}
	}

	if (status == '1' || status == '5')   // 1 = registered, home network, 5 = registered, roaming
	{
		output = 1;
	}

	return output;
}

static int get_session_id(const char * cmdr)
{
	// Parse the following response
	// +KTCPCFG: <session_id>

	char status = '\0';
	for (int index = 0; index < strlen(cmdr); index++)
	{
		if (cmdr[index] == ':')
		{
			status = cmdr[index + 2];
			break;
		}
	}
	switch (status)
	{
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	default: break;
	}

    return 0;
}

static int IndexOf(const char * cmdr, char c)
{
	int p = -1;
	for (int index = 0; index < strlen(cmdr); index++)
	{
		if (cmdr[index] == c)
		{
			p = index;
			break;
		}
	}
	return p;
}

#if 0
static int find(const char * cmdr, const char * str) /* check if a substring exist inside a given string. then return the starting position */
{
	int p = -1;
	int IsFound = 1;
	for (int index = 0; index < strlen(cmdr); index++)	{	if (cmdr[index] == str[0]  ) { p = index;	break;	}}
	// Now make sure that all the characters are present
	for (int i = 0; i < strlen(str); i++)
	{
		if (cmdr[p+i] != str[i]) { IsFound = 0; }
	}
	return IsFound;
}


static int findall(const char * cmdr, const char * str) /* check if a substring exist inside a given string. then return the starting position */
{
	int p = -1;
	int IsFound = 1;
	for (int index = 0; index < strlen(cmdr); index++)
		{
			//if (cmdr[index] == str[0]  ) { p = index;	break;	}}
			// Now make sure that all the characters are present
			IsFound = 1;
			p = index;
			for (int i = 0; i < strlen(str); i++)
			{
				if (cmdr[p+i] != str[i]) { IsFound = 0; break;}
			}
			if(IsFound) break;
		}
	return IsFound;
}
#endif


#if 0
static void ZEROALL(char * arr, int sz)
{
	for (int i = 0; i < sz; i++)
	{
		arr[i] = '\0';
	}
}
#endif

extern int8_t cell_parse_incoming_JSON(const char *p);



//
// parse inbuf1 for:
//        ack     {"1":"K"}
//        Resend  {"1":"R","N":"123"}
//     or Verbose {"1":"T","N":"123"}
//     or {"1":"R","N":"0",...}  where ... is sent to the WiFi JSON parser...
static int parse_ack(void)
{
	int ack = ACK_RECEIVED;

	s2c_resend_record_number = 0;
	if (strstr((const char *)uart1_inbuf,"{\"1\":\"R\",\"N\":\"0\","))
	{
		s2c_resend_record_number = (uint32_t) atoi((const char *)uart1_inbuf+14);
		ack = ACK_RESEND;
		// parse additional JSON separately
		uart1_inbuf[16] = '{';  // replace the comma with an open bracket
		cell_parse_incoming_JSON((const char *)&uart1_inbuf[16]);
	}
	else if (strstr((const char *)uart1_inbuf,"{\"1\":\"R\",\"N\":\""))
	{
		s2c_resend_record_number = (uint32_t) atoi((const char *)uart1_inbuf+14);
		ack = ACK_RESEND;
	}
	else if (strstr((const char *)uart1_inbuf,"{\"1\":\"T\",\"N\":\""))
	{
		s2c_resend_record_number = (uint32_t) atoi((const char *)uart1_inbuf+14);
		ack = ACK_RESEND_VERBOSE;
	}

	return ack;
}


#if 0
void test_parse(void)
{
	strcpy(uart1_inbuf,"{\"1\":\"R\",\"N\":\"123\"}");
	parse_ack();
	strcpy(uart1_inbuf,"{\"1\":\"T\",\"N\":\"456\"}");
	parse_ack();

}
#endif

static void SPY5_echo_inbound_chars(void)
{
	uint8_t RxChar;
	uint16_t inbuf_tail = uart1_inbuf_tail;
	uint16_t inbuf_head = uart1_inbuf_head;

	while (inbuf_tail != inbuf_head)
	{
		// Extract the next character to be processed
		RxChar = uart1_inbuf[inbuf_tail];

		lpuart1_putchar(&RxChar);  // echo inbound chars to UART5

		// Advance to next character in buffer (if any)
		inbuf_tail++;

		// Wrap (circular buffer)
		if (inbuf_tail == sizeof(uart1_inbuf))
			inbuf_tail = 0;
	}
}

// Set up a timeout for ACK
// Typical times seen are just under 6500 ms for a good clean network. AWS uses 29000 for a timeout on their side.
// Old code retried 3 times, each with a timeout of 15000, so overall, a 45000 ms timeout if no response at all.
// Server logs indicated that the Server itself may introduce delays on the order of 30 seconds ---
// it reported back-to-back packets (try2, and try3) arriving 30 seconds after timestamping the first try packet and ACK.
// So, clearly a longer timeout is definitely merited.
// The Server was also getting garbage that did not look like it originated from the iTracker, or was clearly not JSON.
// In those cases, the Server should close the socket immediately as no recovery is possible.
// 02-15-2023 Found and fixed typo that caused an 18 sec timeout instead of a 180 second timeout

//uart1_timeout = 90000;    // wait 90 seconds for an ACK
//uart1_timeout   = 18000UL;  // wait 180 seconds for an ACK (3 minutes)  NOTE:  THIS HAS A TYPO!  IS ONLY 18 seconds !!

#define CLOUD_ACK_TIMEOUT  90000UL   //  90,000 ms is  90 seconds
//#define CLOUD_ACK_TIMEOUT  180000UL  // 180,000 ms is 180 seconds

static uint32_t ack_timeout = CLOUD_ACK_TIMEOUT;
static uint32_t ack_timeout_count = 0;

static int wait_for_ack(void)
{
	uint32_t tickstart;
	uint32_t duration;
	uint16_t prev_inbuf1_head;
	int ack = ACK_RECEIVED;

	tickstart = HAL_GetTick();

	uart1_timeout = ack_timeout;

	prev_inbuf1_head = uart1_inbuf_head;

	while (uart1_json_string_received != RC_JSON_CLOSE)   // ISR will set when RX is complete, might already be done by the time we get here.
	{
		led_heartbeat();

		// extend timeout if any characters received -- note: can received URCs like "NO CARRIER"
        if (prev_inbuf1_head != uart1_inbuf_head)
        {
        	uart1_timeout = uart1_timeout + 7000;  // extend remaining timeout by a max of 7 seconds
        }

        prev_inbuf1_head = uart1_inbuf_head;

    	// Really need a way of detecting "NO CARRIER" so that energy is not wasted

		if (uart1_timeout == 0)  // RX timeout expired
		{
			if (sd_CellDataLogEnabled)
			{
				trace_AppendMsg("\nRX Timeout: received:\n");
				trace_AppendMsg((const char *)uart1_inbuf);
				trace_AppendMsg("\nRX End received.");
			}

			if (sd_UART_5SPY1)  SPY5_echo_inbound_chars();

			// HAIL MARY SITUATION --- ASSUME CLOUD GOT THE PACKET AND CONTINUE SENDING DATA
			// A LOG packet can contain 12 log records, so a daily drop of 96 log records would take 8 packets plus 1 for the header.
			// The header is already sent, so the total time to transmit the first H packet and the remaining 8 packets based on a new timeout:
			//  Timeout   Total Seconds    Total Minutes
			//  90        810              13.5
			//  45        450               7.5
			//  30        330               5.5
			//  25        290               4.8
			//  20        250               4.2
			//  15        210               3.5
			//  10        170               2.8

			ack_timeout_count++;  // keep track of how many times ack has timed out during this phone call
			ack_timeout = 25000;  // dynamically reduce timeout to 25 seconds for the remainder of this call to guarantee a max 5 minute call

			uart1_flush_inbuffer();

			return ACK_RECEIVED;

			//return ACK_TIMEOUT;   // original code
		}
	}

	// Both { and } were received in the correct order

	duration = HAL_GetTick() - tickstart;

	if (uart1_inbuf_head)
	{
		if (sd_CellDataLogEnabled)
		{

			char msg[75];
			sprintf(msg, "\nR %lu ms X ", duration);
			trace_AppendMsg(msg);
			trace_AppendMsg((const char *)uart1_inbuf);
			trace_AppendMsg("\n");
		}

		if (sd_UART_5SPY1)  SPY5_echo_inbound_chars();

		ack = parse_ack();
		//parse_ack((char *) uart1_inbuf, &ack, &s2c_resend_record_number);

		uart1_flush_inbuffer();
	}

	return ack;
}

static int cell_socket_transmit(uint32_t txSize)
{
	int ack;

	if (sd_UART_5SPY1) lpuart1_tx_bytes((const uint8_t *)uart1_outbuf, txSize, 0); // echo outbound chars to UART5

	// Best to clear the RX buffer in case we are in a Hail Mary situation and the Cloud responded to our earlier TX.
	// Really need a way of detecting "NO CARRIER" so that energy is not wasted
	uart1_flush_inbuffer();

	if (sd_TestCellNoTx)
	{
		// Don't send anything over the socket, just log it to the trace file for analysis.
		trace_AppendMsg("\nTS ");
		trace_AppendMsg(uart1_outbuf);


#ifdef TEST_RESEND
		// inject different resend test cases here...
		if (test_resend_state == TEST_RESEND_ARMED)
		{
			char msg[100];
			sprintf(msg,"\nInject RESEND ack:  %d,  rec_no = %d", test_resend_ack, test_resend_rec_number);

			trace_AppendMsg(msg);
			test_resend_state = TEST_RESEND_TRIGGERED;  // only inject once
			s2c_resend_record_number = test_resend_rec_number;
			return test_resend_ack;
		}
		return ACK_RECEIVED;
#else
		return ACK_RECEIVED;
#endif
	}

	uart1_tx(txSize);

	ack = wait_for_ack();

	return ack;
}

// ASSERT:  Echo is off
// ASSERT:  Verbose codes are returned <CR><LF>[RESULT CODE]<CR><LF>

static int wait_for_response(uint32_t Timeout_ms)
{

	uart1_timeout = Timeout_ms;  // Start the ISR-based countdown timer

	while (cell_response_code_received == RC_NONE)
	{
		led_heartbeat();

		if (uart1_timeout == 0)  // timeout expired
		{
			cell_response_code_received = RC_TIMEOUT;
		}

		// Provide countdown status message for App once per second -- but since this is blocking, no way to provide info...

	}
	return cell_response_code_received;
}



static int AT(const char * Cmd, uint32_t Timeout_ms)
{
	int result;
	uint32_t tickstart;  // When the command TX was started
	uint32_t duration;   // Duration of the last command (time from TX to RX complete)

	tickstart = HAL_GetTick();

	if (sd_CellCmdLogEnabled)
	{
		trace_AppendMsg("\n=====\n");  // to help visually isolate each command/response pair
		trace_AppendMsg(Cmd);
	}

	// Prepare RX buffer for command response parsing
	uart1_flush_inbuffer();

	cell_send_string(Cmd);

	result = wait_for_response(Timeout_ms);  // The modem must respond with a result code, or timeout.

	duration = HAL_GetTick() - tickstart;

	if (sd_CellCmdLogEnabled)
	{
		char duration_msg[35];

		sprintf(duration_msg,"\nR%d %luX ", result, duration);

		trace_AppendMsg(duration_msg);
		trace_AppendMsg((const char *)uart1_inbuf);  // This technique requires inbuf1 to always be null-terminated, not good for sockets?
	}

	if (sd_UART_5SPY1)  SPY5_echo_inbound_chars();

	HAL_led_delay(21);  // telit: from datasheet, page 20.  Must wait 20 ms after each cmd/response cycle

	return result;

}

// The syntax for an XBEE AT command is:
//    AT prefix + ASCII command + Space (optional) + Parameter (optional, HEX) + Carriage Return
// Multiple commands can be separated by a comma, example:
//     ATNIMy XBee,AC<cr>
// Note the second command does not need the AT prefix.
//
// The xbee will respond with  "OK\r" or "ERROR\r"
//
// Consequently, the AT_OK function will work equally well for telit or xbee modules.

int AT_OK(const char *Cmd, uint32_t Timeout_ms)
{
  if (AT(Cmd, Timeout_ms) == RC_OK) return 1;
  return 0;
}

static int AT_CONNECT(const char *Cmd, uint32_t Timeout_ms)
{
  int response_code;
  response_code = AT(Cmd, Timeout_ms);
  if (response_code == RC_CONNECT)
  {
		uart1_flush_inbuffer();  // to prevent dual logging of incoming text
  }
  return response_code;
}

#if 0
static int AT_KTCPCFG(const char *Cmd, uint32_t Timeout_ms)
{
  int response_code;
  response_code = AT(Cmd, Timeout_ms);
  if (response_code == RC_KTCPCFG)
  {
		uart1_flush_inbuffer();  // to prevent dual logging of incoming text
  }
  return response_code;
}
#endif


//if (!AT_OK("+++", BREAK_TIMEOUT)) return 0;  //
//if (!AT_CONNECT(xxx, XXX_TIMOUT)) return 0;

static int is_utc_valid(void)
{
	//                                              1         2         3
	//                                   0 123456789012345678901234567890
	// Verify the utc date time string  \r\n#CCLK: "20/06/15,20:32:05-24,1"\r\n\r\n
	if (uart1_inbuf[0] != '\r') return 0;
	if (uart1_inbuf[1] != '\n') return 0;
	if (uart1_inbuf[2] != '#') return 0;
	if (uart1_inbuf[3] != 'C') return 0;
	if (uart1_inbuf[4] != 'C') return 0;
	if (uart1_inbuf[5] != 'L') return 0;
	if (uart1_inbuf[6] != 'K') return 0;
	if (uart1_inbuf[7] != ':') return 0;
	if (uart1_inbuf[8] != ' ') return 0;
	if (uart1_inbuf[9] != '\"') return 0;
	if (!isdigit((int)uart1_inbuf[10])) return 0;
	if (!isdigit((int)uart1_inbuf[11])) return 0;

#if 0
	static int induce_failure_once = 1;
	if (induce_failure_once)
	{
		induce_failure_once = 0;
		return 0;
	}
#endif

	// Verify year is between 2x and 3x
	// to prevent situations where the cell modem does not actually
	// have a good copy of network time and it is using some sort
	// of default value, for example, the following was returned
	//   #CCLK: "80/01/06,00:00:08-24,1"
	if ((uart1_inbuf[10] != '2') && (uart1_inbuf[10] != '3')) return 0;

	if (uart1_inbuf[12] != '/') return 0;
	if (!isdigit((int)uart1_inbuf[13])) return 0;
	if (!isdigit((int)uart1_inbuf[14])) return 0;
	if (uart1_inbuf[15] != '/') return 0;
	if (!isdigit((int)uart1_inbuf[16])) return 0;
	if (!isdigit((int)uart1_inbuf[17])) return 0;
	if (uart1_inbuf[18] != ',') return 0;
	if (!isdigit((int)uart1_inbuf[19])) return 0;
	if (!isdigit((int)uart1_inbuf[20])) return 0;
	if (uart1_inbuf[21] != ':') return 0;
	if (!isdigit((int)uart1_inbuf[22])) return 0;
	if (!isdigit((int)uart1_inbuf[23])) return 0;
	if (uart1_inbuf[24] != ':') return 0;
	if (!isdigit((int)uart1_inbuf[25])) return 0;
	if (!isdigit((int)uart1_inbuf[26])) return 0;
	if ((uart1_inbuf[27] != '-') && (uart1_inbuf[27] != '+')) return 0;
	if (!isdigit((int)uart1_inbuf[28])) return 0;
	if (!isdigit((int)uart1_inbuf[29])) return 0;

	return 1;
}

static void telit_parse_utc_and_set_rtc(int Start_UTC_Timer)
{
	int i;
	int j;
	char utc_date_time_str[28]; // The format is:  yy/mm/dd,hh:mm:ss-zz
	char parse_date_time_str[28]; // The format is: mm/dd/YYYY hh:mm:ss
	char parse_timezone_str[6];  // The format is: -zz

	// Extract the utc date time string  \r\n#CCLK: "20/06/15,20:32:05-24,1"\r\n\r\n
	j = 10;
	for (i=0; i < 20; i++)
	{
		utc_date_time_str[i] = uart1_inbuf[j];
		j++;
	}

	// Convert the string into the form "mm/dd/20YY hh:mm:ss"
	parse_date_time_str[ 0] = utc_date_time_str[3];
	parse_date_time_str[ 1] = utc_date_time_str[4];
	parse_date_time_str[ 2] = '/';
	parse_date_time_str[ 3] = utc_date_time_str[6];
	parse_date_time_str[ 4] = utc_date_time_str[7];
	parse_date_time_str[ 5] = '/';
	parse_date_time_str[ 6] = '2';
	parse_date_time_str[ 7] = '0';
	parse_date_time_str[ 8] = utc_date_time_str[0];
	parse_date_time_str[ 9] = utc_date_time_str[1];
	parse_date_time_str[10] = ' ';
	parse_date_time_str[11] = utc_date_time_str[9];
	parse_date_time_str[12] = utc_date_time_str[10];
	parse_date_time_str[13] = ':';
	parse_date_time_str[14] = utc_date_time_str[12];
	parse_date_time_str[15] = utc_date_time_str[13];
	parse_date_time_str[16] = ':';
	parse_date_time_str[17] = utc_date_time_str[15];
	parse_date_time_str[18] = utc_date_time_str[16];
	parse_date_time_str[19] = 0;  // null-terminate

	// The format is: -zz  where - is plus or minus
	parse_timezone_str[0] = utc_date_time_str[17];
	parse_timezone_str[1] = utc_date_time_str[18];
	parse_timezone_str[2] = utc_date_time_str[19];
	parse_timezone_str[3] = 0;  // null-terminate

	UTC_timezone_offset_hh = atoi(parse_timezone_str);


	// Parse the UTC
	struct tm tm_st;
	parse_datetime(parse_date_time_str, &tm_st);

	// start the UTC timer at power on so it is available to the WiFi UI when enabling/disabling cell
	if (Start_UTC_Timer == UTC_START_TIMER)
	{
	  UTC_countdown_timer_ms = 1000;
	  UTC_time_s = convertDateToTime(&tm_st);
	}

	// If cell is enabled, sync the RTC to UTC - note the UTC value was validated before this function is called.
	if (IsCellEnabled()) rtc_sync_time(&tm_st);

}

static int telit_sync_to_network_UTC_time(int Start_UTC_Timer )
{

    // Set the cell modem clock for UTC mode      12_03_2022 THIS MAY NOT MEET CURRENT CLOUD SPECS TO USE LOCAL TIME
	if (!AT_OK("AT#CCLKMODE=1\r", CCLK_TIMEOUT)) return 0;

	// Get the current network time in UTC
	if (!AT_OK("AT#CCLK?\r", CCLK_TIMEOUT)) return 0;

	if (is_utc_valid())
	{
		telit_parse_utc_and_set_rtc(Start_UTC_Timer);
	}
	else
	{
		return 0;
	}

	return 1;
}

int get_imei(void)
{
	int  p;

	// check to see if the imei was already obtained
	if (s2c_imei[0] != 0)  return 1;

	if (!AT_OK("AT+CGSN\r", CGSN_TIMEOUT)) return 0;

	p = IndexOf((const char *)uart1_inbuf, '\n'); // AT+CGSN\r\r\n353535099996462\r\n\r\nOK\r\n     or+CME_ERROR: <err>
	if (p != -1)
	{
		for (int i = 0; i < 15; i++)
		{
			s2c_imei[i] = uart1_inbuf[p + 1 + i];
		} //353535099996462

		s2c_imei[15] = '\0'; // Every String in C is terminated with a NULL

		return 1;
	}
	else
	{
		// No IMEI found, zero out the return buffer
		for (int i = 0; i < 15; i++)
		{
			s2c_imei[i] = 0;
		}
	}
	return 0;
}



static int set_CGDCONT(int cid, const char * APN)
{
	char apnbuff[64];

    if (state_CGDCONT) return 1;  // already done

	sprintf(apnbuff, "AT+CGDCONT=%d,\"IP\",\"%s\"\r", cid, APN);
	//sprintf(apnbuff, "AT+CGDCONT=%d,\"IPV4V6\",\"%s\"\r", cid, APN);

	// this can fail immediately if the ctx is wrong --- maybe log a hint message to try a different ctx ?

	if (AT_OK(apnbuff, CGDCONT_TIMEOUT))
	{
		state_CGDCONT = 1;  // successfully configured the context for this power-on cycle
		return 1;
	}
	return 0;
}

static int laird_set_KCNXCFG(int cid, const char * APN)
{

	char apnbuff[64];

    if (state_KCNXCFG) return 1;  // already done

	//AT+KCNXCFG=1,"GPRS","hologram"     // 1 = cnx_cnf, see CGDCONT

	sprintf(apnbuff, "AT+KCNXCFG=%d,\"GPRS\",\"%s\"\r", cid, APN);

	// this can fail immediately if the ctx is wrong --- maybe log a hint message to try a different ctx ?

	if (AT_OK(apnbuff, CGDCONT_TIMEOUT))
	{
		state_KCNXCFG = 1;  // successfully configured the context for this power-on cycle
		return 1;
	}
	return 0;
}

#if 0 // from Slims code
int IsRegisterd()
{
	char cmdr[128];
	AT("AT+CREG?\r", cmdr);
	return getStatus(cmdr);
}

int IsContext()
{
	char cmdr[64];
	int output = 0;
	AT("AT+CGPADDR=1\r", cmdr);
	AT("AT#SCFG=1,1,300,90,600,50\r", cmdr);
	AT("AT#SGACT?\r", cmdr);
	if (getStatus(cmdr) == 1)
	{
		output = 1;
	}
	else
	{
		AT("AT#SGACT=1,1\r", cmdr);

	}
	return output;
}

int IsConnected(void)
{
	int try_ctr = 0;
	int IsRegistered = 0;
	int IsConnected = 0;
	int MaxTries = 10;
	while (try_ctr < MaxTries)	// block and wait for registration to be finished
	{

		if (IsRegistered == 0)
		{
			if (IsRegisterd() == 1)
			{
				IsRegistered = 1;
			}
		} // Check Registration to Network
		if (IsRegistered == 1)
		{
			if (IsContext() == 1)
			{
				IsConnected = 1;
				break;
			}
		} // Check Context Activation
		try_ctr += 1;
	}
	return IsConnected;
}
#endif




// ===========================================================================================
// Original code tried 15 times before giving up.  Each attempt took about 5 seconds (3 plus the response time of the cmd) so total time could vary from 45 seconds up to 75 seconds.
// Field units often did not register on the network in this amount of time.
// However, visiting the site and manually testing the cell from the App usually worked.
// At power on and using the UI App, the cell has a lot longer than 45 seconds to get registered.
// However, when waking from sleep, registration is crucial and a longer time must be allotted.
//
#if 0
#define S2C_FROM_SLEEP 0   // This tries a fixed amount of time from cell modem powerup, say seven minutes.
#define S2C_FROM_APP   1   // This tries the CEREG command a few times but gives up in a matter of seconds so the UI is still responsive.  In the background, the cell modem keeps trying to register.

static int sc2_cereg_wait_strategy = S2C_FROM_SLEEP;


int wait_for_registration(void)
{

	int try_ctr;
	int max_tries;
	uint32_t delay_ms;
	//uint32_t timeout_ms;

    if (state_CEREG) return 1;  // already registered on network for this power cycle

    // For New Main Loop, always use a short time strategy
    //sc2_cereg_wait_strategy = S2C_FROM_APP;

TryAgain:

    try_ctr = 0;

    if (sc2_cereg_wait_strategy == S2C_FROM_APP)
    {
    	max_tries = 3;
    	//delay_ms = 3000;  // zg - was 1000, then 2000.  Chose 3000 after testing on modem types 3,4,5. Type 3 is the worst.
    	delay_ms = 3000;  // zg - attempt to improve LARID connection rate by not pestering it as often
    }
    else
    {
    	max_tries = 20;  // 20 tries at 3 sec each is 1 minute
    	delay_ms = 3000; // zg - attempt to improve LARID connection rate by not pestering it as often
    }

    // block and wait for registration to be finished, each try is about 5 seconds, so this loop times out depending on max_tries

	while (try_ctr < max_tries)
	{
		led_heartbeat();

		if (!AT_OK("AT+CEREG?\r", CEREG_TIMEOUT))
		{
			update_error("CEREG error");
			return 0;   // if the modem does not respond at all in 20 seconds, an error occurred  IS THAT TRUE?  Sometimes the Laird is not responsive
		}

		if (getStatus((const char *)uart1_inbuf) == 1)
		{
			if (sd_CellCmdLogEnabled)
			{
    			char msg[80];
    			int seconds;
    			seconds = s2c_get_power_on_duration_ms() / 1000;
    			sprintf(msg,"Registered on network. Cell modem on for %d s.\n", seconds);
    			trace_AppendMsg(msg);

    			if (s2c_CellModemFound == S2C_LAIRD_MODEM) laird_report_radio_info();
			}

			state_CEREG = 1;

			return 1;
		}

		HAL_led_delay(delay_ms);
		try_ctr += 1;
	}

    // Original code tried 15 times before giving up.
    // Each attempt took about 5 seconds (3 plus the response time of the cmd) so total time could vary from 45 seconds up to 75 seconds.
	// Field units waking from sleep often did not register on the network in the original 45 to 75 seconds.
	// However, visiting the site and manually testing the cell using the phone App usually worked.
	// At power on and using the UI App, the cell has a lot longer than 45 seconds to get registered.
	// When calling in from Sleep, allow cell modem to be on for a maximum of MAX_CELL_ON_TIME_MS to complete the registration process.
	// Note that this will not normally extend the time the unit takes to go to sleep after wake up
	// because it is based on when the cell phone is turned on, not on when the CEREG confirmation
	// process starts.  The only exception is when told to go to sleep right away at power-on.
	// In that case, an (unsuccessful) power-on-ping may consume up to MAX_CELL_ON_TIME_MS before giving up.
    // A successful power-on-ping will use only the amount of time needed to register.
    //
    // Note that logging intervals that are faster than MAX_CELL_ON_TIME_MS will cause the unit to wake up more frequently
    // than a phone call can be completed and that may cause data points to be missed because the target time was passed
    // waiting for registration.

    #define MAX_CELL_ON_TIME_MS   300000UL   // 5 * 60 * 1000  Five Minutes   on matt's units some took 4 attempts (over 20 minutes of cumulative, pulsed on time) to connect
    //#define MAX_CELL_ON_TIME_MS   420000UL   // 7 * 60 * 1000  Seven Minutes
    //#define MAX_CELL_ON_TIME_MS   600000UL   // 10 * 60 * 1000  Ten Minutes    have not tried this yet

    if (sc2_cereg_wait_strategy == S2C_FROM_SLEEP)
    {
    	if (s2c_get_power_on_duration_ms() < MAX_CELL_ON_TIME_MS)  // Check if the cell modem has been powered on for the desired maximum on time, if not, retry.
    	{
    		if (sd_CellCmdLogEnabled)
    		{
    			char msg[80];
    			int seconds;
    			seconds = s2c_get_power_on_duration_ms() / 1000;
    			sprintf(msg,"CEREG try again. Cell modem on for %d s.\n", seconds);
    			trace_AppendMsg(msg);
    		}

    		goto TryAgain;
    	}
    }

    // Report a diagnostic error message to App and SD card
    {
		char msg[50];
		int seconds;
		seconds = s2c_get_power_on_duration_ms() / 1000;
		// Only 29 characters available in message to App UI
		sprintf(msg,"CEREG timed out in %d s", seconds);
		if (sd_CellCmdLogEnabled)
		{
			trace_AppendMsg(msg);
			trace_AppendMsg("\n");
		}

		update_error(msg);

    }



	return 0;
}

#else


int wait_for_registration(void)
{
	if (state_CEREG) return 1;  // already registered on network for this power cycle

	last_attempted_registration_time = rtc_get_time();
	last_successful_registration_time = 0;

	if (!AT_OK("AT+CEREG?\r", CEREG_TIMEOUT))
	{
		update_error("CEREG error");
		return 0;   // if the modem does not respond at all in 20 seconds, an error occurred  IS THAT TRUE?  Sometimes the Laird is not responsive
	}

	if (getStatus((const char *)uart1_inbuf) == 1)
	{
		last_successful_registration_time = rtc_get_time();
		last_failed_registration_time = 0;

		if (sd_CellCmdLogEnabled)
		{
			char msg[80];
			int seconds;
			seconds = s2c_get_power_on_duration_sec();
			sprintf(msg,"Registered on network. Cell modem on for %d s.\n", seconds);
			trace_AppendMsg(msg);
			//sprintf(msg,"Registered in %d s", seconds);  // For App user, maybe: Connected to tower in %d s
			//update_status(msg);

			if (s2c_CellModemFound == S2C_LAIRD_MODEM) laird_report_radio_info();
		}

		state_CEREG = 1;

		return 1;
	}

	// Report a diagnostic error message to App and SD card
	{
		char msg[50];
		int seconds;
		seconds = s2c_get_power_on_duration_sec();

		// Only 29 characters available in message to App UI
		sprintf(msg,"CEREG timed out in %d s", seconds);   // TODO need to update retry status at a higher level? So App user can see progress ?

		update_error(msg);

	}

	return 0;
}
#endif

static int telit_configure_context(int context)
{

	int try_ctr = 0;
	int MaxTries = 10;   // zg - original setting was 10
	char cmd[64];

    if (state_SCFG) return 1;  // already done

	while (try_ctr < MaxTries)	// block and wait for registration to be finished
	{
		led_heartbeat();

		// configure the socket context
		sprintf(cmd, "AT#SCFG=1,%d,300,90,600,50\r", context);   // This specifies a 600 second timeout to establish a socket connection!
		if (AT_OK(cmd, SCFG_TIMEOUT))
		{
			state_SCFG = 1;
			if (sd_CellCmdLogEnabled)
			{
				trace_AppendMsg("Context configured.\n");
			}
			//HAL_led_delay(1000);
			return 1;  // Configured
		}

		HAL_led_delay(2000);   // zg - was 1000.

		try_ctr += 1;
	}
	return 0;
}



int configure_context(int context)
{
	int result;
	switch (s2c_CellModemFound)
	{
	  default:
	  case S2C_NO_MODEM:    result = 0; break;
	  case S2C_TELIT_MODEM: result = telit_configure_context(context); break;
	  case S2C_XBEE3_MODEM: result = 0; break;
	  case S2C_LAIRD_MODEM: result = 1; break;
	}

	return result;
}

static int telit_check_activation(int context)
{
	char cmd[32];

	if (state_SGACT_QUERY) return 1;

	AT_OK("AT#SGACT?\r", SGACTQUERY_TIMEOUT);  // Check Context Activation

	sprintf(cmd, "%d,1", context);  // Looking for <context>,1 in the response  \r\n#SGACT: 1,0\r\n#SGACT: 2,1\r\n#SGACT: 3,1\r\n\r\nOK\r\n
	if (strstr((const char *)uart1_inbuf, cmd))
	{
		state_SGACT_QUERY = 1;
		if (sd_CellCmdLogEnabled)
		{
			trace_AppendMsg("Context is active.\n");
		}
		return 1;  // Activated and connected
	}

	return 0;
}

static int telit_activate_context(int context)
{
	int try_ctr = 0;
	int MaxTries = 10;   // zg - original setting was 10
	char cmd[32];

    if (state_SGACT_SET) return 1;  // already done

    // check if it is already activated
    if (telit_check_activation(context))
    {
    	state_SGACT_SET = 1;
		return 1;
    }

	while (try_ctr < MaxTries)	// block and wait for registration to be finished
	{
		led_heartbeat();

	    // start context activation
		sprintf(cmd,"AT#SGACT=%d,1\r", context);   // datasheet indicates this can take up to 150 seconds
		if (AT_OK(cmd, SGACT_TIMEOUT))
		{
			state_SGACT_SET = 1;
			if (sd_CellCmdLogEnabled)
			{
				trace_AppendMsg("Context activation started.\n");
			}
			return 1;  // Configured
		}
		else
		{
			// see if it is already activated ---
		    if (telit_check_activation(context))
		    {
		    	state_SGACT_SET = 1;
				return 1;
		    }
		}

		HAL_led_delay(2000);   // zg - was 1000.

		try_ctr += 1;
	}
	return 0;
}

static int laird_activate_context(int context)
{
  return 1;
}

static int activate_context(int context)
{
	int result;
	switch (s2c_CellModemFound)
	{
	  default:
	  case S2C_NO_MODEM:    result = 0; break;
	  case S2C_TELIT_MODEM: result = telit_activate_context(context); break;
	  case S2C_XBEE3_MODEM: result = 0; break;
	  case S2C_LAIRD_MODEM: result = laird_activate_context(context); break;
	}

	return result;
}

static int telit_wait_for_activation(int context)
{

	int try_ctr = 0;
	int MaxTries = 10;   // zg - original setting was 10

	while (try_ctr < MaxTries)	// block and wait for registration to be finished
	{
		led_heartbeat();

		if (telit_check_activation(context)) return 1;

		HAL_led_delay(2000);   // zg - was 1000.

		try_ctr += 1;
	}
	return 0;
}

static int laird_wait_for_activation(int context)
{
  return 1;
}

static int wait_for_activation(int context)
{
	int result;
	switch (s2c_CellModemFound)
	{
	  default:
	  case S2C_NO_MODEM:    result = 0; break;
	  case S2C_TELIT_MODEM: result = telit_wait_for_activation(context); break;
	  case S2C_XBEE3_MODEM: result = 0; break;
	  case S2C_LAIRD_MODEM: result = laird_wait_for_activation(context); break;
	}

	return result;
}

int connect_to_network_apn_ctx(const char * APN, int cid)
{

	if (!set_CGDCONT(cid, APN))
	{
		update_error("CGDCONT error");
		return 0;  // This can take up to 20 seconds, is there any reason to re-try if failure?
	}

	if (s2c_CellModemFound == S2C_LAIRD_MODEM)
	{
		if (!laird_set_KCNXCFG(cid, APN))
		{
			update_error("KCNXCFG error");
			return 0;
		}
	}

	if (!wait_for_registration())
	{
		// diagnostic message is updated by lower-level routines, do not override here
		return 0;
	}

	if (!configure_context(cid))   // Only for Telit
	{
		update_error("NETWORK error 1");
		return 0;
	}

	if (!activate_context(cid))  // Only for Telit
	{
		update_error("NETWORK error 2");
		return 0;
	}

	if (!wait_for_activation(cid))  // Only for Telit
	{
		update_error("NETWORK error 3");
		return 0;
	}

	return 1;
}


//==========================================================================================



#define CARRIER_FIRMWARE_ERROR    9
#define CARRIER_FIRMWARE_ATT      1  // must match ctx = 1 = ATT
#define CARRIER_FIRMWARE_VERIZON  3  // must match ctx = 3 = VERIZON

static int telit_get_carrier_firmware(void)
{
	//if (sd_CellCmdLogEnabled)
	//{
	//	AT_OK("AT#FWSWITCH=?\r", FWSWITCH_TIMEOUT);  // for debugging only, report the range of values
	//}

#if 0
	static int induce_error = 1;
	if (induce_error)
	{
		induce_error = 0;
		return CARRIER_FIRMWARE_ERROR;
	}
#endif

	if (!AT_OK("AT#FWSWITCH?\r", FWSWITCH_TIMEOUT)) return CARRIER_FIRMWARE_ERROR;



	     if(strstr((const char *)uart1_inbuf, "#FWSWITCH: 0"))  {	return CARRIER_FIRMWARE_ATT;	}
	else if(strstr((const char *)uart1_inbuf, "#FWSWITCH: 1"))  {	return CARRIER_FIRMWARE_VERIZON;	}

  return CARRIER_FIRMWARE_ERROR;
}




static int telit_configure_cell_modem_firmware_for_network(int DesiredNetwork)
{
	char cmd[20];
	switch (telit_get_carrier_firmware())
	{
		case CARRIER_FIRMWARE_ERROR:
			if (sd_CellCmdLogEnabled)
			{
				trace_AppendMsg("CARRIER_FIRMWARE_ERROR cannot FWSWITCH\n");
			}
			return 0;
		case CARRIER_FIRMWARE_VERIZON:
			if (DesiredNetwork == CARRIER_FIRMWARE_VERIZON) return 1; // firmware is configured as desired
			strcpy(cmd, "AT#FWSWITCH=0,1\r");  // switch from Verizon to ATT
			break;
		case CARRIER_FIRMWARE_ATT:
			if (DesiredNetwork == CARRIER_FIRMWARE_ATT) return 1; // firmware is configured as desired
			strcpy(cmd, "AT#FWSWITCH=1,1\r");  // switch from ATT to Verizon
			break;
	}
	return AT_OK(cmd, FWSWITCH_TIMEOUT);  // this performs a system reboot...
}

// These hw_xyz variables are set at power-on
static int  hw_ctx;
static char hw_apn[CELL_APN_MAX];


// LE910-NAG  North America AT&T
// LE910-NVG  North America Verizon
// LE910-SVG  North America Verizon
// LE910-SV1  North America Verizon
// ME910C1-NA North America
// LE910-EUG  Europe
// LE910-SKG  Korea

#define HW_CELL_NONE   0
#define HW_TELIT_LAT3  1
#define HW_TELIT_LAT1  2
#define HW_TELIT_LVW3  3
#define HW_TELIT_LVW2  4
#define HW_TELIT_MNA1  5
#define HW_LAIRD       6
#define HW_XBEE3       7

static int  hw_modem = HW_CELL_NONE;

static int get_apn_and_ctx(char * apn, int * ctx)
{
	// ASSERT:  the low-level hw_xxx variables were assigned at power-on

	// check for an override stored in flash, or recently configured
	if (CellConfig.ctx && strlen(CellConfig.apn))
	{
		*ctx = CellConfig.ctx;
		strcpy(apn, CellConfig.apn);
		if (sd_CellCmdLogEnabled)
		{
			char msg[100];
			sprintf(msg, "Using override: ctx=%lu, apn=%s\n", CellConfig.ctx, CellConfig.apn);
			trace_AppendMsg(msg);
		}
	}
	else
	{
		// Use values determined at power-on
		*ctx = hw_ctx;
		strcpy(apn, hw_apn);

		if (sd_CellCmdLogEnabled)
		{
			char msg[100];
			sprintf(msg, "Using hardware: ctx=%d, apn=%s.  Cfg ctx=%lu, apn=%s\n", hw_ctx, hw_apn, CellConfig.ctx, CellConfig.apn);
			trace_AppendMsg(msg);
		}
	}


	// check that modem has correct firmware (only for modem 5)
	if ((hw_modem == HW_TELIT_MNA1) && (telit_get_carrier_firmware() != *ctx))
	{
		if (sd_CellCmdLogEnabled)
		{
			trace_AppendMsg("Cell modem firmware does not match requested carrier.\n");
			trace_AppendMsg("Switching firmware.  Power cycle required.\n");
		}

		// This reboots the cell module firmware, so a power-cycle is required
		telit_configure_cell_modem_firmware_for_network(*ctx);
		return 0;
	}


	return 1;
}

int echo_off(void)
{

	int retry_count = 0;

	// echo_off is the first command sent after power-on.
	// A delay is needed after power-on, otherwise the modem does not respond to commands.
	// Just in case the delay is not enough by the time the code gets here,
	// retry a few times to give the modem time to wake up.

	try_again:

    if (AT_OK("ATE0\r", ATE_TIMEOUT)) return 1;

	if (retry_count < 3)
	{
		retry_count++;
		HAL_led_delay(5000);
		goto try_again;
	}

	// echo_off did not succeed after 3 attempts, report failure.
	return 0;
}





static int telit_open_socket_to_server(void)
{
	char cmd[64];  // to leave room for a long URL
	int response_code;
	//int retry_count = 0;

	// SOCKET DIAL (CONNECTION TO S2C SERVER)
	// zg - This establishes a socket connection --- the correct response code is "CONNECT"
	// SD=connId,txProt,rPort,IPaddr
	//connId = 1..max
	//txProt = 0 = TCP; 1 = UDP

	//sprintf(cmd,"AT#SD=1,0,9009,\"35.196.120.101\"\r");

	sprintf(cmd,"AT#SD=1,0,%s,\"%s\"\r", CellConfig.port, CellConfig.ip);   // datasheet indicates this can take 140 seconds

//	try_again:

	response_code = AT_CONNECT(cmd, SD_TIMEOUT);

	// Earlier modems seem to report an ERROR right away instead of waiting long enough to connect.
	// Adding 28 retries to extend to the full 140 seconds did not work.
	// For example, the MNA-1 reported a 22 second connect time, but the other two reported an ERROR right away.  Same SIM, same network..
	// 140  / 5 = 28
	// 140 / 3 = 47
	// Leave a short retry loop of about 3
#if 0
	if (response_code == RC_ERROR)
	{
		if (retry_count < 3)
		{
			retry_count++;
			HAL_led_delay(5000);
			goto try_again;
		}
	}
#endif

	if (response_code == RC_CONNECT) return 1;  // socket was opened successfully

	return 0;  // an error occurred
}



static int laird_configure_tcp(void)
{
	char cmd[64];  // to leave room for a long URL
	int session_id = 0;
	int retry_count = 0;

	char apn[28];
	int  ctx;

	if (!get_apn_and_ctx(apn, &ctx))   // this is fast and does not access the modem except for Telit firmware selection (very rare)
	{
		update_error("NETWORK error apn ctx");
		return 0;
	}

    //sprintf(cmd,"AT+KTCPCFG=1,0,"<ip>",<port>\r");   // Example:  AT+KTCPCFG=1,0,"18.236.236.185",2222
	//Response is: \r\n+KTCPCFG: <session_id>\r\n

	sprintf(cmd,"AT+KTCPCFG=%d,0,\"%s\",%s\r", ctx, CellConfig.ip, CellConfig.port);

	try_again:

	if (AT_OK(cmd, SD_TIMEOUT))
	{
		session_id = get_session_id((const char *)uart1_inbuf);
	}

	if (session_id == 0)
	{
		if (retry_count < 3)
		{
			retry_count++;
			HAL_led_delay(5000);
			goto try_again;
		}
	}

	return session_id;  // if 0 an error occurred
}

static int laird_open_socket_to_server(int session_id)
{
	char cmd[64];  // to leave room for a long URL
	int response_code;

	sprintf(cmd,"AT+KTCPSTART=%d\r", session_id);   // Example:  AT+KTCPSTART=2

	response_code = AT_CONNECT(cmd, SD_TIMEOUT);

	if (response_code == RC_CONNECT) return 1;  // socket was opened successfully

	return 0;  // an error occurred
}

int session_id;

static int config_tcp(void)
{
	if (state_TCP_CONFIGURED) return 1;  // nothing to do, already configured

	switch (s2c_CellModemFound)
	{
	  default:
	  case S2C_NO_MODEM:    session_id = 1; break;
	  case S2C_TELIT_MODEM: session_id = 1; break;
	  case S2C_XBEE3_MODEM: session_id = 1; break;
	  case S2C_LAIRD_MODEM: session_id = laird_configure_tcp(); break;
	}

	if (session_id == 0)
	{
		update_error("TCP config error");
		return 0;
	}

	state_TCP_CONFIGURED = 1;

	return 1;
}

static int open_socket_to_server(void)
{
	int result;

	// Configure TCP if needed
	result = config_tcp();
	if (!result) return 0;

	// open socket to server
	switch (s2c_CellModemFound)
	{
	  default:
	  case S2C_NO_MODEM:    result = 0; break;
	  case S2C_TELIT_MODEM: result = telit_open_socket_to_server(); break;
	  case S2C_XBEE3_MODEM: result = 0; break;
	  case S2C_LAIRD_MODEM: result = laird_open_socket_to_server(session_id); break;
	}

	return result;
}





#if 0
static int check_for_signal(void)
{
	// Ok for Laird
	return AT_OK("AT+CSQ\r", CSQ_TIMEOUT);
}
#endif

#if 0
static int read_sim_spn(void)
{
	// Not for Laird
	return AT_OK("AT#SPN\r", CSQ_TIMEOUT);
}



#endif

#if 0
static int get_iccid(void)
{
	// Ok for Laird, ok for Telit?
	// where to save this info?  How to report via JSON?
	return AT_OK("AT+CCID\r", CSQ_TIMEOUT);
}
#endif

#if 0  // Just for information - may turn this on for debugging or logging
static int check_for_SIM_card(void)
{
	//Not meaningful?  this is the enter pin command...
	return AT_OK("AT+CPIN?\r", CPIN_TIMEOUT);
}
#endif

#if 0
static int get_imsi(void)
{
	// Returns IMSI (International Mobile Subscriber Identity) of the mobile terminal.
	return AT_OK("AT#CIMI\r", CIMI_TIMEOUT);
}
#endif


#if 0 // zg - possible other useful commands
AT#PSNT?     returns info on Packet Service Network Type
AT+CTZU      enables Automatic Time Zone Update
AT#NITZ      Network Identity and Time Zone
AT+CSMS      Select Message Service
AT+CPMS      Preferred Message Storage
#endif

#if 0  // This is not an AT command...
static int send_break(void)
{
	return AT_OK("+++", BREAK_TIMEOUT);  // escape does NOT respond with OK
}
#endif

#if 0
static int echo_off_and_get_imei(void)
{
	if (!echo_off()) return 0;

	if (!get_imei()) return 0;

	return 1;
}
#endif


static int connect_to_network(void)  // only called for Laird and Telit
{
	char apn[28];
	int  ctx;

	// must call echo_off_and_get_imei() before calling this function

	if (!get_apn_and_ctx(apn, &ctx))   // this is fast and does not access the modem except for Telit firmware selection (very rare)
	{
		update_error("NETWORK error apn ctx");
		return 0;
	}

	if (!connect_to_network_apn_ctx(apn, ctx))
	{
		// diagnostic message is updated by lower-level routines, do not override here
		return 0;
	}

	return 1;
}



#if 0
static void get_MAC_sn_str(char *MAC_sn_str)
{
	/*
	 * "mmMMmmMMmmMMsssSSSsss"
	 */

	int i;
	int j;

	// Copy the MAC but skip colons  mm:MM:mm:MM:mm:MM  18 with null term
	j = 0;
	for (i=0; i < 19; i++ )
	{
		char ch;
		ch = MainConfig.MAC[i];
		if (ch != ':')
		{
			MAC_sn_str[j] = ch;
			j++;
		}
	}
    // append the serial number (if any)
	strcat(MAC_sn_str, MainConfig.serial_number);

}
#endif


static void get_MAC_str(char * MAC_str)
{
	int i;
	int j = 0;
	for (i=0;i < 18; i++)  // Copy the MAC but skip colons  mm:MM:mm:MM:mm:MM  18 with null term
	{
		if (MainConfig.MAC[i] != ':')
		{
			MAC_str[j] = MainConfig.MAC[i];
			j++;
		}
	}

	// Or - Use the one with colons
	//strcpy(MAC_str,MainConfig.MAC);

#if 0   // zg - masquerade as Max's cell unit
	strcpy(MAC_str,"4C55CC1752F1");
#endif

}

#if 0
static void get_sn_str(char *serial_no_str)
{
#if 1
	strcpy(serial_no_str, MainConfig.serial_number);
#else
	// check if serial number is valid otherwise send the IMEI instead. this is to fix the issue the serial number being reset.
	if (find(MainConfig.serial_number,"16") || find(MainConfig.serial_number,"17"))
	{
		strcpy(serial_no_str, MainConfig.serial_number);
	}
	else if (find(s2c_imei,"35"))
	{
		strcpy(serial_no_str, s2c_imei);
	}
	else
	{
		strcpy(serial_no_str, "NO SERIAL");
	}
#endif
}
#endif

//          1         2         3         4         5
// 12345678901234567890123456789012345678901234567890
// mm/dd/yyyy hh:mm:ss PING sent
// mm/dd/yyyy hh:mm:ss DATA sent
// mm/dd/yyyy hh:mm:ss ALERT 1 sent
// mm/dd/yyyy hh:mm:ss ALERT 2 sent
// mm/dd/yyyy hh:mm:ss ALERT cleared

static void update_status(char *msg)
{
	char time_str[TIME_STR_SIZE];
	char short_msg[30];

	// Cell status max is 50 (CELL_STATUS_MAX)
	// Time stamp is 20
	// Msg can be up to (50-20-1) = 29
	for (int i=0; i < 29; i++)
	{
		short_msg[i] = msg[i];
		if (msg[i] == '\0') break;
	}
	short_msg[29]='\0';

	sprintf(CellConfig.status, "%s %s", get_current_time_str(time_str), short_msg);

	AppendStatusMessage(short_msg);  // don't need two timestamps in log file

	saveCellConfig = 1;

	//if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	//{
	//	trace_AppendMsg("\n");
	//	trace_AppendMsg(CellConfig.status);
	//	trace_AppendMsg("\n");
	//}
}

static void update_error(char *msg)
{
	char time_str[TIME_STR_SIZE];
	char short_msg[30];

	// Cell status max is 50 (CELL_STATUS_MAX)
	// Time stamp is 20
	// Msg can be up to (50-20-1) = 29
	for (int i=0; i < 29; i++)
	{
		short_msg[i] = msg[i];
		if (msg[i] == '\0') break;
	}
	short_msg[29]='\0';

	sprintf(CellConfig.error, "%s %s", get_current_time_str(time_str), short_msg);

	// errors automatically override status as the "status" is the only item sent via the Settings packet
	strcpy(CellConfig.status, CellConfig.error);

	saveCellConfig = 1;

	AppendStatusMessage(short_msg);  // don't need two timestamps in log file

	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		trace_AppendMsg("\n");
		trace_AppendMsg(CellConfig.error);
		trace_AppendMsg("\n");
	}


}



void s2c_prep_settings_packet(char * outbuf)
{
	/* The Settings Packet looks like this:
	{
	"0":"mmMMmmMMmmMM",         // mac address (colons optional)
	"1":"S",                    // packet type
	"a":"sssSSSsss",            // serial number
	"b":"123456789012345",      // imei number
	"c":"n.n.nnnn",             // firmware version string
	"d":"Site_ID_abcdefghijk",  // Site ID
	"e":1234567890,             // battery reset timestamp
	"f":nn,                     // battery percentage
	"g":1234567890,             // log start timestamp
	"h":458751,                 // logged record count
	"i":n,                      // bit flags
	"j":nn,                     // log interval (minutes)
	"k":nn.nn,                  // pipe id (inches)
	"l":nn.nn,                  // vertical mount (inches)
	"m":nn,                     // damping
	"n":nnnn,                   // population
	"o":nn.nn,                  // alert level 1 (inches)
	"p":nn.nn}                  // alert level 2 (inches)
	*/

	char MAC_str[20];                // with null 13
	char packet_id_str[4];           // with null 2
	char firmware_version_str[20];   // with null  8
    char battery_reset_date_str[14]; // with null 11
    char battery_percentage_str[4];  // with null  3
    char log_start_str[12];          // 10 digits
	char logged_record_count_str[8]; // nnnnnn max 6 digits
	char bitflags_str[4];            // n
	char log_interval_str[6];        // nn but allow for nnnnn when testing sleep current
	char pipe_id_str[8];             // nn.nn
	char vertical_mount_str[8];      // nn.nn
	char damping_str[4];             // nn
	char population_str[6];          // nnnn
	char alert_1_str[8];             // nn.nn
	char alert_2_str[8];             // nn.nn


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


    get_MAC_str(MAC_str);

    strcpy(packet_id_str,"S");  // Settings packet

	sprintf(firmware_version_str, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);

	sprintf(battery_reset_date_str, "%lu", MainConfig.battery_reset_datetime);

	sprintf(battery_percentage_str, "%d", ltc2943_get_battery_percentage());

	sprintf(log_start_str, "%lu", LogTrack.start_datetime);
	sprintf(logged_record_count_str, "%lu", LogTrack.log_counter);

#if 0
	// Bitflags, currently only units and first response. May be more in the future.
	if (IsFirstResponse())
	{
		if (MainConfig.units == UNITS_ENGLISH)
			sprintf(bitflags_str, "3");  // binary: 11
		else
			sprintf(bitflags_str,"2");   // binary: 10
	}
	else
	{
		if (MainConfig.units == UNITS_ENGLISH)
			sprintf(bitflags_str, "1");  // binary: 01
		else
			sprintf(bitflags_str,"0");   // binary: 00
	}
#else
	if (MainConfig.units == UNITS_ENGLISH)
		sprintf(bitflags_str, "1");  // binary: 01
	else
		sprintf(bitflags_str,"0");   // binary: 00
#endif

	sprintf(log_interval_str, "%lu", (MainConfig.log_interval / 60));
	f_to_str_2dec(pipe_id_str, MainConfig.pipe_id);
	f_to_str_2dec(vertical_mount_str, MainConfig.vertical_mount);
	sprintf(damping_str, "%d", MainConfig.damping);
	sprintf(population_str, "%lu", MainConfig.population);
	f_to_str_2dec(alert_1_str, MainConfig.AlertLevel1);
	f_to_str_2dec(alert_2_str, MainConfig.AlertLevel2);


    char * fmt =
    		"{"
    		"\"0\":\"%s\","
    		"\"1\":\"%s\","
    		"\"a\":\"%s\","
    		"\"b\":\"%s\","
    		"\"c\":\"%s\","
    		"\"d\":\"%s\","
    		"\"e\":%s,"
    		"\"f\":%s,"
    		"\"g\":%s,"
    		"\"h\":%s,"
    		"\"i\":%s,"
    		"\"j\":%s,"
    		"\"k\":%s,"
    		"\"l\":%s,"
    		"\"m\":%s,"
    		"\"n\":%s,"
    		"\"o\":%s,"
    		"\"p\":%s}";

    sprintf(outbuf, fmt,
    		/* "0" */ MAC_str,
			/* "1" */ packet_id_str,
			/* "a" */ MainConfig.serial_number,
			/* "b" */ s2c_imei,
			/* "c" */ firmware_version_str,
			/* "d" */ MainConfig.site_name,
			/* "e" */ battery_reset_date_str,
			/* "f" */ battery_percentage_str,
			/* "g" */ log_start_str,
			/* "h" */ logged_record_count_str,
			/* "i" */ bitflags_str,
			/* "j" */ log_interval_str,
			/* "k" */ pipe_id_str,
			/* "l" */ vertical_mount_str,
			/* "m" */ damping_str,
			/* "n" */ population_str,
			/* "o" */ alert_1_str,
			/* "p" */ alert_2_str);
}

static void prep_header_packet(void)
{
	/* The Header Packet looks like this:
	{
	"0":"mmMMmmMMmmMM",         // mac address (colons optional)
	"1":"H",                    // packet type
	"f":nn,                     // battery percentage
	"h":458751,                 // logged record count
	}
	*/

	char MAC_str[20];                // with null 13
	char packet_id_str[4];           // with null 2
    char battery_percentage_str[4];  // with null  3
	char logged_record_count_str[8]; // nnnnnn max 6 digits

    get_MAC_str(MAC_str);

    strcpy(packet_id_str,"H");  // Header packet

	sprintf(battery_percentage_str, "%d", ltc2943_get_battery_percentage());

	sprintf(logged_record_count_str, "%lu", LogTrack.log_counter);

    char * fmt =
    		"{"
    		"\"0\":\"%s\","
    		"\"1\":\"%s\","
    		"\"f\":%s,"
    		"\"h\":%s}";

    sprintf(uart1_outbuf, fmt,
    		/* "0" */ MAC_str,
			/* "1" */ packet_id_str,
			/* "f" */ battery_percentage_str,
			/* "h" */ logged_record_count_str);

}

#if 0
// Calculate how many log records to send in an array
// The members of the array must all share the same vertical mount value.
static int calc_array_size_vm(uint32_t StartAddr, int maxlen)
{

  log_t    log_rec;
  int      first_rec = 1;
  int      size = 0;


  log_EnumStart(StartAddr, LOG_FORWARD);

  while (log_EnumGetNextRecord(&log_rec))
  {
	  float prev_vertical_mount;

	  if (first_rec)
	  {
	     prev_vertical_mount = log_rec.vertical_mount;
	     first_rec = 0;
	  }
#if 0
	  // Try and detect old format log files, vertical_mount would be zero (or level in 4.1.4045 and 4.1.4046)
	  // both of which are smaller than distance.
	  // That is a vertical_mount greater than distance indicates a new-format log file.
	  if (log_rec.vertical_mount > log_rec.distance)
	  {
	    // Stop enumerating records at the first mismatch on vertical mount
	    if (log_rec.vertical_mount != prev_vertical_mount) break;
	  }
#else
	  // Stop enumerating records at the first mismatch on vertical mount
	  if (log_rec.vertical_mount != prev_vertical_mount) break;
#endif
	  // Set up for next record
	  prev_vertical_mount = log_rec.vertical_mount;

	  size++;

	  if (size == maxlen) break;
  }

  return size;
}
#endif


#if 0
#define FIXED_DATE_UTC  (1514764800UL)    // 2018-01-01T00:00:00
#define FIXED_DATE_UTC  0x5A497A00
// 0x5A497A00
static const uint32_t fixed_date_utc = 0x5A497A00; //  1514764800  2018-01-01 00:00:00
#endif

// Calculate how many log records to send in an array
// The members of the array must all share the same log interval value to within 30 seconds.
//  That is the distance between data points is always within (li-30) < li < (li+30)



//#define ENABLE_ARRAY_SIZE_TRACE_MSGS

static void calc_array_size_ts(uint32_t StartAddr, int maxlen, uint32_t *p_array_size, uint32_t * p_tso, uint32_t *p_log_interval_min)
{

  log_t    log_rec;
  uint32_t size=0;
  int32_t first_li = 0;  // first log interval found in data
  int32_t next_li = 0;  // next log interval found in data
  int32_t diff_li;

  uint32_t prev_datetime=0;
  uint32_t tso;

  *p_array_size = 0;
  *p_tso = 0;
  *p_log_interval_min = 0;

  tso = 0;
  log_EnumStart(StartAddr, LOG_FORWARD);

#ifdef ENABLE_ARRAY_SIZE_TRACE_MSGS
  trace_AppendMsg("\n");
#endif

  while (log_EnumGetNextRecord(&log_rec, LogTrack.next_logrec_addr))
  {
	  size++;

	  if (size==1)  // first record
	  {
		  tso = log_rec.datetime;  // use the actual timestamp, no changes

		  first_li = MainConfig.log_interval;  // by definition, reference the log interval for an array with a single value

#ifdef ENABLE_ARRAY_SIZE_TRACE_MSGS
		  // For development, print out each array element for inspection
		  {
			  char msg[80];
			  sprintf(msg,"%lu: %lu, %lu, %lu\n", size, log_rec.datetime, tso, first_li);
			  trace_AppendMsg(msg);
		  }
#endif

	  }
	  else
	  {
		  if (size==2)  // this is the second record so the first interval.  This overrides the headers log interval
		  {
			  // Note: individual data points have clock jitter due to variable times the sensor takes to get a reading.
			  // This can result in any two data points being closer than the log interval, or greater than the log interval.

			  if (log_rec.datetime > prev_datetime)
			     first_li = log_rec.datetime - prev_datetime;
			  else
				 break;  // a gap exists due to the clock going backwards

			  if (first_li > MainConfig.log_interval)
				  diff_li = first_li - MainConfig.log_interval;
			  else
				  diff_li = MainConfig.log_interval - first_li;

			  if (diff_li <= 30)
			  {
				  first_li = MainConfig.log_interval;  // just use the reference log interval for the entire set
			  }

		  }
		  if (log_rec.datetime > prev_datetime)
			  next_li = log_rec.datetime - prev_datetime;
		  else
			  break; // a gap exists due to the clock going backwards

#ifdef ENABLE_ARRAY_SIZE_TRACE_MSGS
		  // For development, print out each array element for inspection
		  {
			  char msg[80];
			  uint32_t this_tso;


			  sprintf(msg,"%lu: %lu, %lu, %lu, %lu\n", size, log_start_datetime, log_rec.datetime, this_tso, first_li);
			  trace_AppendMsg(msg);
		  }
#endif


		  // Stop enumerating records when the distance between timestamps
		  // differs from the first interval of the set by more than 30 seconds.
		  // This is intended to accommodate the clock jitter between logged data points.
		  if (first_li > next_li)
			  diff_li = first_li - next_li;
		  else
			  diff_li = next_li - first_li;

		  if (diff_li > 30)
		  {
			  break;
		  }

	  }

	  if (size == maxlen)
	  {
		  break;
	  }


	  // Set up for next record
	  prev_datetime = log_rec.datetime;
  }

  // If we run out of records, or have zero records, we get here, and size is correct.

  // compute the log interval in minutes
  first_li = first_li / 60;
  if (first_li < 1) first_li = 1;  // enforce a minimum log interval of 1 minute.

  *p_array_size = size;
  *p_tso = tso;
  *p_log_interval_min = first_li;  // only report log intervals to the nearest minute

}

static void prep_array_strings(uint32_t addr, int array_size, char * gain_array_str, char * distance_array_str, char * vertical_mount_array_str)
{
	log_t log_rec;
	int16_t gain;
	int16_t comma;

	comma = ADD_COMMA;

	gain_array_str[0] = '\0';
	distance_array_str[0] = '\0';
	vertical_mount_array_str[0] = '\0';

	for (int c = 0; c < array_size; c++)
	{
		if (c == (array_size - 1)) 	comma = NO_COMMA;

		log_readstream_ptr(&log_rec, addr);

		gain = 255 - (log_rec.gain & 0xff);

		append_to_array(gain_array_str, (float)gain, 0, comma);

		append_to_array(distance_array_str, log_rec.distance, 2, comma);

		append_to_array(vertical_mount_array_str, log_rec.vertical_mount, 2, comma);

		addr += LOG_SIZE;
		if (addr >= LOG_TOP)
		{
			addr = LOG_BOTTOM;
		}
	}
}

uint32_t s2c_prep_log_packet(char * outbuf, char packet_id, uint32_t StartingLogRecNumber, uint32_t MaxArraySize)  // max array size is 12, but can request less, e.g. 1
{
	/* The Log Data Packet looks like this
	{
	"0":"mmMMmmMMmmMM",         // mac address (colons optional)
	"1":"L",                    // packet type  L,E,F,G
    "N":[123456,1234567890,nn]  // log record number, timestamp, and optional actual log interval in min (1,5,10,15,30,60)
    "T":nn,              // temperature (F)
    "G":[nnn,...]        // Gain up to 12 array values
    "D":[nn.nn,...]      // Distance up to 12 array values - for SewerWatch, this may be nnn.nn
    }
	*/
	char MAC_str[20];                // with null 13
	char packet_id_str[4];           // with null 2


	uint32_t array_size;
    uint32_t tso;
	uint32_t log_interval_min;
	char gain_array_str[60];     // nnn,nnn       4*12 = 48 + null = 49
	char distance_array_str[80]; // nn.nn,nn.nn   6*12 = 72 + null = 73  For SewerWatch nnn.nn,
	char vertical_mount_array_str[80]; // nn.nn,nn.nn   6*12 = 72 + null = 73
	log_t first_log_rec;
	uint32_t start_addr;

	char log_rec_str[30];            // with null  20   123456,1234567890,nn(20)   actual at Kingsport: 2101,4627626
    //char vertical_mount_str[10];   // with null  6
    char temperature_str[6];         // with null  3


    get_MAC_str(MAC_str);

    sprintf(packet_id_str,"%c", packet_id);  // L,V,E,F,G


	array_size = MaxArraySize;
	if (array_size > 12) array_size = 12;

	// Calculate start address
	start_addr = log_FindRecAddr(StartingLogRecNumber);
	if (start_addr == 0) return 0; // no log records found

	calc_array_size_ts(start_addr, array_size, &array_size, &tso, &log_interval_min); // only send a series of records that have matching time intervals

	sprintf(log_rec_str, "%lu,%lu", StartingLogRecNumber, tso);

    // check if actual log interval agrees with header log interval
    if (log_interval_min != (MainConfig.log_interval/60))
    {
    	// This can happen if the user changes the interval without starting a new log.
    	// One possible idea is to add a 3rd parameter to the "N" array with the correct interval.
    	// The cloud then has to watch for this optional parameter and use it if given.
    	sprintf(log_rec_str, "%lu,%lu,%lu", StartingLogRecNumber, tso, log_interval_min);
    }


	//f_to_str_2dec(vertical_mount_str, MainConfig.vertical_mount);

	log_readstream_ptr(&first_log_rec, start_addr);
	f_to_str_0dec(temperature_str, first_log_rec.temperature);

	prep_array_strings(start_addr, array_size, gain_array_str, distance_array_str, vertical_mount_array_str);

	if (packet_id == 'V')
	{
		char * fmt =
				"{"
				"\"0\":\"%s\","
				"\"1\":\"%s\","
				"\"N\":[%s],"
				"\"T\":%s,"
				"\"G\":[%s],"
				"\"D\":[%s],"
				"\"V\":[%s]"
				"}";

		sprintf(outbuf, fmt,
				/* "0" */ MAC_str,
				/* "1" */ packet_id_str,
				/* "N" */ log_rec_str,
				/* "T" */ temperature_str,
				/* "G" */ gain_array_str,
				/* "D" */ distance_array_str,
		        /* "V" */ vertical_mount_array_str);
	}
	else
	{
		char * fmt =
				"{"
				"\"0\":\"%s\","
				"\"1\":\"%s\","
				"\"N\":[%s],"
				"\"T\":%s,"
				"\"G\":[%s],"
				"\"D\":[%s]"
				"}";

		sprintf(outbuf, fmt,
				/* "0" */ MAC_str,
				/* "1" */ packet_id_str,
				/* "N" */ log_rec_str,
				/* "T" */ temperature_str,
				/* "G" */ gain_array_str,
				/* "D" */ distance_array_str);
	}

	return array_size;  // return the number of records actually bundled into the packet

}




#if 0
static void prep_PING_text(void)  // zg - I think this is obsolete....
{

	char firmware_version_str[16];
	char serial_no_str[16];  // sn is 10 chars; imei is 16 chars; "NO SERIAL" is 10

	int dec1;
	int dec2;
	char pipeID_str[8];
	char verticalMount_str[8];
	char AlertLevel1_str[8];
	char AlertLevel2_str[8];
	float tmp_pipe_id;
	float tmp_AlertLevel1;
	float tmp_AlertLevel2;
	float tmp_vertical_mount;

	tmp_pipe_id     = MainConfig.pipe_id;
	tmp_AlertLevel1 = MainConfig.AlertLevel1;
	tmp_AlertLevel2 = MainConfig.AlertLevel2;
	tmp_vertical_mount = MainConfig.vertical_mount;

	if (MainConfig.units == UNITS_METRIC)
	{
		tmp_pipe_id        *= 25.4;
		tmp_AlertLevel1    *= 25.4;
		tmp_AlertLevel2    *= 25.4;
		tmp_vertical_mount *= 25.4;
	}

	sprintf(firmware_version_str, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);

	convfloatstr_2dec(tmp_pipe_id, &dec1, &dec2);
	sprintf(pipeID_str, "%d.%02d", dec1, dec2);

	convfloatstr_2dec(tmp_AlertLevel1, &dec1, &dec2);
	sprintf(AlertLevel1_str, "%d.%02d", dec1, dec2);

	convfloatstr_2dec(tmp_AlertLevel2, &dec1, &dec2);
	sprintf(AlertLevel2_str, "%d.%02d", dec1, dec2);

	convfloatstr_2dec(tmp_vertical_mount, &dec1, &dec2);
	sprintf(verticalMount_str, "%d.%02d", dec1, dec2);


	get_sn_str(serial_no_str);


	// Format the PING JSON response
	sprintf(uart1_outbuf, "{\"0\":\"%s\",\"1\":\"PING\",\"10\":\"%s\",\"11\":\"%s\",\"12\":\"%ld\",\"13\":\"%s\",\"8\":\"%s\",\"15\":\"%ld\",\"16\":\"%s\",\"17\":\"%s\"}",
		MainConfig.MAC,                  //"0"
		firmware_version_str,            //"10"
		serial_no_str,                   //"11"
		MainConfig.log_interval,         //"12"
		pipeID_str,                      //"13"
		verticalMount_str,               //"8"    // zg - propose using 14 as site_ID
		MainConfig.population,           //"15"
		AlertLevel1_str,                 //"16"
		AlertLevel2_str );               //"17"
}
#endif



static int send_packet(char *Description)  // PING, DATA, ALERT
{
	int result;
	char msg[75];

	result = cell_socket_transmit(strlen(uart1_outbuf));

	if (result == ACK_TIMEOUT)
	{
		sprintf(msg,"%s: ACK_TIMEOUT", Description);
		update_error(msg);
	}
	else
	{
		sprintf(msg,"%s sent", Description);
		update_status(msg);
	}

#if 0
	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		trace_AppendMsg(msg);
		trace_AppendMsg("\n");
	}
#endif

	// Report UART buffer sizes here
	check_and_report_uart_buffer_overflow();

	return result;
}



#if 0
static int send_settings_packet(void)
{

	int result;

	s2_prep_settings_packet(uart1_outbuf);
	result = send_packet("SETTINGS");

#if 1
	//======Test stuff here======
	prep_header_packet();
	result = send_packet("HEADER");


	//test_entire_log();


	//====== End Test stuff here======
#endif


	return result;
}
#endif

#define POWER_ON_PING 1
#define DATA_PING     2

static int prep_and_send_PING(int PingType)
{

	char msg[25];

	if (PingType == POWER_ON_PING)
	{
		strcpy(msg,"SETTINGS");
		s2c_prep_settings_packet(uart1_outbuf);
	}
	else
	{
		strcpy(msg,"HEADER");
		prep_header_packet();
	}


	return send_packet(msg);

}





static uint16_t cell_poweron_acc;
static uint16_t cell_powerdown_acc;
static uint32_t cell_power_on_tick_start;

int s2c_cell_power_is_on = 0;

int s2c_try_band_reboot = 0;

static uint32_t s2c_get_power_on_duration_sec(void)
{
	uint32_t duration_sec;
	if (s2c_cell_power_is_on)
	{
	  if (cell_powerup_time == 0) return 0;  // should never happen
	  duration_sec = rtc_get_time() - cell_powerup_time;
	}
	else
	{
	  duration_sec = 0;
	}
	return duration_sec;
}

static uint32_t s2c_get_power_off_duration_sec(void)
{
	uint32_t duration_sec;
	if (s2c_cell_power_is_on)
	{
		duration_sec = 0;
	}
	else
	{
		if (cell_powerdown_time == 0) return 0;  // ignore startup
		duration_sec = rtc_get_time() - cell_powerdown_time;
	}
	return duration_sec;
}

static void establish_cell_power_on_time(void)
{
	cell_poweron_acc = ltc2943_get_raw_accumulator();
	cell_power_on_tick_start = HAL_GetTick();
	cell_powerup_time = rtc_get_time();
	s2c_Wake_Time = cell_powerup_time + 1800; // Leave cell on for max of 30 minutes at a time
}

static uint32_t s2c_powerup_cell_hardware(void)
{
	uint32_t duration_off_sec;
	uint32_t HwFlowCtl;
	char msg[100];

	if (s2c_cell_power_is_on)
	{
		if (s2c_CellModemFound == S2C_LAIRD_MODEM)
		{
			// check for urc KSUP
			if (strstr((const char *)uart1_inbuf,"+KSUP: "))
			{
				if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
				{
					char msg[80];
					sprintf(msg,"\nKSUP detected in %lu sec.\n", s2c_get_power_on_duration_sec());
					trace_AppendMsg(msg);
				}
				//return ?;  what to return
			}
		}

		return s2c_get_power_on_duration_sec();
	}
	duration_off_sec = s2c_get_power_off_duration_sec();

	s2c_cell_power_is_on = 1;

	uart1_flush_inbuffer();

	// Initialize the UART for the specified cell module
	switch (s2c_CellModemFound)
	{
	  default:
	  case S2C_NO_MODEM:    HwFlowCtl = UART_HWCONTROL_NONE;    break;
	  case S2C_TELIT_MODEM: HwFlowCtl = UART_HWCONTROL_RTS_CTS; break;
	  case S2C_XBEE3_MODEM: HwFlowCtl = UART_HWCONTROL_NONE;    break;
	  case S2C_LAIRD_MODEM: HwFlowCtl = UART_HWCONTROL_RTS_CTS; break;
	}

    MX_USART1_UART_Init(115200, UART_STOPBITS_1, HwFlowCtl);

    // Turn on the CELL
	HAL_GPIO_WritePin(GEN45_CELL_PWR_EN_Port, GEN45_CELL_PWR_EN_Pin, 1);


	//HAL_led_delay(1);  // is this really necessary ?

	// At this point, the cell modem is deemed to have power
	establish_cell_power_on_time();

	sprintf(msg, "Cell power is on. Was off for %lu sec.", duration_off_sec);

	if (sd_UART_5SPY1)
	{
		lpuart1_tx_string("\n\n=====\n");
		lpuart1_tx_string(msg);
		lpuart1_tx_string("\n");
	}

	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		trace_AppendMsg("\n\n=====\n");
		trace_TimestampMsg(msg);
	}

	AppendStatusMessage(msg);  // useful for debugging power up issues

	return 0;   // It has been on for zero sec

}

uint32_t s2c_powerdown_cell_hardware(void)
{
	uint32_t duration_on_sec;
	uint16_t acc_consumed;
	char msg[100];

	if (s2c_cell_power_is_on == 0) return 0;  // power is already off, nothing to do

	duration_on_sec = s2c_get_power_on_duration_sec();  // calculate how long power was on

	s2c_cell_power_is_on = 0;
	s2c_Wake_Time = 0;

	// Clear all state variables since power is going off

	state_CGDCONT = 0;
	state_KCNXCFG = 0;
	state_CEREG = 0;
	state_SCFG = 0;
	state_SGACT_SET = 0;
	state_SGACT_QUERY = 0;
	state_NETWORK_CONNECTED = 0;
	state_TCP_CONFIGURED = 0;
	state_SOCKET_OPEN = 0;
	state_LAIRD_MUX = 0;

	ack_timeout_count = 0;
	ack_timeout = CLOUD_ACK_TIMEOUT;

	cell_trigger = TRIGGER_NONE;
	cell_state = CELL_OFF;

	HAL_GPIO_WritePin(GEN45_CELL_PWR_EN_Port, GEN45_CELL_PWR_EN_Pin, 0);

	HAL_UART_MspDeInit(&huart1);

	cell_powerdown_time = rtc_get_time();

	cell_powerdown_acc = ltc2943_get_raw_accumulator();

	acc_consumed = cell_poweron_acc - cell_powerdown_acc;

	sprintf(msg, "Cell power is off. Was on for %lu sec. Used %u acc", duration_on_sec, acc_consumed);

	if (sd_UART_5SPY1)  // VEL2 spies on CELL
	{
		lpuart1_tx_string("\n\n");
		lpuart1_tx_string(msg);
		lpuart1_tx_string("\n");
	}

	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		trace_AppendMsg("\n\n");
		trace_TimestampMsg(msg);
	}

	trace_SaveBuffer();  // Just in case other logging is active, this event will help sync things

	AppendStatusMessage(msg);  // useful for debugging power up issues

	return duration_on_sec;
}

static uint32_t get_powerup_delay_sec()
{
	uint32_t delay_ms;

	switch (s2c_CellModemFound)
	{
	  default:
	  case S2C_NO_MODEM:    delay_ms = 0;                   break;
	  case S2C_TELIT_MODEM: delay_ms = TELIT_POWER_ON_DELAY; break;
	  case S2C_XBEE3_MODEM: delay_ms = 3000;                 break;
	  case S2C_LAIRD_MODEM: delay_ms = LAIRD_POWER_ON_DELAY; break;
	}
	return delay_ms / 1000;
}



static void check_laird_MUX(void)
{
	static int laird_is_MUX_switch_required = 2;  // 0 = no; 1 = yes; 2 = check at power on

	// Kludge for laird communication boards that do not have the AUTORUN pin (M2.41) grounded
	//
	// Some black comm boards did not ground the AUTORUN pin and so the Laird can have the
	// MUX pointing to the Nordic 840 instead of the HL7800.  This will bypass
	// that issue.  It has to be done at each power on.  If the HL7800 is connected,
	// it will report an error for this command, but only after the OK timeout.  Hmmm....

	if (s2c_CellModemFound != S2C_LAIRD_MODEM) return;

	if (state_LAIRD_MUX) return; // Laird MUX was already applied for this power on cycle

	state_LAIRD_MUX = 1;

	//AppendStatusMessage("Checking Laird MUX");

	if (laird_is_MUX_switch_required == 2)  // must check once at power on
	{
		if (AT_OK("AT!\r", 50))  // The Nordic 840 usually responds in 1 ms, so 50 is plenty.
		{
			// Got an OK, so the MUX switch is required
			laird_is_MUX_switch_required = 1;

			AppendStatusMessage("Laird MUX switch is required at each power on.");

			// Flush RX buffer
			uart1_flush_inbuffer();

			return;
		}
		else
		{
			// Command timed out, so MUX is already pointing to HL7800

			laird_is_MUX_switch_required = 0;

			AppendStatusMessage("Laird MUX switch is NOT required at power on.");

			// Flush RX buffer
			uart1_flush_inbuffer();

			return;
		}
	}
	else if (laird_is_MUX_switch_required == 1)
	{
		AT_OK("AT!\r", 50);

		AppendStatusMessage("Laird MUX switch applied.");

		// Flush RX buffer
		uart1_flush_inbuffer();
	}
}

static void log_power_on_messages(void)
{
	if (strlen((const char *)uart1_inbuf))
	{
		if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
		{
			trace_AppendMsg("\nCell Modem Power On Message\n\n>>");
			trace_AppendMsg((const char *)uart1_inbuf);
			trace_AppendMsg("<<\n\n");
		}

		// Flush RX buffer as power on messages (URCs) are not currently supported
		uart1_flush_inbuffer();
	}
}


static int s2c_is_cell_powerup_complete(void)  // This requires global s2c_CellModemFound be set
{
	uint32_t time_power_has_been_on_sec;

	time_power_has_been_on_sec = s2c_powerup_cell_hardware();

	if (time_power_has_been_on_sec > get_powerup_delay_sec())
	{
		log_power_on_messages();  // For diagnostics

		check_laird_MUX();

		sprintf(s2c_cell_status, "Warmup complete.");

		return 1;  // cell hardware powerup is complete, including any laird MUX switching
	}
	else
	{
		int seconds_remaining;
		seconds_remaining = get_powerup_delay_sec() - time_power_has_been_on_sec;

		sprintf(s2c_cell_status, "Warmup for %d seconds.", seconds_remaining);
	}

	return 0;
}

#if 0
int s2c_is_cell_powerup_complete(int *pRemainingSec)
{
	uint32_t time_power_has_been_on_ms;
	uint32_t time_needed_ms;

	// If there is no cell modem, the powerup is complete
	if (s2c_CellModemFound == S2C_NO_MODEM)
	{
		if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
		{
			char msg[80];
			sprintf(msg,"\nNo cell modem found.  Cell power up complete in 0 ms.\n");
			trace_AppendMsg(msg);
		}
		if (pRemainingSec) *pRemainingSec = 0;
		s2c_try_band_reboot = 0;
		return 1;
	}

	time_needed_ms = get_powerup_delay_ms();

	time_power_has_been_on_ms = s2c_powerup_cell_hardware();  // this will power on cell hardware if not on already

	if (time_power_has_been_on_ms > time_needed_ms)
	{
		if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
		{
			char msg[80];
			sprintf(msg,"\nPower on delay completed.  Cell power up complete in %lu ms.\n", time_power_has_been_on_ms);
			trace_AppendMsg(msg);
		}
		if (pRemainingSec) *pRemainingSec = 0;
		s2c_try_band_reboot = 0;
		fix_comm_board_issue();
		return 1;
	}

	// This is laird specific and requires the laird be configured with AT+KSREP=1
	// Check uart buffer for URC that indicates powerup is complete

	if (cell_urc_received == RC_KSUP)
	//if (strstr((const char *)uart1_inbuf, "+KSUP: 0"))
	{
		if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
		{
			char msg[80];
			sprintf(msg,"\nKSUP detected.  Cell power up complete in %lu ms.\n", time_power_has_been_on_ms);
			trace_AppendMsg(msg);
		}
		if (pRemainingSec) *pRemainingSec = 0;
		s2c_try_band_reboot = 0;
		fix_comm_board_issue();
		return 1;
	}

	// Cell modem powerup is not yet complete, compute remaining time in seconds

	if (pRemainingSec)
	{
		int time_remaining_sec;

		// To allow formatting a message for the UI
		time_remaining_sec = (time_needed_ms - time_power_has_been_on_ms) / 1000 + 1;

		*pRemainingSec = time_remaining_sec;
	}

	return 0;
}
#endif


void S2C_POWERUP(void)
{

	if (s2c_CellModemFound == S2C_NO_MODEM) return;

	while (!s2c_is_cell_powerup_complete())
	{
		HAL_led_delay(1);  //Modem Warm up must be at least Delay seconds before it will talk -- confirmed by testing (zg).  Varies with modem.
	}
}

void laird_exit_data_mode(void)
{
	// From HL7800 AT command guide on the +++ Command
	// Line needs one second of silence before and one second after, do not end with terminating character

	HAL_led_delay(1001);
	AT("+++", 2000);
	HAL_led_delay(1001);
}

static void laird_close_socket(void)
{
	char cmd[64];  // to leave room for a long URL

	sprintf(cmd,"AT+KTCPCLOSE=%d,1\r", session_id);  // unclear if the <closing_type> argument should be used...
	AT_OK(cmd, 5000);   // this is the Socket Close command

	sprintf(cmd,"AT+KTCPDEL=%d\r", session_id);
	AT_OK(cmd, 5000);

	state_SOCKET_OPEN = 0;

}

#if 0
static void laird_close_socket(void)
{
	if (state_SOCKET_OPEN)
	{
		char cmd[64];  // to leave room for a long URL
		//cell_send_string("+++");  // Send the escape sequence, not sure about how long to wait, no response is required by spec.  Not an AT command.
		// one second of silence before
		// send +++ with no cr
		// one second of silence after
		// response is OK
		HAL_led_delay(1000);
		AT_OK("+++", 5000); // The +++ sequence will result in an OK from the command that opened the socket
		sprintf(cmd,"AT+KTCPCLOSE=%d\r", session_id);
		AT_OK(cmd, 5000);   // this is the Socket Close command

		state_SOCKET_OPEN = 0;
	}
}
#endif


#if 0
static void laird_power_down(void)
{
	if (state_SOCKET_OPEN)
	{
		char cmd[64];  // to leave room for a long URL
		//cell_send_string("+++");  // Send the escape sequence, not sure about how long to wait, no response is required by spec.  Not an AT command.
		// one second of silence before
		// send +++ with no cr
		// one second of silence after
		// response is OK
		HAL_led_delay(1000);
		AT_OK("+++", 5000); // The +++ sequence will result in an OK from the command that opened the socket
		//AT_OK("AT+KTCPCLOSE=1\r", 5000);   // this is the Socket Close command
		sprintf(cmd,"AT+KTCPCLOSE=%d\r", session_id);   // Example:  AT+KTCPSTART=2
		AT_OK(cmd, 5000);   // this is the Socket Close command
	}

	AT_OK("AT+CPWROFF=1\r", 1000); // this is the fast modem power off command

	HAL_led_delay(1000);
}
#endif


// From datasheet for Multitech MTSMC-MNA1, page 11:
// To properly power-down your device, use the following sequence:
// 1. Issue the AT#SHDN command.
// 2. Wait 30 seconds.
// 3. Power off or disconnect power.
//
#if 0
static void telit_power_down(void)
{
	if (state_SOCKET_OPEN)
	{
		//cell_send_string("+++");  // Send the escape sequence, not sure about how long to wait, no response is required by spec.  Not an AT command.
		AT_OK("+++", BREAK_TIMEOUT); // The +++ sequence will result in an OK from the command that opened the socket
		AT_OK("AT#SH=1\r", SH_TIMEOUT);   // this is the Socket Shutdown command
	}


	AT_OK("AT#SHDN\r", SHDN_TIMEOUT); // this is the modem Shutdown command
	// zg - datasheet says to wait 30 seconds after AT#SHDN before removing power to preserve non-volatile memory.
	// Original code was using 3000 so leave it
	HAL_led_delay(3000);
}
#endif

#define TELIT_DEF_TIMEOUT    2000L

#define LAIRD_DEF_TIMEOUT    2000L
#define LAIRD_KCARRIERCFG_TIMEOUT    4000L

#define XBEE3_DEF_TIMEOUT    2000L
#define XBEE3_BREAK_TIMEOUT  1500L    // use 1.5 sec
#define XBEE3_SD_TIMEOUT     1500L  // after two minutes the Xbee will return ERROR to the SD command

static int xbee3_enter_command_mode(void)
{
	// To enter command mode from transparent mode at power on:
	// 1. wait 1 second
	// 2. send +++ (no CR)
	// 3. wait 1 second
	// 4. Xbee will respond "OK\r"
	HAL_led_delay(1500);
	return AT_OK("+++", 5000); // The +++ sequence will result in an OK\r after 1 second
	//return AT_OK("+++", XBEE3_BREAK_TIMEOUT); // The +++ sequence will result in an OK\r after 1 second

}

static void xbee3_exit_command_mode(void)
{
    // Two ways to exit command mode:
	//  a) issue CN command
	//  or - b) wait 10 seconds
	AT_OK("ATCN\r", XBEE3_DEF_TIMEOUT);
}

#if 0
static void xbee3_power_down(void)
{
	if (state_SOCKET_OPEN)
	{
		xbee3_enter_command_mode();
	}

	AT_OK("ATSD\r", XBEE3_SD_TIMEOUT);   // this is the Shutdown command
}
#endif

static int xbee3_get_imei()
{
	AT_OK("ATIM\r", XBEE3_DEF_TIMEOUT);   // returns 356441114952738


	// Parse the response  356441114952738\r
    if (strlen((const char *)uart1_inbuf) > 15)
    {
		for (int i = 0; i < 15; i++)
		{
			s2c_imei[i] = uart1_inbuf[i];
		} //356441114952738\r

		s2c_imei[15] = '\0'; // Every String in C is terminated with a NULL

		return 1;
	}
	else
	{
		// No IMEI found, zero out the return buffer
		for (int i = 0; i < 15; i++)
		{
			s2c_imei[i] = 0;
		}
	}
	return 0;
}

static void xbee3_association_status(void)
{
	int i;

	for (i=0; i < 60; i++)
	{
		AT_OK("ATSI\r", XBEE3_DEF_TIMEOUT);    // Socket Info command (list of socket IDs)
		//AT_OK("ATSI 0\r", XBEE3_DEF_TIMEOUT);  // Socket Info for socket ID 0
		AT_OK("ATAI\r", XBEE3_DEF_TIMEOUT);    // Association Indication

		// Parse the HEX response  2D\r
		if (strlen((const char *)uart1_inbuf) > 0)
		{
			uint32_t status;

			status = (uint32_t) strtoul((const char *)uart1_inbuf, 0, 16);

			if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
			{
				char msg[100];
				sprintf(msg, "\n\nAI value is %lu at attempt %d", status, i);
				trace_AppendMsg(msg);
			}
		}

		//HAL_led_delay(30000);
		HAL_led_delay(5000);
	}

}

static void xbee3_get_utc()
{
	AT_OK("ATDL\r", XBEE3_DEF_TIMEOUT);    // Destination Address

	AT_OK("ATDE\r", XBEE3_DEF_TIMEOUT);    // Destination Port

	xbee3_association_status();


	// The response is the number of seconds since 2000-01-01 00:00:00
	// as a 32 bit HEX number;
	AT_OK("ATDT\r", XBEE3_DEF_TIMEOUT);    // Cellular Network Time - this is only updated about once per hour

	// Parse the HEX response  2826D7EC\r
    if (strlen((const char *)uart1_inbuf) > 3)
    {
    	uint32_t dt;
    	//dt = (uint32_t) atoi(inbuf1);
    	dt = (uint32_t) strtoul((const char *)uart1_inbuf, 0, 16);

    	// verify dt is a reasonable number
    	if ((dt > 1620694800) && (dt < 2251846800))  // Valid dates are:  05-11-2021 1:00:00 am plus 20 years
    	{
        	rtc_sync_time_from_ptime(&dt);
    	}

    	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
    	{
    		char msg[100];
    		sprintf(msg, "\n\nDT value is %lu s", dt);
    		trace_AppendMsg(msg);
    	}
    }
}


void xbee3_reset(void)
{

	SetCellEnabled();
	SetCellModule(CELL_MOD_XBEE3);
	saveMainConfig = 1;
	Reset_CellConfig();
	saveCellConfig = 1;
	s2c_CellModemFound = S2C_XBEE3_MODEM;


	S2C_POWERUP();

	// From page 127 of Digi XBee� 3 Cellular LTE-M/NB-IoT Global Smart Modem User Guide

	// Two alternatives for bringing up Xbee3 in a known state:
	//   1. Send a break for 6 seconds to force xbee3 into a known state (9600 baud, Command mode)
	//   2. Assert DIN (low) at power on
	// Both will return OK/r at 9600 baud to confirm Command mode.

	// Power xbee3 on
	// Use AT commands to configure the xbee3 to Eastech defaults:
	// RE   Reset to factory defaults
	// DL 54.24.219.107
	// DE 8AE               <==  port_#_in_hex  2222 = 8AE
	// BD 7    to specify baud of 115200
	// WR      save changes
	// CN      exit command mode and apply changes


}

void telit_reset(void)
{


	SetCellEnabled();
	SetCellModule(CELL_MOD_TELIT);
	saveMainConfig = 1;
	Reset_CellConfig();
	saveCellConfig = 1;
	s2c_CellModemFound = S2C_TELIT_MODEM;

  S2C_POWERUP();

  AT_OK("AT&F1\r"       , TELIT_DEF_TIMEOUT); // Set Full Factory Defaults
  AT_OK("ATE0\r"        , TELIT_DEF_TIMEOUT); // Echo Off
#if 0
  AT_OK("ATZ0\r"        , TELIT_DEF_TIMEOUT); // Soft Reset
  AT_OK("AT#Z=0\r"      , TELIT_DEF_TIMEOUT); // Extended Reset
#endif
  AT_OK("AT+COPS=0\r"   , TELIT_DEF_TIMEOUT); // Automatic registration
  AT_OK("AT&P0\r"       , TELIT_DEF_TIMEOUT); // Load profile zero at startup
  AT_OK("AT&W0\r"       , TELIT_DEF_TIMEOUT); // Save active configuration to profile zero
  AT_OK("AT+CFUN=1,1\r" , TELIT_DEF_TIMEOUT); // Reboot module

  // Why wait here ?  If done via "set serial number" path, Production is not likely to talk to the modem.
  // If done via UI, again, user is not likely to talk to modem.  Unless doing the PING test...
  //HAL_led_delay(1000);  // wait one second just in case.  Won't be noticed by production or UI
  HAL_led_delay(TELIT_POWER_ON_DELAY);  // This is 10 seconds
  //AT_OK("AT\r" , TELIT_DEF_TIMEOUT); // For testing modem responsiveness after reboot -- use SD card to log response


}

static void laird_factory_config(void)
{
	// ASSERT:  state machine has powered on the cell modem

	// Default factory settings for HL78xx are:
	//
	//  E1    Echo On
	//  Q0
	//  V1
	//  X4
	//  &C1   DCD line active when data carrier present
	//  &D1   DTR drops from active to inactive - change to command mode
	//  &R1   RTS/CTS see &K3
	//  &S0   DSR signal is always active
	//  +IFC=2,2      DTE-DCE local flow control is RTS/CTS
	//  &K3           Hardware flow control
	//  +IPR=115200   Set baud
	//  +FCLASS0
	//  S00:0
	//  S01:0
	//  S03:13
	//  S04:10
	//  S05:8
	//  S07:255
	//  S08:0
	//  S10:1

	// Operator Id's
	// 0 Default
	// 1 Verizon
	// 2 CMCC
	// 3 RJIL
	// 4 KDDI
	// 5 AT&T
	// 6 USCC
	// 7 Docomo
	// 8 Softbank
	// 9 LGU+
	// 10 KT
	// 11 T-Mobile
	// 12 SKT
	// 13 TELSTRA
	// 14 China Telecom
	// 15 Sierra Wireless

	AT_OK("AT&F\r",         LAIRD_DEF_TIMEOUT); // Set Factory Defaults
	AT_OK("ATE0\r"        , LAIRD_DEF_TIMEOUT); // Echo Off

	AT_OK("AT+KSIMDET=0\r", LAIRD_DEF_TIMEOUT); // Disable SIM detect
	//AT_OK("AT+KSIMSEL=9\r", LAIRD_DEF_TIMEOUT); // Force internal SIM card
	AT_OK("AT+KSIMSEL=0\r", LAIRD_DEF_TIMEOUT); // Force external SIM card  --- preferred company default going forward


#if 0
	// KSIMDET=1 and KSIMSEL=20 do not always result in the external SIM card being selected on the black comm boards
	AT_OK("AT+KSIMDET=1\r", LAIRD_DEF_TIMEOUT);  // Enable SIM detect
	AT_OK("AT+KSIMSEL=20\r", LAIRD_DEF_TIMEOUT); // Enable auto SIM select (does not appear to work)
#endif



	// https://www.phonearena.com/news/Cheat-sheet-which-4G-LTE-bands-do-AT-T-Verizon-T-Mobile-and-Sprint-use-in-the-USA_id77933
	//
	// AT&T               2,12,17   then 4&66,5  licensed but not used much  14,29,30
	// T-Mobile           2,4,12,66 not used much 5, 71
	// Verizon            2,4,66,13 then 5,46,48
	// Sprint/T-Mobile    25, 26, not used much 41

	// Search Strategy:  2,4,12,66,13,17[,5,26,25]  Note:  66 is a superset of 4, so enable both of those at the same time?

#if 0
	//
	//    3            2           1          0
	//  2109 8765 4321 0987 6543 2109 8765 4321
	//  0000 0011 0000 0001 0001 1000 0001 1010   = 0301 181A       2,4,5,12,13,17,25,26
	AT_OK("AT+KBNDCFG=0,0301181A\r", LAIRD_DEF_TIMEOUT);
#endif

	//    3            2           1          0
	//  2109 8765 4321 0987 6543 2109 8765 4321
	//  0000 0000 0000 0001 0001 1000 0000 1010   = 0001 180A        2,4,12,13,17
	AT_OK("AT+KBNDCFG=0,1180A\r", LAIRD_DEF_TIMEOUT);




	// Notes on the cmd  AT+KCARRIERCFG=  from the User Manual:
	//    Operator must be set prior to using the module. Refer to section 6 of AirPrime HL7800-M MNO and
	//    RF Band Customization at Customer Production Site Application Note (reference number:
	//    2174213) for details.
	//
	//    Configuration is saved immediately in non-volatile memory. The answer to the write
	//    command is therefore displayed a few seconds after it is sent. However, the new
	//    configuration is only taken into account on the next reboot.
	//
	AT_OK("AT+KCARRIERCFG=0\r", LAIRD_KCARRIERCFG_TIMEOUT);    // WARNING: Must Set Operator. The value zero sets Operator to default. See list above.


	//AT_OK("AT+KSREP=0\r",   LAIRD_DEF_TIMEOUT); // Disable URC at startup
	AT_OK("AT+KSREP=1\r",   LAIRD_DEF_TIMEOUT); // Enable +KSUP URC at startup.  Useful to speed up turn on and to detect unexpected power resets

	AT_OK("AT+CREG=0\r",    LAIRD_DEF_TIMEOUT); // Disable URC
	AT_OK("AT+CEREG=0\r",   LAIRD_DEF_TIMEOUT); // Disable URC
	//AT_OK("AT+CEREG=3\r",   LAIRD_DEF_TIMEOUT); // Enable URC with extended info
	AT_OK("AT+COPS=0\r",    LAIRD_DEF_TIMEOUT); // Automatic registration

	AT_OK("AT+KURCCFG=\"TCP\",0,0\r", LAIRD_DEF_TIMEOUT);  // disable URCs for TCP


	// Note that CGDCONT is stored in non-volatile memory, but KCNXCFG is not.  Both must match so must do at each power on.
	//AT_OK("AT+CGDCONT=1,"IP","APN"\r",    LAIRD_DEF_TIMEOUT); // Define PDP Context need cid and APN see +KCARRIERCFG
	//Refer to Table 2 Device Configuration of AirPrime
	//HL7800-M MNO and RF Band Customization at Customer Application Site
	//Application Note (reference number 2174213) for configuration description.
	//set_CGDCONT(cid, APN); // Define PDP Context, saved in non-volatile memory
	AT_OK("AT+CGDCONT=1,\"IP\",\"flolive.net\"\r",    LAIRD_DEF_TIMEOUT); // Define PDP Context using defaults.  Normally use configured cid and APN at run time. See +KCARRIERCFG

	AT_OK("AT&W\r"        , LAIRD_DEF_TIMEOUT); // Save active configuration to profile zero

	// ASSERT:  The state machine will power off the cell modem next

}






static void laird_internal_sim(void)  // {"comm":35}
{
  if (AT_OK("AT+KSIMDET=0\r", LAIRD_DEF_TIMEOUT)) // Disable SIM detect
  {
    if (AT_OK("AT+KSIMSEL=9\r", 9000)) // Force internal SIM card, 4 sec was too short?
    {
       // The KSIMSEL command is sticky, no need to write profile
       //AT_OK("AT&W\r"        , LAIRD_DEF_TIMEOUT); // Save active configuration to profile zero
       //HAL_led_delay(1000);  // give the modem time to do the write
       update_status("iSIM selected.");
	   return;
    }
  }

  update_error("iSIM error.");
}

static void laird_external_sim(void)   // {"comm":36}
{
  if (AT_OK("AT+KSIMDET=0\r", LAIRD_DEF_TIMEOUT)) // Disable SIM detect
  {
    if (AT_OK("AT+KSIMSEL=0\r", 9000)) // Force external SIM card
    {
       // The KSIMSEL command is sticky, no need to write profile
       //AT_OK("AT&W\r"        , LAIRD_DEF_TIMEOUT); // Save active configuration to profile zero
       //HAL_led_delay(1000);  // give the modem time to do the write
       update_status("eSIM selected.");
	   return;
    }
  }
  update_error("eSIM error.");
}

#if 0
  // KSIMDET=1 and KSIMSEL=20 do not always result in the external SIM card being selected on the black comm boards
  AT_OK("AT+KSIMDET=1\r", LAIRD_DEF_TIMEOUT);  // Enable SIM detect
  AT_OK("AT+KSIMSEL=20\r", LAIRD_DEF_TIMEOUT); // Enable auto SIM select (does not appear to work)
#endif


  // Notes on the cmd  AT+KBNDCFG=  from the User Manual:
  //
  // RF bands must be set prior to using the module. It is highly recommended to limit the number of
  // enabled RF bands to lessen power consumption. Additionally, the number of enabled RF bands
  // should be limited to avoid prolonged scanning operations. Scanning operations take place
  // regardless of number of RF bands enabled but will take longer if too many bands are enabled.
  // Refer to section 5 of AirPrime HL78XX Customization Guide Application Note (reference number:
  // 2174213) for details.

  // 808189F sets LTE bands 28,20,13,12,8,5,4,3,2,1 and seems to be the default out of factory config ?



// List of all bands supported by this module for CAT-M1.
// This value is what is returned by AT+KBNDCFG=?
// bands 1, 2, 3, 4, 5, 8, 9, 10, 12, 13,
// 17, 18, 19, 20, 25, 26, 27, 28, 66

//    3            2           1          0
//  2109 8765 4321 0987 6543 2109 8765 4321     Hex              Human
//  0000 0000 0000 0000 0000 0000 0000 0001   = 0000 0001        1
//  0000 0000 0000 0000 0000 0000 0000 0010   = 0000 0002        2
//  0000 0000 0000 0000 0000 0000 0000 0100   = 0000 0004        3
//  0000 0000 0000 0000 0000 0000 0000 1000   = 0000 0008        4
//  0000 0000 0000 0000 0000 0000 0001 0000   = 0000 0010        5
//  0000 0000 0000 0000 0000 0000 1000 0000   = 0000 0080        8
//  0000 0000 0000 0000 0000 0001 0000 0000   = 0000 0100        9
//  0000 0000 0000 0000 0000 0010 0000 0000   = 0000 0200        10
//  0000 0000 0000 0000 0000 1000 0000 0000   = 0000 0800        12
//  0000 0000 0000 0000 0001 0000 0000 0000   = 0000 1000        13
//  0000 0000 0000 0001 0000 0000 0000 0000   = 0001 0000        17
//  0000 0000 0000 0010 0000 0000 0000 0000   = 0002 0000        18
//  0000 0000 0000 0100 0000 0000 0000 0000   = 0004 0000        19
//  0000 0000 0000 1000 0000 0000 0000 0000   = 0008 0000        20
//  0000 0001 0000 0000 0000 0000 0000 0000   = 1000 0000        25
//  0000 0010 0000 0000 0000 0000 0000 0000   = 2000 0000        26
//  0000 0100 0000 0000 0000 0000 0000 0000   = 4000 0000        27
//  0000 1000 0000 0000 0000 0000 0000 0000   = 8000 0000        28


#define BAND_1    "AT+KBNDCFG=0,1\r"
#define BAND_2    "AT+KBNDCFG=0,2\r"
#define BAND_3    "AT+KBNDCFG=0,4\r"
#define BAND_4    "AT+KBNDCFG=0,8\r"
#define BAND_5    "AT+KBNDCFG=0,10\r"
#define BAND_8    "AT+KBNDCFG=0,80\r"
#define BAND_9    "AT+KBNDCFG=0,100\r"
#define BAND_10   "AT+KBNDCFG=0,200\r"
#define BAND_12   "AT+KBNDCFG=0,800\r"
#define BAND_13   "AT+KBNDCFG=0,1000\r"
#define BAND_17   "AT+KBNDCFG=0,10000\r"
#define BAND_18   "AT+KBNDCFG=0,20000\r"
#define BAND_19   "AT+KBNDCFG=0,40000\r"
#define BAND_20   "AT+KBNDCFG=0,80000\r"
#define BAND_25   "AT+KBNDCFG=0,10000000\r"
#define BAND_26   "AT+KBNDCFG=0,20000000\r"
#define BAND_27   "AT+KBNDCFG=0,40000000\r"
#define BAND_28   "AT+KBNDCFG=0,80000000\r"
#define BAND_66   "AT+KBNDCFG=0,20000000000000000\r"


// 9000 + Country Number is reserved for setting a default group of bands for a country
  //    3            2           1          0
  //  2109 8765 4321 0987 6543 2109 8765 4321
  //  0000 0000 0000 0001 0001 1000 0000 1010   = 0001 180A        2,4,12,13,17
#define BAND_9001 "AT+KBNDCFG=0,1180A\r"       // default USA bands

  //  6           5            4           3            2           1          0
  //  0987 6543 2109 8765 4321 0987 6543 2109 8765 4321 0987 6543 2109 8765 4321
  //  0000 0000 0000 0000 0000 0000 0000 0000 1000 0000 0000 0000 0000 0101 0101   = 08000055
#define BAND_9061 "AT+KBNDCFG=0,08000055\r"    // default AU Bands 1, 3, 5, 7, 28   (40 is not supported)

  //    3            2           1          0
  //  2109 8765 4321 0987 6543 2109 8765 4321
  //  0000 1000 0000 0000 0000 0000 0000 0100   = 08000004   Bands 3 and 28
#define BAND_9064 "AT+KBNDCFG=0,08000004\r"    // default NZ bands 3, 28




// Both +KBND? and +KBNDCFG? can share the same band masks
// "+KBND: 0,00000000000000000002"
// "+KBNDCFG: 0,00000000000000000002"
// Note that KBND only works if the radio is actually connected to the network
// Whereas KBNDCFG will work at any time

#define IS_BAND_1       ": 0,00000000000000000001"
#define IS_BAND_2       ": 0,00000000000000000002"
#define IS_BAND_3       ": 0,00000000000000000004"
#define IS_BAND_4       ": 0,00000000000000000008"
#define IS_BAND_5       ": 0,00000000000000000010"
#define IS_BAND_8       ": 0,00000000000000000080"
#define IS_BAND_9       ": 0,00000000000000000100"
#define IS_BAND_10      ": 0,00000000000000000200"
#define IS_BAND_12      ": 0,00000000000000000800"
#define IS_BAND_13      ": 0,00000000000000001000"
#define IS_BAND_17      ": 0,00000000000000010000"
#define IS_BAND_18      ": 0,00000000000000020000"
#define IS_BAND_19      ": 0,00000000000000040000"
#define IS_BAND_20      ": 0,00000000000000080000"
#define IS_BAND_25      ": 0,00000000000010000000"
#define IS_BAND_26      ": 0,00000000000020000000"
#define IS_BAND_27      ": 0,00000000000040000000"
#define IS_BAND_28      ": 0,00000000000080000000"
#define IS_BAND_66      ": 0,00020000000000000000"

#define IS_BAND_9001    ": 0,0000000000000001180A"
#define IS_BAND_9061    ": 0,00000000000008000055"
#define IS_BAND_9064    ": 0,00000000000008000004"


int laird_get_band_in_use(void)
{
	if (state_CEREG == 0) return 0;  // not registered yet

	// Determine which band is actually in use
	if (!AT_OK("AT+KBND?\r", LAIRD_DEF_TIMEOUT))
	{
		return 0;
	}
	if (strstr((const char *)uart1_inbuf, IS_BAND_1))  return 1;
	if (strstr((const char *)uart1_inbuf, IS_BAND_2))  return 2;
	if (strstr((const char *)uart1_inbuf, IS_BAND_3))  return 3;
	if (strstr((const char *)uart1_inbuf, IS_BAND_4))  return 4;
	if (strstr((const char *)uart1_inbuf, IS_BAND_5))  return 5;
	if (strstr((const char *)uart1_inbuf, IS_BAND_8))  return 8;
	if (strstr((const char *)uart1_inbuf, IS_BAND_9))  return 9;
	if (strstr((const char *)uart1_inbuf, IS_BAND_10)) return 10;
	if (strstr((const char *)uart1_inbuf, IS_BAND_12)) return 12;
	if (strstr((const char *)uart1_inbuf, IS_BAND_13)) return 13;
	if (strstr((const char *)uart1_inbuf, IS_BAND_17)) return 17;
	if (strstr((const char *)uart1_inbuf, IS_BAND_18)) return 18;
	if (strstr((const char *)uart1_inbuf, IS_BAND_19)) return 19;
	if (strstr((const char *)uart1_inbuf, IS_BAND_20)) return 20;
	if (strstr((const char *)uart1_inbuf, IS_BAND_25)) return 25;
	if (strstr((const char *)uart1_inbuf, IS_BAND_26)) return 26;
	if (strstr((const char *)uart1_inbuf, IS_BAND_27)) return 27;
	if (strstr((const char *)uart1_inbuf, IS_BAND_28)) return 28;
	if (strstr((const char *)uart1_inbuf, IS_BAND_66)) return 66;
	if (strstr((const char *)uart1_inbuf, IS_BAND_9001)) return 9001;
	if (strstr((const char *)uart1_inbuf, IS_BAND_9061)) return 9061;
	if (strstr((const char *)uart1_inbuf, IS_BAND_9064)) return 9064;
	return 0; // unknown band
}

static void laird_try_band(int n)
{
	char *band;
	char *is_band;
	char msg[80];

	switch (n)
	{

	case 1:   band = BAND_1;  is_band = IS_BAND_1; break;
	case 2:   band = BAND_2;  is_band = IS_BAND_2; break;
	case 3:   band = BAND_3;  is_band = IS_BAND_3; break;
	case 4:   band = BAND_4;  is_band = IS_BAND_4; break;
	case 5:   band = BAND_5;  is_band = IS_BAND_5; break;
	case 8:   band = BAND_8;  is_band = IS_BAND_8; break;
	case 9:   band = BAND_9;  is_band = IS_BAND_9; break;
	case 10:  band = BAND_10;  is_band = IS_BAND_10; break;
	case 12:  band = BAND_12; is_band = IS_BAND_12; break;
	case 13:  band = BAND_13; is_band = IS_BAND_13; break;
	case 17:  band = BAND_17; is_band = IS_BAND_17; break;
	case 18:  band = BAND_18; is_band = IS_BAND_18; break;
	case 19:  band = BAND_19; is_band = IS_BAND_19; break;
	case 20:  band = BAND_20; is_band = IS_BAND_20; break;
	case 25:  band = BAND_25; is_band = IS_BAND_25; break;
	case 26:  band = BAND_26; is_band = IS_BAND_26; break;
	case 27:  band = BAND_27; is_band = IS_BAND_27; break;
	case 28:  band = BAND_28; is_band = IS_BAND_28; break;
	case 66:  band = BAND_66; is_band = IS_BAND_66; break;

	case 9001:  band = BAND_9001; is_band = IS_BAND_9001; break;
	case 9061:  band = BAND_9061; is_band = IS_BAND_9061; break;
	case 9064:  band = BAND_9064; is_band = IS_BAND_9064; break;

	default:
		// Error: unknown band
		sprintf(msg, "Unknown band %d", n);
		update_error(msg);
		return;
	}


	// Check if the desired band is already configured
	if (!AT_OK("AT+KBNDCFG?\r", LAIRD_DEF_TIMEOUT))
	{
		sprintf(msg, "AT+KBNDCFG? error");
		update_error(msg);
		return;
	}
	if (strstr((const char *)uart1_inbuf, is_band))
	{
		sprintf(msg, "Band %d already configured.", n);
		update_status(msg);
		return;  // band is already active
	}


    // Configure the new band
	if (!AT_OK(band, LAIRD_DEF_TIMEOUT))           // Select band
	{
		sprintf(msg, "AT+KBNDCFG error");
		update_error(msg);
		return;
	}

	sprintf(msg, "Band %d configured.", n);
	update_status(msg);

	// ASSERT:  The state machine will power down the modem which is required for a new band to take effect

}


#if 0
// For Reference
Laird Module 1:
AT+CGSN 354616091129549        15 digits   site_name is: 20 characters
       12345678901234567890
+CCID: 89332500000007613223    20 digits

Proposed syntax in site-name to keep it easy to type on a phone:
i4  = try internal SIM band 4
e2  = try external SIM band 2
The site name will be overwritten with the CCID, or other message to prevent infinite loops
when repeatedly pressing the "Cellular Test Routine" button
#endif



#if 0
void laird_min_from_power_on(void)
{

// Laird minimum commands from power on, provided module was previously configured and saved.
// Note, CGDCONT is saved in non-volatile memory, but KCNXCFG is not.  They must match.
// Will do both from power on, since they must match anyway and take the same run-time arguments (cid, apn).
// AT+CEREG?    wait for registration
// AT+CGDCONT=1,"IP","hologram"         1=cid, hologram=apn
// AT+KCNXCFG=1,"GPRS","hologram"       1=cid, hologram=apn
// AT+KTCPCFG=1,0,"52.24.219.107",2222  ip, port
// AT+KTCPSTART=1
#if 0
	{"0":"4C55CC24A0B8","1":"S","a":"","b":"","c":"4.4.5","d":"ZackLaird","e":1603099931,"f":94,"g":1621442622,"h":0,"i":1,"j":10,"k":8.00,"l":30.00,"m":3,"n":100,"o":23.00,"p":27.00}
#endif
}
#endif

void S2C_CLOSE_SOCKET(void)
{
	if (state_SOCKET_OPEN)
	{
		laird_exit_data_mode();
		laird_close_socket();
	}

	//laird_set_mux_to_ble();	  // release UART1 to be used by BLE
}


void S2C_POWERDOWN(void)
{
	// Turn off power to cell modem here
	s2c_powerdown_cell_hardware();
}

#if 0
static void test_entire_log(void)
{
	uint32_t start_rec;
	int batch_size;

	start_rec = 1;

	while (start_rec < LogTrack.log_counter)
	{
	  batch_size = s2c_prep_log_packet(uart1_outbuf, start_rec, 12);

	  send_packet("LOG");

	  start_rec += batch_size;

	  //if (start_rec > 50) break;
    }
}
#endif

static int prep_and_send_log_records(void)
{
	uint32_t addr;
	uint32_t rec_number;
	uint32_t batch_size;
	log_t    log_rec;
	int ack;

	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		char msg[100];
		sprintf(msg, "\nStart:  LogTrack.next_cellsend_addr = %lu", LogTrack.next_cellsend_addr);
		trace_AppendMsg(msg);
	}


	ack = ACK_TIMEOUT;
	addr = LogTrack.next_cellsend_addr;
	log_readstream_ptr(&log_rec, addr);
	rec_number = log_rec.log_num;

	while (rec_number <= LogTrack.log_counter)
	{

#ifdef TEST_RESEND

				if ((rec_number == 37) || (rec_number ==241))  // inject in the middle of a log, these should be ignored
				{
					test_resend_state = TEST_RESEND_ARMED;
					test_resend_ack = ACK_RESEND;
					test_resend_rec_number = rec_number - 4;  // test sending settings
				}
#endif
		batch_size = s2c_prep_log_packet(uart1_outbuf, 'L',rec_number, 12);

		ack = send_packet("LOG");
		if ( ack == ACK_TIMEOUT) break;  // give up on sending log records, but save LogTrack

		// advance to next record (if any)
		rec_number += batch_size;

		addr += (LOG_SIZE * batch_size);
		if (addr >= LOG_TOP) addr = LOG_BOTTOM;

		// Update the LogTrack to point to next record to send --- this should be done after ACK_RECEIVED...
		LogTrack.next_cellsend_addr = addr;
	}

	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		char msg[100];
		sprintf(msg, "\nEnd:  LogTrack.next_cellsend_addr = %lu", LogTrack.next_cellsend_addr);
		trace_AppendMsg(msg);
	}

	BSP_Log_Track_Write(1);  // update next_cellsend_addr and battery accumulator

	return ack;
}

static int prep_and_send_RESEND(int ackType)
{
	int ack;

	if (s2c_resend_record_number == 0)
	{
		s2c_prep_settings_packet(uart1_outbuf);
		return send_packet("SETTINGS");
	}

	if (ackType == ACK_RESEND)
	{
		s2c_prep_log_packet(uart1_outbuf, 'L',s2c_resend_record_number, 12);
		ack = send_packet("LOG");
	}
	else
	{
		s2c_prep_log_packet(uart1_outbuf, 'V',s2c_resend_record_number, 6);  // only send 6 to keep packet size reasonable
		ack = send_packet("VLOG");
	}

	return ack;
}



#if 0
static int old_protocol_send_log_records(void)
{
	log_t log_rec;
	float tmpfloat;

	uint32_t addr;
	int dec1, dec2;
	int ack;


	int nMessage = 8;
	int lenMess = 12;
	int NSamples;  // 8*12 = 96
	float distArray[96];
	float gainArray[96];
	float tempArray[96];
	float timeArray[96];
	char distList[128];

	int ts = 0;

	char bat[16]; // zg - not sure how big these need to be
	char vMo[16]; // zg - not sure how big these need to be


	memset(distArray,0,sizeof(distArray));
	memset(gainArray,0,sizeof(gainArray));
	memset(tempArray,0,sizeof(tempArray));
	memset(timeArray,0,sizeof(timeArray));
	memset(distList,0,sizeof(distList));


	tmpfloat = ltc2943_get_battery_percentage();
	convfloatstr_2dec(tmpfloat, &dec1, &dec2);
	sprintf(bat, "%d.%02d", dec1, dec2);

	// Vertical Mount
	tmpfloat = MainConfig.vertical_mount;
	if (MainConfig.units == UNITS_METRIC)
		tmpfloat *= 25.4;
	convfloatstr_2dec(tmpfloat, &dec1, &dec2);
	sprintf(vMo, "%d.%02d", dec1, dec2);



	NSamples = nMessage * lenMess;  // 8*12 = 96

#if 0
	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		char msg[100];
		sprintf(msg, "\nSEND_LOGS %d\n", NSamples);
		trace_AppendMsg(msg);
	}
#endif

	// zg - what if there are fewer samples in the log than 96 ?
	// zg - what if there are more than 96 unsent log records, due to ACK errors?
	addr = LogTrack.next_cellsend_addr;

	for (int i = 0; i < NSamples; i++)
	{
		log_readstream_ptr(&log_rec, addr);

		if (MainConfig.units == UNITS_ENGLISH)
		{
			tmpfloat = log_rec.temperature;
		}
		else
		{
			tmpfloat = (log_rec.temperature - 32) * 5 / 9;
		}
		tempArray[i] = tmpfloat;
		timeArray[i] = log_rec.datetime;
		distArray[i] = log_rec.distance;
		gainArray[i] = (255 - log_rec.gain);



		addr += LOG_SIZE;
		if (addr >= LOG_TOP)
		{
			addr = LOG_BOTTOM;
		}
	}


	int packetNum = 0;

	for (int j = 0; j < nMessage; j++)
	{
		packetNum++;

		memset(distList,0,sizeof(distList));

		for (int c = 0; c < lenMess; c++)
		{
			if (c < lenMess - 1)
			{
				append_to_array(distList, distArray[j * lenMess + c], 1, ADD_COMMA);
			}
			else
			{
				append_to_array(distList, distArray[j * lenMess + c], 1, NO_COMMA);
			}
		}
		ts = timeArray[lenMess * j];


		sprintf(uart1_outbuf, "{\"0\":\"%s\",\"1\":\"D\",\"2\":\"%d\",\"3\":[%s],\"4\":\"%d\",\"5\":\"%d\",\"6\":\"%d\",\"7\":\"%s\",\"8\":\"%s\"}",
			MainConfig.MAC,       //"0"
			ts,                   //"2"
			distList,             //"3"
			(int) tempArray[j],   //"4"
			(int) gainArray[j],   //"5"
			packetNum,            //"6"
			bat,                  //"7"
			vMo);                 //"8"

		ack = send_packet("DATA");
		if ( ack == ACK_TIMEOUT)
		{
			return ack;
		}
	}

	// Update the LogTrack --- this should be done after ACK_RECEIVED...
	LogTrack.next_cellsend_addr = addr;
	BSP_Log_Track_Write(0);

	return ack;

}
#endif

static int send_log_records(void)
{
	int ack;

	ack = prep_and_send_log_records();

	return ack;
}

static int prep_and_send_alert(log_t *pLogRec)
{

	// Determine the starting point such that the last record in the batch is the alert record
	uint32_t alert_rec_number;
	uint32_t start_rec_number;
	uint32_t end_rec_number;
	uint32_t batch_size;
	char packet_id;

	packet_id = 'L';

	if (s2c_current_alarm_state == ALARMLEVEL1) packet_id = 'E';
	if (s2c_current_alarm_state == ALARMLEVEL2) packet_id = 'F';
	if (s2c_current_alarm_state == NO_ALARM )   packet_id = 'G';

	alert_rec_number = pLogRec->log_num;
	if (alert_rec_number > 12)
	  start_rec_number = alert_rec_number - 12 + 1;
	else
	  start_rec_number = 1;

	batch_size = s2c_prep_log_packet(uart1_outbuf, packet_id, start_rec_number, 12);
	if (batch_size == 0) return ACK_TIMEOUT;  // cannot find a log record
	end_rec_number = start_rec_number + batch_size - 1;

	while (end_rec_number != alert_rec_number)
	{
		start_rec_number += batch_size;
		batch_size = s2c_prep_log_packet(uart1_outbuf, packet_id, start_rec_number, 12);
		if (batch_size == 0) return ACK_TIMEOUT;  // cannot find a log record
		end_rec_number = start_rec_number + batch_size - 1;
		led_heartbeat();
	}

	return send_packet("ALERT");

}


#if 0
static int old_protocol_send_alert(log_t *pLogRec)
{
	log_t tmplog;
	float tmpfloat;
	uint32_t tmpPtr;
	int dec1,dec2;
	int i;
	//int c;

	uint32_t lenMess;

	float distArray[96];
	float gainArray[96];
	float tempArray[96];
	float timeArray[96];
	char distList[128];   // this is a character array holding 12 sprintf(float) values, is it big enough ??

	char MAC_str[20];

	char battery_str[8];
	char verticalMount_str[8];
	char alertDistance_str[8];

	memset(distArray,0,sizeof(distArray));
	memset(gainArray,0,sizeof(gainArray));
	memset(tempArray,0,sizeof(tempArray));
	memset(timeArray,0,sizeof(timeArray));
	memset(distList,0,sizeof(distList));

	tmpfloat = ltc2943_get_battery_percentage();
	convfloatstr_2dec(tmpfloat, &dec1, &dec2);
	sprintf(battery_str, "%d.%02d", dec1, dec2);

	// Vertical Mount
	tmpfloat = MainConfig.vertical_mount;
	if (MainConfig.units == UNITS_METRIC)
		tmpfloat *= 25.4;
	convfloatstr_2dec(tmpfloat, &dec1, &dec2);
	sprintf(verticalMount_str, "%d.%02d", dec1, dec2);

	// Convert Alert Distance
	tmpfloat = pLogRec->distance;
	if (MainConfig.units == UNITS_METRIC)
		tmpfloat *= 25.4;
	convfloatstr_2dec(tmpfloat, &dec1, &dec2);
	sprintf(alertDistance_str, "%d.%02d", dec1, dec2);


	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		char msg[100];
		sprintf(msg, "\n\nSEND_ALERT Distance = %s", alertDistance_str);
		trace_AppendMsg(msg);
	}


	if (LogTrack.log_counter > 12)
	{
		lenMess = 12;
		tmpPtr = LogTrack.next_logrec_addr - (12UL * LOG_SIZE);
	}
	else
	{
		lenMess = LogTrack.log_counter;
		tmpPtr = LOG_BOTTOM;
	}



	//Send the most recent values (12 max); unavailable entries are sent as zeros
	for (i = 0; i < lenMess; i++)
	{

		log_readstream_ptr(&tmplog, tmpPtr);

		if (MainConfig.units == UNITS_ENGLISH)
		{
			tmpfloat = tmplog.temperature;
		}
		else
		{
			tmpfloat = (tmplog.temperature - 32) * 5 / 9;
		}
		tempArray[i] = tmpfloat;
		timeArray[i] = tmplog.datetime;
		distArray[i] = tmplog.distance;
		gainArray[i] = (255 - tmplog.gain);

		if (i < lenMess - 1)
		{
			append_to_array(distList, distArray[i], 1, ADD_COMMA);
		}
		else
		{
			append_to_array(distList, distArray[i], 1, NO_COMMA);
		}


		tmpPtr += LOG_SIZE;
		if (tmpPtr >= LOG_TOP)
		{
			tmpPtr = LOG_BOTTOM;
		}

	}


	strcpy(MAC_str, MainConfig.MAC);  // zg - to allow for debug override

	sprintf(uart1_outbuf, "{\"0\":\"%s\",\"1\":\"A\",\"2\":\"%d\",\"3\":[%s],\"4\":\"%d\",\"5\":\"%d\",\"9\":\"%s\",\"7\":\"%s\",\"8\":\"%s\"}",
		MAC_str,              //"0"
		(int) timeArray[0],   //"2"
		distList,             //"3"
		(int) tempArray[0],   //"4"
		(int) gainArray[0],   //"5"
		alertDistance_str,    //"9"
		battery_str,          //"7"
		verticalMount_str);   //"8"


	send_packet("ALERT");

	return ACK_RECEIVED;  // the old protocol does not support RESEND
}
#endif


void s2c_log_alert(char *desc, float distance, float level)
{
	char distance_str[30];
	char level_str[30];
	char msg[80];
	int dec1, dec2;

	convfloatstr_2dec(distance, &dec1, &dec2);
	sprintf(distance_str,"%d.%02d", dec1, dec2);

	convfloatstr_2dec(level, &dec1, &dec2);
	sprintf(level_str,"%d.%02d", dec1, dec2);

	sprintf(msg,"%s Distance: %s  Level: %s", desc, distance_str, level_str);
	AppendStatusMessage(msg);
}


void s2c_check_for_alert(float distance)
{

	float level;

    level = MainConfig.vertical_mount - distance;

    // ASSERT: MainConfig.AlertLevel1 is always less than or equal to MainConfig.AlertLevel2

	if ((MainConfig.AlertLevel1 != 0) && (level < MainConfig.AlertLevel1))
	{
		if (s2c_current_alarm_state != NO_ALARM)
		{
			s2c_log_alert("Alert Enter: NO_ALARM", distance, level);
		}
		else
		{
			s2c_log_alert("Alert RemainAt: NO_ALARM", distance, level);
		}
		s2c_current_alarm_state = NO_ALARM;
	}
	else if ((MainConfig.AlertLevel2 != 0) && (level >= MainConfig.AlertLevel2))
	{
		if (s2c_current_alarm_state != ALARMLEVEL2)
		{
			s2c_log_alert("Alert Enter: ALARMLEVEL2", distance, level);
		}
		else
		{
			s2c_log_alert("Alert RemainAt: ALARMLEVEL2", distance, level);
		}
		s2c_current_alarm_state = ALARMLEVEL2;
	}
	else if ((MainConfig.AlertLevel1 != 0) && (level >= MainConfig.AlertLevel1))
	{
		if (s2c_current_alarm_state != ALARMLEVEL1)
		{
			s2c_log_alert("Alert Enter: ALARMLEVEL1", distance, level);
		}
		else
		{
			s2c_log_alert("Alert RemainAt: ALARMLEVEL1", distance, level);
		}
		s2c_current_alarm_state = ALARMLEVEL1;
	}
	else
	{
		if (s2c_current_alarm_state != NO_ALARM)
		{
			s2c_log_alert("Alert Enter: NO_ALARM", distance, level);
		}
		else
		{
			s2c_log_alert("Alert RemainAt: NO_ALARM", distance, level);
		}
		s2c_current_alarm_state = NO_ALARM;
	}
}


#if 0
static void manage_test_code(void)
{
	static int debug_cell_cycles = 3;

	if (debug_cell_cycles)
	{
		debug_cell_cycles--;
		if (debug_cell_cycles == 0)
		{
			//sd_CellCmdLogEnabled = 0;
			//sd_CellDataLogEnabled = 0;
			sd_TestCellData = 0;
			sd_TestCellAlerts = 0;
			trace_AppendMsg("\nTurned off cell data logging.  Debug cycles completed.");
		}
	}
}
#endif

static int network_connect(void)   // only called for Laird and Telit
{

	if (!connect_to_network())
	{
		// diagnostic message is updated by lower-level routines, do not override here
		return 0;
	}

	return 1;

}

static int open_socket(void)   // only called for Laird and Telit
{


	if (!open_socket_to_server())
	{
		update_error("SOCKET error");
		return 0;
	}

	return 1;

}

static int xbee3_network_connect(void)
{
	// xbee3 automatically connects to network at power-on when configured to do so
	// Need to verify what happens when it enters and exits command mode...
	//
	return 1;
}


static int xbee3_open_socket(void)
{
	// xbee3 automatically opens the socket at power-on when configured to do so
	// Need to verify what happens when it enters and exits command mode...
	//
	return 1;
}


static int NETWORK_CONNECT()
{
	int result;

	if (state_NETWORK_CONNECTED) return 1; // network is already connected

	switch (s2c_CellModemFound)
	{
	default:
	case S2C_NO_MODEM:    result = 0; break;
	case S2C_TELIT_MODEM: result = network_connect(); break;
	case S2C_LAIRD_MODEM: result = network_connect(); break;
	case S2C_XBEE3_MODEM: result = xbee3_network_connect(); break;
	}
	if (result)
	{
		state_NETWORK_CONNECTED = 1;
	}
	return result;

}


static int OPEN_SOCKET()
{
	int result;

	//return 0;  // Force an error for testing retries

	if (state_SOCKET_OPEN) return 1; // socket is already open

	switch (s2c_CellModemFound)
	{
	default:
	case S2C_NO_MODEM:    result = 0; break;
	case S2C_TELIT_MODEM: result = open_socket(); break;
	case S2C_LAIRD_MODEM: result = open_socket(); break;
	case S2C_XBEE3_MODEM: result = xbee3_open_socket(); break;
	}
	if (result)
	{
		state_SOCKET_OPEN = 1;
	}
	return result;
}


static void S2C_POWER_ON_PING(void)  // only called if cell is enabled
{

	if (sd_CellCmdLogEnabled)
	{
		trace_AppendMsg("\nPOWER_ON_PING\n");
	}

	if (s2c_CellModemFound == S2C_NO_MODEM)
	{
		if (sd_CellCmdLogEnabled)
		{
			trace_AppendMsg("\nCell modem not found.\n");
		}
		goto ErrExit;
	}

#if 0 // already done by state machine or we would not get here
	S2C_POWERUP();    // Cell is already powered on by this point, but must enforce the power-on delay so it can talk

	if (!NETWORK_CONNECT()) // connect to cell network
	{
	  sprintf(s2c_cell_status, "NETWORK_CONNECT failed.");

	  if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled || sd_TestCellAlerts)
	  {
		trace_AppendMsg("\nError:  NETWORK_CONNECT failed.\n");
	  }
	  goto ErrExit;
	}

	if (OPEN_SOCKET())
	{
		sprintf(s2c_cell_status, "Sending data to cloud.");
		prep_and_send_PING(POWER_ON_PING);
	}
#else
	sprintf(s2c_cell_status, "Sending data to cloud.");
	prep_and_send_PING(POWER_ON_PING);
#endif

ErrExit:

	trace_SaveBuffer();
}



#if 0
void S2C_CELLULAR_LOG(log_t *pLogRec)  // latch alerts with each logged data point
{
	int new_alert_type;

	//if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	//{
	//	trace_AppendMsg("\nEnter: S2C_CELLULAR_LOG\n");
	//}

    // Check for a possible new alert and save it
	new_alert_type = s2c_check_for_alert(pLogRec->distance);

	latch_new_alert(new_alert_type, pLogRec);

	//if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	//{
	//	trace_AppendMsg("\nExit: S2C_CELLULAR_LOG\n");
	//}

}
#endif


int  S2C_CELLULAR_MAKE_CALL(void)
{
	int make_cell_call = 0;
	uint32_t NewSamples;
	uint32_t seconds_since_power_on;
	uint32_t log_time_elapsed_sec;

	if (WifiIsAwake) return 0;   // do not make automatic calls when WiFi is active

	// Calculate the number of samples remaining to be sent
	NewSamples = (LogTrack.next_logrec_addr - LogTrack.next_cellsend_addr) >> 5;

	// Calculate the elapsed time it took to gather the samples that have yet to be sent
	log_time_elapsed_sec = NewSamples * MainConfig.log_interval;

	// Frequency:
	// For the first 12 hours from power on, make cell phone calls once per hour if there is one hours worth of data to send
	// After the first 12 hours have expired, send every 4 hours
	//
	// It may be nice to make routine cell calls during low-usage times, between 2 am - 4 am.
	//
#define SECONDS_IN_ONE_HOUR            4600UL  // ( 1 * 60 * 60)
#define SECONDS_IN_FOUR_HOURS         14400UL  // ( 4 * 60 * 60)
#define SECONDS_IN_SIX_HOURS          21600UL  // ( 6 * 60 * 60)
#define SECONDS_IN_TWELVE_HOURS       43200UL  // (12 * 60 * 60)
#define SECONDS_IN_TWENTY_FOUR_HOURS  86400UL  // (24 * 60 * 60)

	// Calculate how long iTracker power has been on  TODO should this reference when the test started ? Is that even knowable?
	seconds_since_power_on =  rtc_get_time() - TimeAtPowerOn;

    if (seconds_since_power_on <= SECONDS_IN_TWELVE_HOURS)
	{
    	if (log_time_elapsed_sec >= SECONDS_IN_ONE_HOUR) make_cell_call = 1;
	}
	else
	{
		if (log_time_elapsed_sec >= SECONDS_IN_FOUR_HOURS) make_cell_call = 1;
	}

    // Alerts will trigger a cell call regardless of how many logged data points are available
    //
    // Allowable Alert transitions are:
    // NO_ALARM to ALARMLEVEL1
    // NO_ALARM to ALARMLEVEL2
    // ALARMLEVEL1 to ALARMLEVEL2
    // ALARMLEVEL2 to NO_ALARM
    // ALARMLEVEL1 to NO_ALARM
    //
    // The transition ALARMLEVEL2 to ALARMLEVEL1 is ignored
	if (s2c_current_alarm_state != s2c_last_alarm_state_sent)
	{
		if ((s2c_last_alarm_state_sent == ALARMLEVEL2) && (s2c_current_alarm_state == ALARMLEVEL1))
		{
			// ignore this transition
		}
		else
		{
		  make_cell_call = 1;
		  switch (s2c_current_alarm_state)
		  {
		    case NO_ALARM:    AppendStatusMessage("Alert Trigger NO_ALARM"); break;
            case ALARMLEVEL1: AppendStatusMessage("Alert Trigger ALARMLEVEL1"); break;
            case ALARMLEVEL2: AppendStatusMessage("Alert Trigger ALARMLEVEL2"); break;
		    default:          AppendStatusMessage("Alert Trigger Unknown"); break;
		  }
		}
	}


	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		if (make_cell_call)
		{
		  trace_AppendMsg("\nS2C_CELLULAR_MAKE_CALL triggered\n");
		}
	}

	return make_cell_call;
}
#if 0
void S2C_TRY_BAND(int Band)
{

	int prev_pattern = led_set_pattern(LED_PATTERN_CELL_ON);
	int sec;

	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		char msg[80];
		sprintf(msg,"\n===== Enter: S2C_LAIRD_TRY_BAND %d\n", Band);
		trace_AppendMsg(msg);
	}

	if (state_SOCKET_OPEN)   // to allow testing of different bands from UI
	{
		laird_close_socket();  // exit socket back to command mode
	}

	//s2c_is_cell_powerup_complete(&sec);  // This will power on the cell modem if needed, but will not block

	if (sec)
	{
		char msg[50];  // max of 29 chars to App
		sprintf(msg, "Cell reboot in %d s", sec);
		update_error(msg);
		goto Exit;   // Try again later
	}

	if (laird_try_band(Band))
	{
		if (connect_to_network())
		{
			// Successfully connected to cell network, confirm band used, report signal quality ?
			//AT_OK("AT+KBND?\r" , LAIRD_DEF_TIMEOUT);    // Confirm band for diagnostics

			char msg[50];  // max of 29 chars to App
			sprintf(msg, "Band %d OK", Band);
			update_error(msg);
		}
		else
		{
			// diagnostic message is updated by lower-level routines, do not override here

		}

	}

	Exit:

	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		char msg[80];
		sprintf(msg,"\n===== Exit: S2C_LAIRD_TRY_BAND %d\n\n", Band);
		trace_AppendMsg(msg);
	}

	led_set_pattern(prev_pattern);
	trace_SaveBuffer();
}
#endif

static int S2C_SEND_DATA(void)
{
	int ack = ACK_RECEIVED;

	// By the time we get here, an alert may have already cleared, due to the cell connection taking minutes or hours.
	// In that case, it does not make sense to report a "latched" alert when the event is in the past.
	// The alert event will show up in the logged data reported to the Cloud.
	// The original event was "latched" by a trigger at a data point.  There is no way to "undo" a trigger, although it may time out.
	// Consequently, it makes sense to re-evaluate if an alert condition is still active and then either report the alert, or
	// simply send data.

	// Alerts take precedence over Logs
	if (s2c_current_alarm_state != s2c_last_alarm_state_sent)
	{
		ack = prep_and_send_alert(&CurrentLog);  // alerts are ALWAYS based on the most recently logged data point

		s2c_last_alarm_state_sent = s2c_current_alarm_state;  // alert is considered sent, even if no ACK

		while ((ack == ACK_RESEND) || (ack == ACK_RESEND_VERBOSE))
		{
			ack = prep_and_send_RESEND(ack);
		}
	}


	if (ack == ACK_RECEIVED)
	{
#ifdef TEST_RESEND
		static int header_count = 0;
		header_count++;
		if (header_count == 1)  // inject on the 1st header, ask for verbose
		{
			test_resend_state = TEST_RESEND_ARMED;
			test_resend_ack = ACK_RESEND_VERBOSE;
			test_resend_rec_number = 3;  // test sending settings
		}

		if (header_count == 3)  // inject on the 3rd header, ask for normal
		{
			test_resend_state = TEST_RESEND_ARMED;
			test_resend_ack = ACK_RESEND;
			test_resend_rec_number = 101;  // test sending settings
		}
#endif
		ack = prep_and_send_PING(DATA_PING);  // report in
		while ((ack == ACK_RESEND) || (ack == ACK_RESEND_VERBOSE))
		{
			ack = prep_and_send_RESEND(ack);
		}

		if (ack == ACK_RECEIVED)
		{
			ack = send_log_records();  // this updates last_cellsend_addr; does not respond to resend requests
		}
	}

	return ack;

}


int S2C_CELLULAR_SEND(void)  // only called if cell is enabled.
{
	int ack;

	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		trace_AppendMsg("\nEnter: S2C_CELLULAR_SEND\n");
	}

	ack = S2C_SEND_DATA();

	trace_SaveBuffer();  // flush any trace data

	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		char msg[80];
		sprintf(msg,"\nExit: S2C_CELLULAR_SEND  %d\n", ack);
		trace_AppendMsg(msg);
	}
	return ack;  // returns ACK_RECEIVED if all data was sent;  ACK_TIMEOUT if something went wrong

}

void S2C_TRIGGER_CELLULAR_SEND(void)
{
	s2c_trigger_cellular_send();
}

static void telit_get_imei_and_utc(void)
{
	if (echo_off())
	{
		if (get_imei())
		{
			if (connect_to_network())
			{
				if (telit_sync_to_network_UTC_time(UTC_START_TIMER))
				{

				}
			}
		}
	}
}


static void xbee3_get_imei_and_utc(void)
{

	xbee3_enter_command_mode();

	xbee3_get_imei();

	xbee3_get_utc();

	xbee3_exit_command_mode();
}

static void get_imei_and_utc(void)
{
	// Establish a default value
	UTC_time_s = rtc_get_time();

	if (s2c_CellModemFound == S2C_LAIRD_MODEM)
	{
		get_imei();
	}
	else if (s2c_CellModemFound == S2C_TELIT_MODEM)
	{
	    telit_get_imei_and_utc();
	}
	else if (s2c_CellModemFound == S2C_XBEE3_MODEM)
	{
		if (0)  xbee3_get_imei_and_utc();   // use if (0) to avoid compiler warning, this function is not used...
	}

	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		char msg[100];
		sprintf(msg, "\nimei: %s, utc: %lu\n", s2c_imei, UTC_time_s);
		trace_AppendMsg(msg);
	}


}




#if 0
void S2C_POWER_ON_GET_IMEI_AND_UTC(void)  // This takes at least 10 seconds
{
	int prev_pattern = led_set_pattern(LED_PATTERN_CELL_ON);

	S2C_POWERUP();

	get_imei_and_utc();

	led_set_pattern(prev_pattern);

	trace_SaveBuffer();

}
#endif

#if 0
static int open_socket_to_server_aws(void)
{


/*	Websocket URL:
	wss://ts680724vf.execute-apus-west-2.amazonaws.com/prod/
*/
	// SOCKET DIAL (CONNECTION TO AWS SERVER)
	// zg - This establishes a socket connection --- the correct response code is "CONNECT"
	// SD=connId,txProt,rPort,IPaddr
	// connId = 1..max
	// txProt = 0 = TCP; 1 = UDP
	// rPort = remote host port number;  by default https uses port 443

	return AT_CONNECT("AT#SD=1,0,2222,\"18.236.236.185\"\r", SD_TIMEOUT);  // from ping


	return 0;
}
#endif




static int get_modem(void)
{
  int result = 0;

	if (!AT_OK("AT+CGMM\r", CGMM_TIMEOUT)) return 0;

	// AT+CGMM\r\r\nLE910-NA1\r\n\r\nOK\r\n

		 if(strstr((const char *)uart1_inbuf, "LE910-NA1"))  {	result = HW_TELIT_LAT3;	} //LAT3 => LE910-NA1;  This supports FWSWITCH but only between:  AT&T, T-Mobile
	else if(strstr((const char *)uart1_inbuf, "LE910-NAG"))  {	result = HW_TELIT_LAT1;	} //LAT1 => LE910-NAG;  AT&T only; does not support FWSWITCH
	else if(strstr((const char *)uart1_inbuf, "LE910-SV1"))  {	result = HW_TELIT_LVW3;	} //LVW3 => LE910-SV1;  Verizon only
	else if(strstr((const char *)uart1_inbuf, "LE910-SVG"))  {	result = HW_TELIT_LVW2;	} //LVW2 => LE910-SVG;  Verizon only
	else if(strstr((const char *)uart1_inbuf, "ME910C1-NA")) {	result = HW_TELIT_MNA1;	} //MNA1 => ME910C1-NA; AT+CGMM\r\r\nME910C1-NA\r\n\r\nOK\r\n    This supports FWSWITCH
	else if(strstr((const char *)uart1_inbuf, "HL7800"))     {	result = HW_LAIRD;	} //Laird

	return result;
}




/*
 *
 * To Detect the physical presence of a Telit cell modem, it appears that
 * the RI pin is pulled low via a resistor, which may be available even
 * when power is off.  Here are the test results of looking at
 * the RI and the CTS pins:
 *
 * With No Cell Hardware soldered to board (pins floating):
 *
 *   Before Power On:  RI=1, CTS=0
 *   After  Power On:  RI=1, CTS=0
 *   After 2 ms:       RI=1, CTS=0
 *   After 4 ms:       RI=1, CTS=0
 *   After 10 seconds: RI=1, CTS=0
 *
 * With Cell Hardware soldered to board:
 *
 *   Before Power On:  RI=0, CTS=0
 *   After  Power On:  RI=0, CTS=1
 *   After 2 ms:       RI=0, CTS=1
 *   After 4 ms:       RI=0, CTS=1
 *   After 10 seconds: RI=1, CTS=0
 *
 * Another possibility is to turn on power briefly,
 * verify RI=0 and CTS=1, then turn it off.
 *
 * Try the passive approach first - Did not work.  Use active approach.
 *
 */
#if 0
// original code
static int Gen4_is_telit_hardware_present(void)
{
	GPIO_PinState RI_pin_passive;
	GPIO_PinState CTS_pin_passive;
	GPIO_PinState RI_pin_active;
	GPIO_PinState CTS_pin_active;

	// Perform passive test on pins - no power
 	RI_pin_passive  = HAL_GPIO_ReadPin(GEN4_CELL_RI_Port, GEN4_CELL_RI_Pin);  // Only for Gen4
	CTS_pin_passive = HAL_GPIO_ReadPin(GEN45_CELL_CTS_Port, GEN45_CELL_CTS_Pin);  // Note: Gen4 had RTS,CTS swapped with respect to processor pins this is fixed

	// Power up the cell module
	s2c_CellModemFound = S2C_TELIT_MODEM;  // Set up for Telit.
	s2c_powerup_cell_hardware();  // this takes about 100 ms

	HAL_Delay(1); // short delay

	// Perform active test on pins - with power
	RI_pin_active  = HAL_GPIO_ReadPin(GEN4_CELL_RI_Port, GEN4_CELL_RI_Pin); // Only for Gen4
	CTS_pin_active = HAL_GPIO_ReadPin(GEN45_CELL_CTS_Port, GEN45_CELL_CTS_Pin);

	// If any pin changed, the telit cell modem hardware must be present
	if ((RI_pin_passive != RI_pin_active) || (CTS_pin_passive != CTS_pin_active))
	{
		return 1;
	}
	return 0;
}

#endif

#if 0

static int Gen45_is_CTS_hardware_present(void)
{

	GPIO_PinState CTS_pin_passive;
	GPIO_PinState CTS_pin_active;

	// Perform passive test on pins - no power to cell module

#if 1
	// debug - make sure pin is an input with a pullup
	{
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin  = GEN45_CELL_CTS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GEN45_CELL_CTS_Port, &GPIO_InitStruct);
	}
#endif

	//CTS_pin_passive = HAL_GPIO_ReadPin(GEN45_CELL_PWR_EN_Port, GEN45_CELL_PWR_EN_Pin); // debug - check power pin

	CTS_pin_passive = HAL_GPIO_ReadPin(GEN45_CELL_CTS_Port, GEN45_CELL_CTS_Pin);

	// Power up the cell module, but do not initialize the UART
	HAL_GPIO_WritePin(GEN45_CELL_PWR_EN_Port, GEN45_CELL_PWR_EN_Pin, 1);

	// Perform active test on pins - with power
	for (int i = 0; i < 1000; i++)
	{
	  CTS_pin_active = HAL_GPIO_ReadPin(GEN45_CELL_CTS_Port, GEN45_CELL_CTS_Pin);
	  if (CTS_pin_active)
	  {
		  break;
	  }
	  HAL_led_delay(1);
	}


	//CTS_pin_passive = HAL_GPIO_ReadPin(GEN45_CELL_PWR_EN_Port, GEN45_CELL_PWR_EN_Pin); // debug - check power pin

	HAL_led_delay(100);  // did not work for Laird
	//HAL_led_delay(500);  // did not work for Laird
	//HAL_led_delay(2000);  // did not work for Laird
	//HAL_led_delay(3000);  // did not work for Laird
	//HAL_led_delay(10000);  // did not work for Laird

	// Perform active test on pins - with power
	CTS_pin_active = HAL_GPIO_ReadPin(GEN45_CELL_CTS_Port, GEN45_CELL_CTS_Pin);

	// Turn power off to cell module
	HAL_GPIO_WritePin(GEN45_CELL_PWR_EN_Port, GEN45_CELL_PWR_EN_Pin, 0);

	//HAL_led_delay(100);

	// If the CTS pin changed, a cell modem hardware module must be present
	if (CTS_pin_passive != CTS_pin_active)
	{
		return 1;
	}
	return 0;
}
#endif





static int get_hw_modem_ctx_apn(void)
{
    static int just_once = 0;

    if (just_once) return hw_modem;
    just_once = 1;


    // Send a blank AT command with a short timeout to verify a modem is physically present
    //if (!AT_OK("AT\r", 1000)) return 0;
    if (!AT_OK("ATE0\r", 1000)) return 0;   // Better to make sure echo is OFF


    hw_modem = get_modem();

	// For modem=5 the default firmware image is ATT e.g. FWSWITCH 0 so set ctx to 1

	// 1: LAT3 => LE910-NA1;  This supports FWSWITCH but only between:  AT&T, T-Mobile
	// 2: LAT1 => LE910-NAG;  AT&T only; does not support FWSWITCH
	// 3: LVW3 => LE910-SV1;  supposedly Verizon only
	// 4: LVW2 => LE910-SVG;  supposedly Verizon only
	// 5: MNA1 => ME910C1-NA;  This supports FWSWITCH
	// 6:      => HLL7800;     Laird

	switch (hw_modem)
	{
		// see https://dataplans.digikey.com/documents/revx-digikey-faq-v3.pdf
		//
		//case 0: break;  // return error code here ??
		case HW_TELIT_LAT3:  hw_ctx = 1; strcpy(hw_apn, "10569.mcs"); break; // ATT
		case HW_TELIT_LAT1:  hw_ctx = 1; strcpy(hw_apn, "10569.mcs"); break; // ATT

		case HW_TELIT_LVW3:  hw_ctx = 3; strcpy(hw_apn, "hologram"); break;  // LVW3 LE910-SV1
		case HW_TELIT_LVW2:  hw_ctx = 1; strcpy(hw_apn, "hologram"); break;  // LVW2 LE910-SVG

		case HW_TELIT_MNA1:  hw_ctx = 1; strcpy(hw_apn, "hologram"); break;  // ATT using hologram

		case HW_LAIRD:       hw_ctx = 1; strcpy(hw_apn, "flolive.net"); break;  // ATT using hologram

        // Defaults for unknown modem models
		default: hw_ctx = 1; strcpy(hw_apn, "flolive.net");  break;  // LTE, other

	}
	return hw_modem;
}



static int telit_confirm_hardware(void)  // s2c_CellModemFound must be set before calling this
{
	int modem_model;

	modem_model = get_hw_modem_ctx_apn();

	if ((modem_model >= HW_TELIT_LAT3) && (modem_model <= HW_TELIT_MNA1))
	{
		if (sd_CellCmdLogEnabled)
		{

			// Log some extra information about cell configuration
			//AT_OK("AT+CSQ\r", TELIT_DEF_TIMEOUT);
			AT_OK("AT+CPIN?\r", TELIT_DEF_TIMEOUT);
			AT_OK("AT+COPS?\r", TELIT_DEF_TIMEOUT);
			AT_OK("AT#CCLK?\r", CCLK_TIMEOUT);
			AT_OK("AT#SGACT?\r", SGACTQUERY_TIMEOUT);  // Check Context Activation
			AT_OK("AT#BND?\r", TELIT_DEF_TIMEOUT);
			//AT_OK("AT+CGDCONT?\r", TELIT_DEF_TIMEOUT);
		}
		return 1;  // A Telit modem is confirmed
	}

	return 0;
}

void laird_report_radio_info(void)
{
	if (sd_CellCmdLogEnabled)
	{
		//TODO - restrict this to just the first time the iTracker is powered on, not each time a cell call is made ?

		AT_OK("AT+KBND?\r", LAIRD_DEF_TIMEOUT);  // This one shows the active LTE Bands (RAT and band)
		AT_OK("AT+COPS?\r", LAIRD_DEF_TIMEOUT);
		AT_OK("AT+CSQ\r", LAIRD_DEF_TIMEOUT);       // Signal Quality
		AT_OK("AT+CESQ\r", LAIRD_DEF_TIMEOUT);      // Extended Signal Quality
		AT_OK("AT+CEREG=3\r",   LAIRD_DEF_TIMEOUT); // Enable network registration, location information and EMM cause value
		AT_OK("AT+CEREG?\r",    CEREG_TIMEOUT);     // Query registration
		AT_OK("AT+CEREG=0\r",   LAIRD_DEF_TIMEOUT); // Disable URC
		AT_OK("AT+CCLK?\r", LAIRD_DEF_TIMEOUT);
	}
}

#if 1
void laird_query_config(void)
{
	if (sd_CellCmdLogEnabled)
	{

		trace_AppendMsg("\n===== LAIRD AS FOUND CONFIG \n");  // to help visually isolate info

		// Log some extra information about cell configuration
		//AT_OK("ATI9\r", LAIRD_DEF_TIMEOUT);  // The response to this is very long, about 400 characters.  To determine optimal buffer size, omit for now.
		//AT_OK("AT+COPS?\r", LAIRD_DEF_TIMEOUT);
		//AT_OK("AT+CCLK?\r", LAIRD_DEF_TIMEOUT);
		//AT_OK("AT+KBND?\r", LAIRD_DEF_TIMEOUT);  // This one shows the active LTE Bands (RAT and band)
		//AT_OK("AT+CSQ\r", LAIRD_DEF_TIMEOUT);  // Signal Quality
		//AT_OK("AT+CESQ\r", LAIRD_DEF_TIMEOUT);  // Extended Signal Quality
		//AT_OK("AT+CPAS\r", LAIRD_DEF_TIMEOUT);  // Phone Activity Status.
		//AT_OK("AT+CPSMS?\r", LAIRD_DEF_TIMEOUT);  // Power Saving Mode Setting.



		AT_OK("AT+CIMI\r", 2000);  // request IMSI number
		AT_OK("AT+CCID\r", 2000);  // request SIM Card Identification,  +CCID:  <ICCID>   or +CME_ERROR:  <error>  or just ERROR  or +CCID: 89332500000007613223    20 digits

		AT_OK("AT+KSIMDET?\r", LAIRD_DEF_TIMEOUT);// Request SIM configuration. Expected response for Eastech SIM is +KSIMDET: 1
		AT_OK("AT+KSIMSEL?\r", LAIRD_DEF_TIMEOUT);// Request SIM configuration. Expected response for Eastech SIM is +KSIMSEL: 9

		//AT_OK("AT+KSIMSEL=4\r", LAIRD_DEF_TIMEOUT);// Request SIM presence status configuration. Expected response for Eastech SIM is +KSIMSEL: 4,0,,1

		//AT_OK("AT+KSREP?\r", LAIRD_DEF_TIMEOUT);   // Request SIM status (was SIM card found):  Expected response is +KSREP: 0,0
		AT_OK("AT+CGSN\r", LAIRD_DEF_TIMEOUT);     // request IMEI

		AT_OK("AT+CPIN?\r", LAIRD_DEF_TIMEOUT);   // request password.  Expected response for Eastech is READY

		AT_OK("AT+KCARRIERCFG?\r", LAIRD_DEF_TIMEOUT);
		AT_OK("AT+KBNDCFG?\r", LAIRD_DEF_TIMEOUT);
		AT_OK("AT+CGDCONT?\r", LAIRD_DEF_TIMEOUT);
		//AT_OK("AT+KCNXCFG?\r", LAIRD_DEF_TIMEOUT);
		//AT_OK("AT+KTCPCFG?\r", LAIRD_DEF_TIMEOUT);
		//AT_OK("AT+KURCCFG?\r", LAIRD_DEF_TIMEOUT);

		trace_AppendMsg("\n===== END AS FOUND CONFIG \n");  // to help visually isolate info

	}
}
#endif

#if 0 // work in progress
static int laird_confirm_SIM_detect(void)
{
	// Confirm SIM detect is active
	AT_OK("AT+KSIMDET?\r", LAIRD_DEF_TIMEOUT);// Read SIM presence status. Expected response for Eastech SIM is +KSIMDET: 1

	// Verify the response  \r\n+KSIMDET: 1\r\n\r\n
	if(strstr((const char *)uart1_inbuf, "+KSIMDET: 1")) return 1;

	update_error("SIM detect not on");

	return 0;
}
#endif

#if 0
static int laird_confirm_SIM_configuration(void)
{

	// Confirm SIM presence is detected.
	AT_OK("AT+KSIMSEL=4\r", LAIRD_DEF_TIMEOUT);// Read SIM presence status. Expected response for Eastech SIM is +KSIMSEL: 4,0,,1

	//                                1         2         3
	//                       0 123456789012345678901234567890
	// Verify the response  \r\n+KSIMSEL: 4,0,,1\r\n\r\n
	if(strstr((const char *)uart1_inbuf, "+KSIMSEL: 4,0,,1")) return 1;

    //AppendStatusMessage("SIM not detected.");
	update_error("SIM not detected.");

	return 0;
}

static int laird_confirm_SIM_status(void)
{
	int retry = 2;

	// Confirm SIM is actually present and active
	// Expected response for Eastech SIM is +KSREP: 0,0
	//                                  1         2         3
	//                       0 123456789012345678901234567890
	// Verify the response  \r\n+KSREP: 0,0\r\n\r\n
	//
	// +KSREP: <act>,<stat>,<timeout>
	// <act>
	//   0 disabled URC
	//   1 enabled URC
	// <stat>
	//   0  Module is ready
	//   1  Module is waiting for access code
	//   2  SIM card not present
	//   3  Module in "SIMlock" state
	//   4  Unrecoverable error
	//   5  Unknown state
	//   6  Inactive SIM

	Retry:

	AT_OK("AT+KSREP?\r", LAIRD_DEF_TIMEOUT);// Request SIM status. Expected response for Eastech SIM is +KSREP: 0,0
	if(strstr((const char *)uart1_inbuf, "+KSREP: 0,0")) return 1;

	// Attempt repair by forcing SIM card selection

    if (retry)
    {
	  retry--;

	  // Report the configuration error and try and correct it
	  AppendStatusMessage("SIM status error KSREP, attempting repair.");

	  // Try to correct the configuration.  Sometimes the Laird gets into a weird state and even if it thinks it has the SIM selected, it does not.
	  AT_OK("AT+KSREP=0\r", LAIRD_DEF_TIMEOUT); // Disable URC at startup
	  AT_OK("AT+KSIMSEL=9\r", LAIRD_DEF_TIMEOUT); // Force SIM card selection

	  HAL_led_delay(1000);  // give the modem a little time

	  goto Retry;

    }


	// Report SIM error for diagnostics
	char msg[25];

	sprintf(msg, "SIM error %c", uart1_inbuf[12]);

	update_error(msg);

	return 0;
}
#endif

#if 0
static void force_laird_misconfiguration(void)
{
	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		trace_AppendMsg("======Start of Force Laird Misconfiguration");
	}
	AT_OK("AT+KSIMSEL=0\r", LAIRD_DEF_TIMEOUT);
	//AT_OK("AT+KSIMSEL=9\r", LAIRD_DEF_TIMEOUT);
	AT_OK("AT+KSIMDET=0\r", LAIRD_DEF_TIMEOUT);
	AT_OK("AT+KSREP=1\r", LAIRD_DEF_TIMEOUT);
	HAL_led_delay(1000);  // give the modem a little time
	if (sd_CellCmdLogEnabled || sd_CellDataLogEnabled)
	{
		trace_AppendMsg("======End of Force Laird Misconfiguration");
	}
}
#endif

static int laird_confirm_hardware(void)
{
	int modem_model;

	modem_model = get_hw_modem_ctx_apn();

	if (modem_model==HW_LAIRD)
	{
		//if (laird_confirm_SIM_configuration())
		{
			//if (laird_confirm_SIM_status())
			{
				laird_query_config();
				return 1;  // A laird modem with active SIM is confirmed
			}
		}
	}

	return 0;
}

static int xbee3_confirm_hardware(void)
{


	// Note the Xbee3 will respond to the "+++" right away,
	// but has not yet opened a socket connection which can take around 30 seconds.
	// So, for our purposes here, no additional waiting is needed
	// and the power is already on

	if (xbee3_enter_command_mode())
	{
		// An Xbee3 is confirmed.  The uart received: "OK\r"
		xbee3_exit_command_mode();

		// Assign some reasonable defaults
		hw_modem = HW_XBEE3;
		hw_ctx = 1;
		strcpy(hw_apn, "hologram");

		return 1;
	}
	return 0;  // no Xbee3 detected

}

#if 0
static int is_xbee3_hardware_present(void)
{

	// Note the Xbee3 will respond to the "+++" right away,
	// but has not yet opened a socket connection which can take around 30 seconds.
	// So, for our purposes here, no additional waiting is needed
	// and the power is already on

	s2c_CellModemFound = S2C_XBEE3_MODEM;  // Set up uart response parsing for Xbee3.
	s2c_powerup_cell_hardware();  // this takes about 100 ms

	if (xbee3_enter_command_mode())
	{
		// An Xbee3 is confirmed.  The uart received: "OK\r"
		xbee3_exit_command_mode();

		return 1;
	}
	return 0;  // no Xbee3 detected

}


static void Gen4_DetectCellModemHardware(void)
{
	int possible_telit_detected = 0;

	// First, perform a hardware detect as before.
	// This will turn on cell power and leave it on.
	// However, the results are not definitive.  Confirmation is required.
	if (Gen45_is_CTS_hardware_present())
	{
		possible_telit_detected = 1; // Possible telit cell modem detected, leave cell modem powered on
	}

	// The hardware detection of the Telit is not 100% accurate when an Xbee is present.

	// Confirm if an Xbee3 is present - this takes about 3 seconds, does not appear to disturb the telit.
	if (is_xbee3_hardware_present())
	{
		//s2c_CellModemFound = S2C_XBEE3_MODEM;  // already set
		//set_apn_and_ctx
		return;
	}

	// Confirm if a telit is actually present - this takes about 12 seconds.  Only try if the hardware detection passed.
	if (possible_telit_detected)
	{
		s2c_CellModemFound = S2C_TELIT_MODEM;  // set to Telit again after xbee3 failed
		if (telit_confirm_hardware())
		{
			return;
		}
	}

	// No cell modem detected or confirmed

	s2c_CellModemFound = S2C_NO_MODEM; // no cell modem detected

	s2c_powerdown_cell_hardware();

	update_error("No Telit or XBee3 cell modem found.");
}


static void Gen5_DetectCellModemHardware(void)
{
	s2c_CellModemFound = S2C_LAIRD_MODEM;  // assume it is a Laird to allow correct ISR & power-on sequence

	s2c_powerup_cell_hardware();  // this takes about 100 ms

	// Confirm if a Laird is actually present - this takes about ?? seconds.

	if (laird_confirm_hardware())
	{
		return;
	}

	// No Laird cell modem detected or confirmed

	s2c_CellModemFound = S2C_NO_MODEM; // no cell modem detected

	s2c_powerdown_cell_hardware();

	update_error("No Laird cell modem found.");
}




void s2c_DetectCellModemHardware(void)
{
	if (isGen4())
	{
		Gen4_DetectCellModemHardware();
	}
	if (isGen5())
	{
		Gen5_DetectCellModemHardware();
	}
}
#endif



int s2c_ConfirmCellModemHardware(void)  // This takes at least 10 seconds for laird and telit.  Returns 1 if complete, 0 if not complete and additional calls are needed
{
  static int s2c_ConfirmCellModem = 1;  // used to confirm cell modem at power on
  int result = 0;
  int CellModemConfigured = GetCellModule();

  // Power on delay is complete

  // exit if hw was previously confirmed

  if (s2c_ConfirmCellModem == 0) return 1;  // the one-time confirmation process is complete

  // Try and confirm the modem is really there and the SIM is configured, present, and active


  switch (CellModemConfigured)
  {
    case S2C_TELIT_MODEM:     	result = telit_confirm_hardware();     	break;
    case S2C_XBEE3_MODEM:       result = xbee3_confirm_hardware();    	break;
    case S2C_LAIRD_MODEM:    	result = laird_confirm_hardware();    	break;
    default:                 	result = 0;    	break;
  }

  if (result) get_imei_and_utc();   // The utc comes from the network AFTER registration


  // If no actual cell modem is confirmed, report discrepancy
  if (result == 0)
  {
	  char msg[80];
	  char cell[10];
	  switch (CellModemConfigured)
	  {
	  case CELL_MOD_NONE   : strcpy(cell,"NONE"); break;
	  case CELL_MOD_TELIT  : strcpy(cell,"TELIT"); break;
	  case CELL_MOD_XBEE3  : strcpy(cell,"XBEE3"); break;
	  case CELL_MOD_LAIRD  : strcpy(cell,"LAIRD"); break;
	  }
	  sprintf(msg, "Configured cell modem %s not found. Cell is disabled.", cell);
	  AppendStatusMessage(msg);

	  ClearCellEnabled();

	  s2c_CellModemFound = S2C_NO_MODEM;
  }
  else
  {
	  // Apparently, can get here with s2c_CellModemFound = S2C_NO_MODEM and result non zero
	  // report that the configured cell modem was found and if it is enabled or not
		switch (s2c_CellModemFound)
		{
		  case S2C_NO_MODEM:    AppendStatusMessage("No cell modem found.");break;
		  case S2C_TELIT_MODEM: AppendStatusMessage("Telit cell modem found."); break;
		  case S2C_XBEE3_MODEM: AppendStatusMessage("Xbee cell modem found."); break;
		  case S2C_LAIRD_MODEM: AppendStatusMessage("Laird cell modem found."); break;
		}

		if (IsCellEnabled())
		{
			AppendStatusMessage("Cell enabled.");
		}
		else
		{
			AppendStatusMessage("Cell disabled.");
		}
  }

  s2c_ConfirmCellModem = 0;  // the confirmation process is complete

  if (sd_CellCmdLogEnabled)
  {
	  char msg[80];
	  int seconds;
	  seconds = s2c_get_power_on_duration_sec();
	  sprintf(msg,"\n===== Cell confirmation complete. Cell modem on for %d s.\n", seconds);
	  trace_AppendMsg(msg);
  }

  // Cause any trace messages to be written to the SD
  trace_SaveBuffer();

  return 1;
}

#if 0
// Prepares a message of the form:
// Poweron in n seconds.
static void countdown_message(const char * label, int seconds)
{
	char msg[80];
	sprintf("%s in %d seconds.", label, seconds);

}
#endif




static int ok_to_turn_on(void)
{

	uint32_t current_time;
	uint32_t sec_off;
	int seconds_remaining;

	current_time = rtc_get_time();
	if (current_time >= cell_powerdown_time)
	{
	  sec_off = current_time - cell_powerdown_time;
	}
	else
	{
		sec_off = 15;
	}

	if (sec_off >= 15)
	{
		sprintf(s2c_cell_status, "Turning cell power on.");
		return 1;
	}

	// Prepare countdown message for App
	seconds_remaining = 15 - sec_off;

	sprintf(s2c_cell_status, "Retry in %d seconds.", seconds_remaining);

	return 0;

}


static void go_to_next_state(int new_state)
{
	char msg[80];
	sprintf(msg,"Cell next state: %d ==> %d", cell_state, new_state);
	AppendStatusMessage(msg);
	cell_state = new_state;
}


static int check_for_trigger_on(void)
{
	int result = MAIN_LOOP_OK_TO_SLEEP;

	// check if cell is configured, if not, stay in this state
	int CellModemConfigured = GetCellModule();

	if (CellModemConfigured == S2C_NO_MODEM)
	{
		static int just_once = 1;
		if (just_once)
		{
		  AppendStatusMessage("No cell modem configured.");
		  just_once = 0;
		}

		sprintf(s2c_cell_status, "No cell modem configured.");

		return result;  // stay in this state
	}

	// A cell modem is configured, check if it is enabled

	// check if cell is enabled for use, if not, stay in this state
	if (!IsCellEnabled())
	{
		sprintf(s2c_cell_status, "Cell is disabled.");
		return result;  // stay in this state
	}

	// check if cell has been triggered to turn on
	if (cell_trigger)
	{
		if (ok_to_turn_on())  // Wait N seconds beyond last power-down before turning cell modem on again
		{
			go_to_next_state( CELL_WAIT_FOR_WIFI_HOLDOFF );
		}

		result = MAIN_LOOP_DO_NOT_SLEEP;
	}
	else
	{
		sprintf(s2c_cell_status, "Cell off. %s", CellConfig.status);  // this overwrites battery too low messages
	}
	return result;
}

static int wait_for_wifi_holdoff(void)
{
	if (wifi_delay_timer_ms == 0) // Wait N seconds beyond WiFi power up before turning on cell modem
	{
		go_to_next_state( CELL_WAITING_FOR_WARMUP );
	}
	else
	{
		int seconds_remaining;
		seconds_remaining = wifi_delay_timer_ms / 1000;
		sprintf(s2c_cell_status, "Holdoff for %d seconds.", seconds_remaining);
	}

	return MAIN_LOOP_DO_NOT_SLEEP;
}

static int waiting_for_warmup(void)
{
	int CellModemConfigured = GetCellModule();

	s2c_CellModemFound = CellModemConfigured;  // establish the modem type for all sub-systems

	// Turn cell power on, if needed.  Also confirms the power-on time has completed.
	if (s2c_is_cell_powerup_complete())  // This requires global s2c_CellModemFound be set
	{
		// Power on delay is complete
		go_to_next_state( CELL_WAITING_FOR_CONFIRM_HW );
	}

	return MAIN_LOOP_DO_NOT_SLEEP;
}

static int confirm_cell_hw(void)
{

	if (s2c_ConfirmCellModemHardware())   // keep checking until power on and one-time hw confirmation process is complete
	{
		if ((cell_trigger == TRIGGER_CONFIRM_HW) ||
			(cell_trigger == TRIGGER_FACTORY_CONFIG) ||
			(cell_trigger == TRIGGER_INTERNAL_SIM) ||
			(cell_trigger == TRIGGER_EXTERNAL_SIM)  ||
			(cell_trigger == TRIGGER_CELL_BAND) )

		{
		  go_to_next_state( CELL_WAITING_FOR_TRIGGER );  // always shut down the cell modem after the trigger
		}
		else
		{
		  go_to_next_state( CELL_WAITING_FOR_CONFIGURATION );
		}
	}

	return MAIN_LOOP_DO_NOT_SLEEP;
}


static void exit_state_machine(void)
{
	cell_trigger = TRIGGER_POWER_OFF;
	go_to_next_state( CELL_WAITING_FOR_TRIGGER );
}

static int confirm_config(void)
{
	// perform cell modem configuration that must be done at each power on

	// For example, there must be a valid CCID (SIM Card), otherwise no need to attempt registration
	//+CCID: 89332500000007613223    20 digits
	if (!AT_OK("AT+CCID\r", 2000))  // request SIM Card Identification,  +CCID:  <ICCID>   or +CME_ERROR:  <error>  or just ERROR  or +CCID: 89332500000007613223    20 digits
	{
		update_error("No SIM card");

		exit_state_machine();

		return MAIN_LOOP_DO_NOT_SLEEP;
	}

	// TODO parse SIM card id for App ?

	go_to_next_state( CELL_WAITING_FOR_REGISTRATION );

	return MAIN_LOOP_DO_NOT_SLEEP;

}

static void format_duration_msg(char *msg, uint32_t remaining_sec)
{
	if (remaining_sec < 61)
	{
		sprintf(msg,"%lu sec", remaining_sec);
	}
	else if (remaining_sec < 3601)
	{
		uint32_t remaining_min;
		remaining_min = remaining_sec / 60;
		sprintf(msg,"%lu min", remaining_min);
	}
	else
	{
		uint32_t remaining_hrs;
		remaining_hrs = remaining_sec / 3600;
		sprintf(msg,"%lu hrs", remaining_hrs);
	}
}

static int waiting_for_registration(void)
{
	uint32_t elapsed_time_sec;
	uint32_t holdoff_time_sec;
	uint32_t duration_of_cell_power_on_sec;
	static int retry_count = 1;


	// if cell on time exceeds X minutes during the registration process, power down the cell modem and try again later at next log data point event.
	duration_of_cell_power_on_sec = s2c_get_power_on_duration_sec();

    #define THIRTY_MINUTES_IN_SECONDS   1800  // (30 * 60) this consumes a LOT of battery especially in cases where the cell antenna is missing or damaged.  Not a good choice.
    #define FIVE_MINUTES_IN_SECONDS      300  // (5 * 60)
    #define FOUR_MINUTES_IN_SECONDS      240  // (4 * 60)

	if (duration_of_cell_power_on_sec > FOUR_MINUTES_IN_SECONDS)
	{
		char time_duration[20];

		format_duration_msg(time_duration, duration_of_cell_power_on_sec);

		sprintf(s2c_cell_status, "Tower timed out in %s.", time_duration);

		AppendStatusMessage(s2c_cell_status);

		last_failed_registration_time = rtc_get_time();

		retry_count = 1;

		// Retry tower at next data point.  Note:  a holdoff time will be enforced between failed 30 minute registrations.

		exit_state_machine();

		return MAIN_LOOP_DO_NOT_SLEEP;
	}

	if (retry_count > 1)
	{
        // enforce a holdoff time

		elapsed_time_sec = rtc_get_time() - last_attempted_registration_time;

		// Vary the holdoff time depending on how long the cell modem has been on

		if (duration_of_cell_power_on_sec < 60)
		{
			holdoff_time_sec = 5;
		}
		else if (duration_of_cell_power_on_sec < 120)
		{
			holdoff_time_sec = 10;
		}
		else
		{
			holdoff_time_sec = 15;
		}

		if (elapsed_time_sec < holdoff_time_sec) // don't check registration too often
		{
			char time_duration[20];
			// can potentially set a wakeup time here and let the unit go to sleep, need to test what happens to uart
			int seconds_remaining;

			seconds_remaining = holdoff_time_sec - elapsed_time_sec;

			//TODO - maybe specify the time at which it will be retried when in the 30 minute countdown state

			format_duration_msg(time_duration, seconds_remaining);

			sprintf(s2c_cell_status, "Tower retry (%d) in %s.", retry_count, time_duration);
			// Retry tower in %d seconds

			return MAIN_LOOP_DO_NOT_SLEEP;
		}
	}


	last_attempted_registration_time = rtc_get_time();

	//AppendStatusMessage("Checking cell registration");

	if (!NETWORK_CONNECT()) // connect to cell network
	{
		retry_count++;

		// stay in this state, but allow unit to go to sleep depending on how long cell has been powered on

		if (duration_of_cell_power_on_sec < 60)
		{
			return MAIN_LOOP_DO_NOT_SLEEP;
		}
		else if (duration_of_cell_power_on_sec < 180)
		{
			return MAIN_LOOP_DO_NOT_SLEEP;
		}
		else
		{
			s2c_Wake_Time = rtc_get_time() + 60;   // once per minute wakeups for X minutes

    		return MAIN_LOOP_OK_TO_SLEEP;  // TODO Maybe just leave BLE active as well ???
		}
	}

	retry_count = 1;

	// ASSERT: registration has succeeded, try and open a socket
	go_to_next_state( CELL_WAITING_FOR_OPEN_SOCKET );

	return MAIN_LOOP_DO_NOT_SLEEP;
}


static int waiting_for_open_socket(void)
{
	static int retries = 0;

	if (retries < 3)
	{
		if (cell_delay_timer_ms)    // don't attempt open socket too frequently
		{
			if ((cell_delay_timer_ms % 1000) == 0)  // don't update message too frequently
			{
				char msg[80];
				uint32_t remaining_sec;
				remaining_sec = cell_delay_timer_ms / 1000;

				sprintf(msg, "Cloud retry in %ld seconds.", remaining_sec);
				sprintf(s2c_cell_status, "%s", msg);

				AppendStatusMessage(msg);
			}

			return MAIN_LOOP_DO_NOT_SLEEP;
		}

		if (OPEN_SOCKET())
		{
			retries = 0;

			go_to_next_state( CELL_WAITING_FOR_TRIGGER );

			return MAIN_LOOP_DO_NOT_SLEEP;
		}
		else
		{
          char msg[80];

	      // ASSERT: registration has succeeded, but open socket has failed.
	      // stay in this state for 3 retries
		  retries++;

		  cell_delay_timer_ms = 5000;   // once every five seconds

		  sprintf(msg, "Cloud retry in 5 seconds.");
		  sprintf(s2c_cell_status, "%s", msg);

		   AppendStatusMessage(msg);

		  return MAIN_LOOP_DO_NOT_SLEEP;
		}
	}

	// ASSERT: 3 retries have failed, powerdown modem
	AppendStatusMessage("Open Socket Failed.");

	retries = 0;

	exit_state_machine();

	return MAIN_LOOP_DO_NOT_SLEEP;

}

static int waiting_for_trigger(void)
{
	// ASSERT:  there must be some prior cell_trigger, otherwise we would not be here

	if (cell_trigger == TRIGGER_POWER_ON_PING)
	{
		S2C_POWER_ON_PING();  // blocking
	}
	else if (cell_trigger == TRIGGER_CELLULAR_SEND)
	{
		S2C_CELLULAR_SEND();  // blocking
	}
	else if (cell_trigger == TRIGGER_FACTORY_CONFIG)  // {"comm":33}
	{
		laird_factory_config();
	}
	else if (cell_trigger == TRIGGER_INTERNAL_SIM)  // {"comm":35}
	{
		laird_internal_sim();
	}
	else if (cell_trigger == TRIGGER_EXTERNAL_SIM)  // {"comm":36}
	{
		laird_external_sim();
	}
	else if (cell_trigger == TRIGGER_CELL_BAND)  // {"cell_band":n}
	{
		laird_try_band(s2c_try_band);
	}
	else if (cell_trigger == TRIGGER_POWER_ON)
	{
		// TODO - set a flag that keeps the cell modem powered on

	}
	else if (cell_trigger == TRIGGER_POWER_OFF)
	{
		// TODO - clear a flag that keeps the cell modem powered on
	}

	// Close socket (if open)
	if (state_SOCKET_OPEN)
	{
		laird_exit_data_mode();
		laird_close_socket();

		// start a holdoff timer to prevent power off until the Cloud has time to receive the close socket indicators
		cell_delay_timer_ms = 15000;

		state_SOCKET_OPEN = 0;
	}

	go_to_next_state( CELL_WAITING_FOR_CLOSE_SOCKET );

	return MAIN_LOOP_DO_NOT_SLEEP;
}

static int waiting_for_close_socket(void)
{
	// Must Wait N seconds to allow the Cloud to get the close socket notification before powering off cell modem.
	// If power off right away, the Cloud does not get the close socket notification.

	if (cell_delay_timer_ms == 0)
	{
		  // TODO - add logic that allows cell modem to stay powered on?

		  S2C_POWERDOWN();

		  return MAIN_LOOP_OK_TO_SLEEP;
	}
	else
	{
		// Provide info to App
		int seconds_remaining;
		seconds_remaining = cell_delay_timer_ms / 1000;
		sprintf(s2c_cell_status, "Cell off in %d seconds.", seconds_remaining);
	}

	return MAIN_LOOP_DO_NOT_SLEEP;
}


int s2c_timeslice(void)
{
	int result = MAIN_LOOP_OK_TO_SLEEP;

	switch (cell_state)
	{
	  case CELL_OFF:                       result = check_for_trigger_on(); break;
	  case CELL_WAIT_FOR_WIFI_HOLDOFF:     result = wait_for_wifi_holdoff(); break;
	  case CELL_WAITING_FOR_WARMUP:        result = waiting_for_warmup(); break;
	  case CELL_WAITING_FOR_CONFIRM_HW:    result = confirm_cell_hw(); break;
	  case CELL_WAITING_FOR_CONFIGURATION: result = confirm_config(); break;
	  case CELL_WAITING_FOR_REGISTRATION:  result = waiting_for_registration(); break;
	  case CELL_WAITING_FOR_OPEN_SOCKET:   result = waiting_for_open_socket(); break;
	  case CELL_WAITING_FOR_TRIGGER:       result = waiting_for_trigger(); break;
	  case CELL_WAITING_FOR_CLOSE_SOCKET:  result = waiting_for_close_socket(); break;
	}

	return result;
}

int s2c_is_cell_active(void)
{
	return cell_trigger;
}



static void trace_trigger(char *desc)
{
	char msg[80];
	sprintf(msg,"Triggered: %s", desc);
	AppendStatusMessage(msg);
}

static void trace_ignore_trigger(char *desc, char *reason)
{
	char msg[80];
	sprintf(msg,"Trigger ignored: %s. %s.", desc, reason);
	AppendStatusMessage(msg);
}

static void cancel_tower_holdoff(void)
{
	last_successful_registration_time = 0;
	last_failed_registration_time = 0;
	last_attempted_registration_time = 0;
}


static int is_ok_to_trigger(char *desc)  // determines if it is ok to trigger with respect to failed registration attempts
{

    if (last_failed_registration_time > last_successful_registration_time)  // strictly, only one of these is non-zero.  Never are both non-zero at the same time.
    {
    	uint32_t now;
    	uint32_t seconds_with_power_off;
    	uint32_t enforced_off_time;
    	now = rtc_get_time();

    	if (now < last_failed_registration_time)
    	{
    		// problem: time appears to have gone backward, reset everything
    		cancel_tower_holdoff();

    		return 1;  // ok to trigger
    	}

    	seconds_with_power_off = now - last_failed_registration_time;

        //#define ENFORCED_OFF_TIME      600UL   // Ten minutes = ( 10 * 60) =  600 seconds
        //#define ENFORCED_OFF_TIME     3600UL   // 1 hrs = ( 1 * 60 * 60) =  3,600 seconds (for testing)
        //#define ENFORCED_OFF_TIME     7200UL   // 2 hrs = ( 2 * 60 * 60) =  7,200 seconds (for testing)
        //#define ENFORCED_OFF_TIME    14400UL   // 4 hrs = ( 4 * 60 * 60) = 14,400 seconds
        //#define ENFORCED_OFF_TIME    21600UL   // 6 hrs = ( 6 * 60 * 60) = 21,600 seconds
        //#define ENFORCED_OFF_TIME    28800UL   // 8 hrs = ( 8 * 60 * 60) = 28,800 seconds
        //#define ENFORCED_OFF_TIME    43200UL   //12 hrs = (12 * 60 * 60) = 43,200 seconds
        //#define ENFORCED_OFF_TIME    86400UL   //24 hrs = (24 * 60 * 60) = 86,400 seconds

    	// The amount of enforced off time depends on the application.
    	// For SewerWatch, which is intended to aggressively send alerts, a shorter interval makes sense.
    	// For iTracker, which is intended for logging, a longer interval makes sense.

    	//if (get_configuration() == CONFIGURED_AS_SEWERWATCH)
    	if (ProductType == PRODUCT_TYPE_SEWERWATCH)
    	{
    		enforced_off_time = 600UL;   // Ten minutes = ( 10 * 60) =  600 seconds
    	}
    	else
    	{
    		enforced_off_time = 28800UL;   // 8 hrs = ( 8 * 60 * 60) = 28,800 seconds
    	}

    	if (seconds_with_power_off < enforced_off_time)
    	{
			char time_duration[20];

        	uint32_t remaining_sec;
        	remaining_sec = enforced_off_time - seconds_with_power_off;

			format_duration_msg(time_duration, remaining_sec);

    		sprintf(s2c_cell_status, "Tower holdoff for %s.", time_duration);

    		trace_ignore_trigger(desc, s2c_cell_status);
    		return 0;  // not ok to trigger, cell has not been off long enough since last registration failure
    	}
    }
	return 1; // ok to trigger
}

static int attempt_to_trigger(char *desc, int which_trigger)
{
	// check battery capacity
	if (ltc2943_get_battery_percentage() < 5)
	{
		char msg[80];
		sprintf(msg, "Battery under 5%%, %s ignored.", desc);
		AppendStatusMessage(msg);
		trace_AppendMsg("\n");
		trace_AppendMsg(msg);
		trace_AppendMsg("\n");
		// Post 29 char msg for display in App
		            //12345678901234567890123456789
		sprintf(msg, "Battery under 5%% cannot run.");
		update_error(msg);
		sprintf(s2c_cell_status, msg);
		return 0;  // do not make cell calls when less than 5% battery capacity remaining
	}

	if (IsCellEnabled())
	{
		if (cell_trigger == TRIGGER_NONE)
		{
			if (is_ok_to_trigger(desc))
			{
				cell_trigger = which_trigger;
				trace_trigger(desc);
				return 1;
			}
		}
		else if (cell_trigger == which_trigger)
		{
			// already triggered
			trace_ignore_trigger(desc, "Already triggered.");
			return 1;
		}
		else
		{
			// A Previous trigger is in progress
			update_error("Cell busy.");
			trace_ignore_trigger(desc, "Cell busy.");
		}
	}
	else
	{
		update_error("Cell disabled.");
		trace_ignore_trigger(desc, "Cell disabled.");
	}
	return 0;
}


void s2c_trigger_confirm_cell_hw(void)
{
	char *pDesc = "confirm_cell_hw";

	attempt_to_trigger(pDesc, TRIGGER_CONFIRM_HW);

}

void s2c_trigger_power_on(void)
{
	char *pDesc = "power_on";

	attempt_to_trigger(pDesc, TRIGGER_POWER_ON);
}

void s2c_trigger_power_off(void)
{
	char *pDesc = "power_off";

	attempt_to_trigger(pDesc, TRIGGER_POWER_OFF);
}



void s2c_trigger_power_on_ping_from_ui(void)
{
	char *pDesc = "power_on_ping";

	cancel_tower_holdoff();

	attempt_to_trigger(pDesc, TRIGGER_POWER_ON_PING);

}



void s2c_trigger_cell_band_from_ui(uint16_t band)
{
	char *pDesc = "cell_band";

	s2c_try_band = band;  // save requested band in global

	cancel_tower_holdoff();

	attempt_to_trigger(pDesc, TRIGGER_CELL_BAND);

}

static void config_for_laird(void)
{

	SetCellEnabled();
	SetCellModule(CELL_MOD_LAIRD);
	saveMainConfig = 1;
	Reset_CellConfig();
	saveCellConfig = 1;
	s2c_CellModemFound = S2C_LAIRD_MODEM;

}

void s2c_trigger_factory_config_from_ui(void)
{
	char *pDesc = "factory_config";

	cancel_tower_holdoff();

	config_for_laird();

	attempt_to_trigger(pDesc, TRIGGER_FACTORY_CONFIG);
}

void s2c_trigger_internal_SIM_from_ui(void)  // 35 {"comm":35}  Configures internal SIM for Laird
{
	char *pDesc = "internal_sim";

	cancel_tower_holdoff();

	attempt_to_trigger(pDesc, TRIGGER_INTERNAL_SIM);
}

void s2c_trigger_external_SIM_from_ui(void)  // 36 {"comm":36}  Configures external SIM for Laird
{
	char *pDesc = "external_sim";

	cancel_tower_holdoff();

	attempt_to_trigger(pDesc, TRIGGER_EXTERNAL_SIM);
}


void s2c_trigger_cellular_send(void)
{
	char *pDesc = "cellular_send";

	attempt_to_trigger(pDesc, TRIGGER_CELLULAR_SEND);

}

int s2c_trigger_initial_power_on_ping(void)
{
	int result = 0;  // no ping was triggered, ok to sleep
	static int just_once = 1;
	char *pDesc = "initial_power_on_ping";
	if (just_once)
	{
		// Trigger a "power-on" ping
		// It is done to allow WiFi user to make changes to settings before they are sent to the cloud.
		// It also informs the Cloud of power on/off events

		if (attempt_to_trigger(pDesc, TRIGGER_POWER_ON_PING))
		{
			result = 1;  // a ping was triggered, not ok to sleep
		}

		just_once = 0;

	}
	return result;
}




#if 0
void s2c_trigger_cell_sleep(void)
{
	char *pDesc = "cell_sleep";
	// must perform the actual power down here because the main loop was exited
	if (cell_state == CELL_WAITING_FOR_TRIGGER)
	{
		trace_trigger(pDesc);
		S2C_POWERDOWN();
	}
	else
	{
		trace_ignore_trigger(pDesc);
	}
}
#endif



