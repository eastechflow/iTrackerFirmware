/*
 * pass_thru.c
 *
 *  Created on: Jul 16, 2021
 *    Contents: Performs uart1 <===> uart3 pass-thru and records data flow to the sd card
 */
#include "trace.h"

#include "usart.h"
#include "uart1.h"
#include "uart2.h"
#include "uart3.h"
#include "uart4.h"
#include "lpuart1.h"
#include "s2c.h"
#include "pass_thru.h"
#include "debug_helper.h"
#include "factory.h"
#include "sensor.h"
#include "Gen5_AD5270.h"
#include "esp32.h"


/*
 * Star Diagram of Flowscope.
 * Note that UART1 can be used in a pass-thru mode to UART3 or to LPUART1 to support
 * TI EVM software running on the PC.
 *
 *                  +-------------+                                      +---------+
 * Cell <= UART1 => |  iTracker   |<= UART3 => RS485 (100 feet) RS485 <= |  Level  |==> Transducer 3
 * WiFi <= UART2 +> |    Board    |                                      |  PGA460 |
 *                  | STM32L746   |                                      +---------+
 *    PC <= USB ==  |             |
 *                  |             |                                      +----------+
 *                  |             |<=LPUART1=>     CABLE        <=UARTx  | Velocity |==> Transducer 2
 *                  |             |                                      | TDC1000  |==> Transducer 1
 *                  |             |                                      | TDC7200  |
 *                  +-------------+                                      +----------+
 *
 *
 */


/* Daisy-Chain Diagram of Flowscope (obsolete --- use the "star" configuration)
 *                   +------------+                        +--------------+                                +---------+
 *   Cell <= UART1 =>|  iTracker  |<= UART3 =>  <= UARTx =>|   Velocity   |<= UARTx => RS485 <= PGA460 =>  |  Level  |
 *   WiFi <= UART2 =>|   Board    |                        |     Board    |                                |  Board  |
 *                   | STM32L746  |                        | STM32G071KB  |                                |  PGA460 |
 *                   |            |                        | TDC1000/7200 |                                |         |
 *                   +------------+                        +--------------+                                +---------+
 *                                                             XD1, XD2                                        XD3
 *
 * To use the TI PC Eval software, the iTracker or the Velocity board needs to operate in a "pass-thru" mode from UART to UART.
 */




#if 0
static void turn_on_power(void)
{
	// Eventually, an iTracker board can be hand-modified such that:
	// 1. PA5 is used to control the supply of 14.8v direct from the battery via Q3
	// 2. PA7 is used to control the "sensor power" to provide 7v to the Level board via U4.

	// For now, PA7 (VH_PWR_EN_Pin) is used to supply 10.5v to both the Velocity and Level at the same time.
	HAL_GPIO_WritePin(GEN45_VH_PWR_EN_Port,GEN45_VH_PWR_EN_Pin, GPIO_PIN_SET);
}

static void turn_off_power(void)
{
	HAL_GPIO_WritePin(GEN45_VH_PWR_EN_Port,GEN45_VH_PWR_EN_Pin, GPIO_PIN_RESET);
}

#endif





#define TX_DIRECTION  0
#define RX_DIRECTION  1

//#define LOG_TIMESTAMPS
//#define LOG_CHECKSUMS

static void trace_pga460_ch_tx(uint8_t ch)
{
	if (sd_EVM_460_Trace)
	{
		char msg[20];
		static uint8_t prev_ch_tx = 0;

		char timestamp[20];
#ifdef LOG_TIMESTAMPS
		sprintf(timestamp, "%lu ", HAL_GetTick());
#else
		timestamp[0]=0;
#endif

		if ((prev_ch_tx == 0x55) && (ch != 0x55))
		{
			// The Command Field contains the cmd and the addr
			uint8_t cmd;
			uint8_t addr;

			// How to decode the Command Field
			cmd = ch & 0xF;
			addr = ch >> 5;
			UNUSED(addr);

			sprintf(msg,"\n%s(85,%d,", timestamp, cmd);  // hex 0x55 is decimal 85, append decoding info, note that the pga460 addr is always zero in our case
			trace_AppendMsg(msg);

		}
		else if (ch != 0x55)
		{
			sprintf(msg,"%d,", ch);
			trace_AppendMsg(msg);
		}


		prev_ch_tx = ch;
	}
}

//#define TRACE_PGA460_RX_CHARS

#ifdef TRACE_PGA460_RX_CHARS
static void trace_pga460_ch_rx(uint8_t ch)
{
	if (sd_EVM_460_Trace)
	{
		char msg[20];
		static uint8_t prev_ch_rx = 0;

		char timestamp[20];
#ifdef LOG_TIMESTAMPS
		sprintf(timestamp, "%lu ", HAL_GetTick());
#else
		timestamp[0]=0;
#endif

		if ((prev_ch_rx == 0x40) && (ch != 0x40))
		{
           // Unclear what the 0x40 (64) is but is appears to always be the first RX byte

			sprintf(msg,"\n%s[64,%d,", timestamp, ch);  // use square bracket for RX
			trace_AppendMsg(msg);

		}
		else if (ch != 0x40)
		{
			sprintf(msg,"%d,", ch);
			trace_AppendMsg(msg);
		}

		prev_ch_rx = ch;
	}
}
#endif

static void trace_pga460_ch(uint8_t ch, int direction)
{

	// Record the RX character - not needed for recording script
	if (direction == TX_DIRECTION)
	{
		trace_pga460_ch_tx(ch);
	}
	else
	{
#ifdef TRACE_PGA460_RX_CHARS
		trace_pga460_ch_rx(ch);  // omit this line to skip recording RX bytes when creating a script
#endif
	}
}









/*
 * 7/30/2021
 *
 * LaunchPad EVM to Eastech Transducer via RS485
 *
 * The LaunchPad apparently "violates" the PGA460 protocol in some cases by appending an extraneous Checksum byte to response commands.
 * In a full-duplex communication system that does not appear to cause any problems.  However, in our half-duplex RS485
 * system, the RS485 bus experiences a bus contention when the Master sends the extra checksum byte and the PGA460
 * is responding with its first byte.
 *
 * To bypass this issue, a scan-and-forward approach is used to parse a command on-the-fly from the LaunchPad and then send
 * a corrected version to the PGA460 by dropping the "extra" CRC byte at the end.
 *
 * All data traffic can be serialized and saved on the SD card for diagnostic purposes.
 *
 * For simplicity, the PGA460 response will simply be passed along byte-by-byte.   The start of the next outbound cmd will frame the end of the response.
 *
 * Page 40 of the data sheet
 * 
 * 7.3.6.2.1.6.2 Response Operation (All Except Register Read)
 * The response operation of the PGA460-Q1 UART interface is initiated with the master sending a response
 * request. After the response request is received by the PGA460-Q1 device, the UART responds with the proper
 * data of the command being requested. In a response operation, the master does not generate a checksum Field,
 * rather it is generated by the PGA460-Q1.
 */



#define WAITING_FOR_SYNC  1
#define WAITING_FOR_CMD   2
#define GATHER_DATA_BYTES 3
#define GATHER_CHECKSUM   4
#define SENDING_TO_460    5
#define CMD_SENT          6

static  int state = WAITING_FOR_SYNC;


static int  idx;
static int  pga460_response;
static int  data_bytes;
static int  checksum_bytes;



static void reset_state_machine(void)
{
	state = WAITING_FOR_SYNC;
	idx = 0;
	data_bytes = 0;
	checksum_bytes = 0;

}



// Basically, if there are data bytes, the checksum is required.  See Table 3. UART Interface Command List page 38.

static int parse_cmd(uint8_t ch)  // returns 1 if cmd OK, 0 otherwise
{
	// The Command Field contains the cmd and the addr
	uint8_t cmd;
	uint8_t addr;

	// How to decode the Command Field
	cmd = ch & 0xF;
	addr = ch >> 5;
	UNUSED(addr);

	switch (cmd)
	{
	// See commands table on page 38 of data sheet
	case 0:   pga460_response = 0; data_bytes =  1; checksum_bytes = 1; break;
	case 1:   pga460_response = 0; data_bytes =  1; checksum_bytes = 1; break;
	case 2:   pga460_response = 0; data_bytes =  1; checksum_bytes = 1; break;
	case 3:   pga460_response = 0; data_bytes =  1; checksum_bytes = 1; break;
	case 4:   pga460_response = 0; data_bytes =  1; checksum_bytes = 1; break;

	case 5:   pga460_response = 1; data_bytes =  0; checksum_bytes = 0; break;
	case 6:   pga460_response = 1; data_bytes =  0; checksum_bytes = 0; break;
	case 7:   pga460_response = 1; data_bytes =  0; checksum_bytes = 0; break;
	case 8:   pga460_response = 1; data_bytes =  0; checksum_bytes = 0; break;
	case 9:   pga460_response = 1; data_bytes =  1; checksum_bytes = 1; break;  // The checksum is required for register read

	case 10:  pga460_response = 0; data_bytes =  2; checksum_bytes = 1; break;

	case 11:  pga460_response = 1; data_bytes =  0; checksum_bytes = 0; break;

	case 12:  pga460_response = 0; data_bytes = 43; checksum_bytes = 1; break;

	case 13:  pga460_response = 1; data_bytes =  0; checksum_bytes = 0; break;

	case 14:  pga460_response = 0; data_bytes =  7; checksum_bytes = 1; break;

	case 15:  pga460_response = 1; data_bytes =  0; checksum_bytes = 0; break;

	case 16:  pga460_response = 0; data_bytes = 32; checksum_bytes = 1; break;
	case 17:  pga460_response = 0; data_bytes =  1; checksum_bytes = 1; break;
	case 18:  pga460_response = 0; data_bytes =  1; checksum_bytes = 1; break;
	case 19:  pga460_response = 0; data_bytes =  1; checksum_bytes = 1; break;
	case 20:  pga460_response = 0; data_bytes =  1; checksum_bytes = 1; break;
	case 21:  pga460_response = 0; data_bytes =  1; checksum_bytes = 1; break;
	case 22:  pga460_response = 0; data_bytes =  2; checksum_bytes = 1; break;
	case 23:  pga460_response = 0; data_bytes = 43; checksum_bytes = 1; break;
	case 24:  pga460_response = 0; data_bytes =  7; checksum_bytes = 1; break;
	case 25:  pga460_response = 0; data_bytes = 32; checksum_bytes = 1; break;

	default:
        return 0;  // unknown cmd

		break;

	}

	return 1; // cmd parsed OK

}



static void forward_byte(uint8_t ch, uint8_t isCheckSum)
{
	idx++;  // keep track of how many bytes received (note: any "extra" CRC bytes will be skipped so this is not the number of TX bytes)

	if (isCheckSum)
	{
#ifdef LOG_CHECKSUMS
		trace_pga460_ch(ch, TX_DIRECTION);  // For recording scripts, omit checksum.  For comparing byte outputs, enable this line.
#endif
		if (sd_EVM_460_Trace)
		{
			char msg[20];
			sprintf(msg,");");  // End the syntax unit to allow easy editing later
			trace_AppendMsg(msg);
		}
	}
	else
	{
	  trace_pga460_ch(ch, TX_DIRECTION);  // do this first to provide a known time-sequence of bytes in the trace file, skip checksum bytes when morphing captured UI traces to firmware code
	}

#ifdef USE_UART1_AND_LPUART1
	// Send the extracted character to lpuart1
	lpuart1_tx_bytes(&ch, 1, 0);
#endif

#ifdef USE_UART4_AND_LPUART1
	// Send the extracted character to lpuart1
	lpuart1_tx_bytes(&ch, 1, 0);
#endif

#ifdef USE_UART1_AND_UART3
	// Send the extracted character to uart3
	uart3_tx_bytes(&ch, 1, 0);
#endif

#ifdef USE_UART4_AND_UART3
	// Send the extracted character to uart3
	uart3_tx_bytes(&ch, 1, 0);
#endif


	// ASSERT: The PGA460 might respond quickly and our RX interrupts might fire before we get here

	// bug hunt: wait briefly, then see if any characters arrive at uart3.  Remove this code for production
//	HAL_led_delay(0);
//	process_uart3();
}

static void report_protocol_error(uint8_t ch)
{
	if (sd_EVM_460_Trace)
	{
	// unexpected character
	char msg[20];
	sprintf(msg," \n?%x", ch);
	trace_AppendMsg(msg);
	}
}

static void report_discard_crc(uint8_t ch)
{
#if 0 // don't report these now that we know the UI is sending them
	if (sd_EVM_460_Trace)
	{
	char msg[20];
	sprintf(msg," \n!%x", ch);
	trace_AppendMsg(msg);
	}
#endif
}

static void scan_and_forward_cmd(uint8_t ch)
{

    // The state machine will forward each byte as needed
	switch (state)
	{
	case WAITING_FOR_SYNC:
		if (ch == 0x55)
		{
			forward_byte(ch,0);
			state = WAITING_FOR_CMD;
		}
		else
		{
			report_protocol_error(ch);
		}
		break;
	case WAITING_FOR_CMD:
		if (ch == 0x55) // ignore repeated 0x55 characters  -- unclear why the TI EVAL software sends them
		{
			report_protocol_error(ch);
			// Stay in the WAITING_FOR_CMD state
			break;
		}
		if (!parse_cmd(ch))
		{
			// unexpected cmd
			report_protocol_error(ch);
			reset_state_machine();
		}
		else
		{
		  forward_byte(ch,0);
		  if (data_bytes)
			state = GATHER_DATA_BYTES;
		  else
			state = GATHER_CHECKSUM;   // The LaunchPad is always sending a checksum
		}
		break;
	case GATHER_DATA_BYTES:
		forward_byte(ch,0);
		if (idx == (1+1+data_bytes))
		{
			state = GATHER_CHECKSUM;   // The LaunchPad is always sending a checksum
		}
		break;
	case GATHER_CHECKSUM:
        // Determine if the checksum byte should be forwarded
		if ((idx+1) == (1+1+data_bytes+checksum_bytes))
		{
			forward_byte(ch,1);
		}
		else
		{
			report_discard_crc(ch);
		}
		// Set up for next cmd
		reset_state_machine();
		break;
	}
}





#ifdef USE_UART1_AND_LPUART1
static void uart1_scan_and_forward_cmd_to_lpuart1(void)
{
	uint8_t ch;

	// ASSERT:  there is at least one rx char in the buffer

	// Extract the next character to be processed
	ch = uart1_inbuf[uart1_inbuf_tail];

	// Advance to next character in buffer (if any)
	uart1_inbuf_tail++;

	// Wrap (circular buffer)
	if (uart1_inbuf_tail == sizeof(uart1_inbuf))
		uart1_inbuf_tail = 0;

	scan_and_forward_cmd(ch);
}
#endif

#ifdef USE_UART4_AND_LPUART1
static void uart4_scan_and_forward_cmd_to_lpuart1(void)
{
	uint8_t ch;

	// ASSERT:  there is at least one rx char in the buffer

	// Extract the next character to be processed
	ch = uart4_inbuf[uart4_inbuf_tail];

	// Advance to next character in buffer (if any)
	uart4_inbuf_tail++;

	// Wrap (circular buffer)
	if (uart4_inbuf_tail == sizeof(uart4_inbuf))
		uart4_inbuf_tail = 0;

	scan_and_forward_cmd(ch);
}
#endif

#ifdef USE_UART1_AND_UART3
static void uart1_scan_and_forward_cmd_to_uart3(void)
{
	uint8_t ch;

	// ASSERT:  there is at least one rx char in the buffer

	// Extract the next character to be processed
	ch = uart1_inbuf[uart1_inbuf_tail];

	// Advance to next character in buffer (if any)
	uart1_inbuf_tail++;

	// Wrap (circular buffer)
	if (uart1_inbuf_tail == sizeof(uart1_inbuf))
		uart1_inbuf_tail = 0;

	scan_and_forward_cmd(ch);
}
#endif

#ifdef USE_UART4_AND_UART3
static void uart4_scan_and_forward_cmd_to_uart3(void)
{
	uint8_t ch;

	// ASSERT:  there is at least one rx char in the buffer

	// Extract the next character to be processed
	ch = uart4_inbuf[uart4_inbuf_tail];

	// Advance to next character in buffer (if any)
	uart4_inbuf_tail++;

	// Wrap (circular buffer)
	if (uart4_inbuf_tail == sizeof(uart4_inbuf))
		uart4_inbuf_tail = 0;

	scan_and_forward_cmd(ch);
}
#endif

#ifdef USE_UART1_AND_LPUART1
static void process_uart1_with_scan_to_lpuart1(void)
{
	while (uart1_inbuf_tail != uart1_inbuf_head)
	{
		uart1_scan_and_forward_cmd_to_lpuart1();
	}
}
#endif

#ifdef USE_UART4_AND_LPUART1
static void process_uart4_with_scan_to_lpuart1(void)
{
	while (uart4_inbuf_tail != uart4_inbuf_head)
	{
		uart4_scan_and_forward_cmd_to_lpuart1();
	}
}
#endif


#ifdef USE_UART1_AND_UART3
static void process_uart1_with_scan_to_uart3(void)
{
	while (uart1_inbuf_tail != uart1_inbuf_head)
	{
		uart1_scan_and_forward_cmd_to_uart3();
	}
}
#endif

#ifdef USE_UART4_AND_UART3
static void process_uart4_with_scan_to_uart3(void)
{
	while (uart4_inbuf_tail != uart4_inbuf_head)
	{
		uart4_scan_and_forward_cmd_to_uart3();
	}
}
#endif

static void process_uart1(void)
{
	uint8_t ch;

	while (uart1_getchar(&ch))
	{
		// Record the RX character
		trace_pga460_ch(ch, RX_DIRECTION);

		// Send the extracted character to lpuart1 (LEVEL - PGA460)
		lpuart1_putchar(&ch);
	}

}

static void process_lpuart1(void)
{
	uint8_t ch;

	while (lpuart1_getchar(&ch))
	{
		// Record the RX character
		trace_pga460_ch(ch, RX_DIRECTION);

		// Send the extracted character to uart1
		uart1_putchar(&ch);
	}

}

#ifdef USE_UART1_AND_LPUART1

static void process_lpuart1_to_uart1(void)
{
	uint8_t ch;
	while (lpuart1_inbuf_tail != lpuart1_inbuf_head)
	{
		// Extract the next character to be processed
		ch = lpuart1_inbuf[lpuart1_inbuf_tail];

		// Advance to next character in buffer (if any)
		lpuart1_inbuf_tail++;

		// Wrap (circular buffer)
		if (lpuart1_inbuf_tail == sizeof(lpuart1_inbuf))
			lpuart1_inbuf_tail = 0;

		// Record the RX character
		trace_pga460_ch(ch, RX_DIRECTION);

		// Send the extracted character to uart1
		uart1_tx_bytes(&ch, 1);
	}
}

#endif

#ifdef USE_UART4_AND_LPUART1

static void process_lpuart1_to_uart4(void)
{
	uint8_t ch;
	while (lpuart1_inbuf_tail != lpuart1_inbuf_head)
	{
		// Extract the next character to be processed
		ch = lpuart1_inbuf[lpuart1_inbuf_tail];

		// Advance to next character in buffer (if any)
		lpuart1_inbuf_tail++;

		// Wrap (circular buffer)
		if (lpuart1_inbuf_tail == sizeof(lpuart1_inbuf))
			lpuart1_inbuf_tail = 0;

		// Record the RX character
		trace_pga460_ch(ch, RX_DIRECTION);

		// Send the extracted character to uart4
		uart4_tx_bytes(&ch, 1, 0);
	}
}

#endif



#ifdef USE_UART1_AND_UART3

static void process_uart3_to_uart1(void)
{
	uint8_t ch;
	while (uart3_inbuf_tail != uart3_inbuf_head)
	{
		// Extract the next character to be processed
		ch = uart3_inbuf[uart3_inbuf_tail];

		// Advance to next character in buffer (if any)
		uart3_inbuf_tail++;

		// Wrap (circular buffer)
		if (uart3_inbuf_tail == sizeof(uart3_inbuf))
			uart3_inbuf_tail = 0;

		// Record the RX character
		uart3_trace_ch(ch);

		// Send the extracted character to uart1
		uart1_tx_bytes(&ch, 1);
	}
}

#endif

#ifdef USE_UART4_AND_UART3
static void process_uart3_to_uart4(void)
{
	uint8_t ch;
	while (uart3_inbuf_tail != uart3_inbuf_head)
	{
		// Extract the next character to be processed
		ch = uart3_inbuf[uart3_inbuf_tail];

		// Advance to next character in buffer (if any)
		uart3_inbuf_tail++;

		// Wrap (circular buffer)
		if (uart3_inbuf_tail == sizeof(uart3_inbuf))
			uart3_inbuf_tail = 0;

		// Record the RX character
		uart3_trace_ch(ch);

		// Send the extracted character to uart4
		uart4_tx_bytes(&ch, 1, 0);
	}
}
#endif


#if 0
static void echo_uart2_stuff(void)
{
	char ch;

	while (uart2_inbuf_tail != uart2_inbuf_head)
	{
		// Extract the next character to be processed
		ch = uart2_inbuf[uart2_inbuf_tail];

		// Advance to next character in buffer (if any)
		uart2_inbuf_tail++;

		// Wrap (circular buffer)
		if (uart2_inbuf_tail >= sizeof(uart2_inbuf))
			uart2_inbuf_tail = 0;

		// Process the extracted character
		//if (add_char_to_JSON_string(ch) == JSON_COMPLETE) break;  // leave any other received characters in the buffer for next pass

	}
}
#endif



void esp32_enter_bootload_mode(int BootPinState)
{
	if (isGen5())
	{

		//sd_UART_4SPY2 = 0;

	  uart2_XON_XOFF_enabled = 0; // Disable XON XOFF and GOT_HERE

	  // Enable the WiFi uart for ESP32
	  MX_USART2_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_NONE);
	  HAL_led_delay(5);  // give the uart a little time
	  //uart2_tx_string("Hello from UART2.  Set up as pass-thru to UART4\n");
	  uart2_flush_inbuffer();

	  MX_UART4_UART_Init(115200, UART_STOPBITS_1);
	  HAL_led_delay(5);  // give the uart a little time
	  //uart4_tx_string("Hello from UART4.  Set up as pass-thru to UART2 (WIFI)\n");
	  uart4_flush_inbuffer();

	  if (BootPinState == 0)
	  {
	    esp32_pull_boot_pin_low();  // If make this conditional, can also use as pass-thru
	  }

		switch (BoardId)
		{
		default:
		case BOARD_ID_GEN4:
		case BOARD_ID_510:
		case BOARD_ID_511:

			break;
		case BOARD_ID_SEWERWATCH_1:

			break;
		}

	  // Turn on power to the ESP32

	  esp32_turn_on();

	  while (1)    // must power-cycle to exit
	  {
	    uart2_to_uart4();
	    uart4_to_uart2();
	  }

	}
}

static void esp32_toggle(void)
{
	static int esp32_enabled = 0;
	if (esp32_enabled)
	{
		// Disable
		uart1_tx_string("ESP32 disabled.\n");
		esp32_enabled = 0;

		wifi_sleep();
		//HAL_GPIO_WritePin(GEN5_WIFI_PWR_EN_Port, GEN5_WIFI_PWR_EN_Pin , GPIO_PIN_RESET);

	}
	else
	{
		// Enable
		uart1_tx_string("ESP32 enabled.\n");
		esp32_enabled = 1;

		wifi_wakeup();
		//HAL_GPIO_WritePin(GEN5_WIFI_PWR_EN_Port, GEN5_WIFI_PWR_EN_Pin , GPIO_PIN_SET);

	}

}

static void pga460_toggle(void)
{
	static int pga460_enabled = 0;
	if (pga460_enabled)
	{
		// Disable
		uart1_tx_string("PGA460 disabled.\n");
		pga460_enabled = 0;
		HAL_GPIO_WritePin(GEN5_RS485_PWR_EN_Port, GEN5_RS485_PWR_EN_Pin, GPIO_PIN_RESET);
	}
	else
	{
		// Enable
		uart1_tx_string("PGA460 enabled.\n");
		pga460_enabled = 1;
		HAL_GPIO_WritePin(GEN5_RS485_PWR_EN_Port, GEN5_RS485_PWR_EN_Pin, GPIO_PIN_SET);
	}

}

// Process the key
static void process_power_up_slow_key(uint8_t ch)
{
	switch (ch)
	{
	  case '1': esp32_toggle();   break;
	  case '2': pga460_toggle();  break;
	}
}

static void PowerUpSlowTests(void)
{
	uint8_t ch;


	MX_USART1_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_NONE);  // CELL
	HAL_led_delay(5);  // give the uart a little time
	uart1_tx_string("Hello from UART1.  Power up slow tests active.\n");


	uart1_tx_string("1 = toggle ESP32,  2 = toggle PGA460\n");

	while (1)
	{
		if (uart1_inbuf_tail != uart1_inbuf_head)
		{
			// Extract the next character to be processed
			ch = uart1_inbuf[uart1_inbuf_tail];

			// Advance to next character in buffer (if any)
			uart1_inbuf_tail++;

			// Wrap (circular buffer)
			if (uart1_inbuf_tail == sizeof(uart1_inbuf))
				uart1_inbuf_tail = 0;

			// Process the key
			process_power_up_slow_key(ch);
		}
	}

}


void PassThruMainLoop(void)
{
	int sd_POWER_UP_SLOW;

	//sd_EVM_1000= 1;  // For Mark Watson cable test kludge to avoid use of SD card
    sd_EVM_460 = 1;  // kludge to avoid use of SD card
    sd_POWER_UP_SLOW = 0;

	// kludges to avoid use of SD card
	//sd_UART1_Test = 1;            // 115200, 8, N, 1   CELL
	//sd_UART2_Test = 1;          // 115200, 8, N, 1   WIFI and BT
	//sd_UART3_Test = 1;          // 115200, 8, N, 1   RS-485
	//sd_UART4_Test = 1;          // 115200, 8, N, 1   VELOCITY 1
	//sd_LPUART1_Test = 1;        // 115200, 8, N, 1   VELOCITY 2
	// end kludge

    //extern void Gen5_MAX31820_test(void);
    //Gen5_MAX31820_test();


    if (sd_POWER_UP_SLOW)
    {
    	PowerUpSlowTests();
    }
    else if (sd_EVM_1000)
	{
		MX_USART1_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_NONE);  // CELL
		HAL_led_delay(5);  // give the uart a little time
		uart1_tx_string("Hello from UART1 (CELL).  Set up as pass-thru to VEL2\n");

		MX_LPUART1_UART_Init(115200, UART_STOPBITS_1);  // Initialize lpuart1 for the initial talking to the Velocity Board

		// Power on Velocity Board
		// PD5 will Enable power (14.8v direct from battery) for the Velocity 1 board.
		// PD6 will Enable power (14.8v direct from battery) for the Velocity 1 board.
		//TODO use Port D5 or D6 depending on Velocity 1 or 2


		HAL_led_delay(1);  // give it time to turn on

#if 0   // This is for EVM mode.

		// Tell the Velocity board to enter mode EVM1
		lpuart1_tx_string("EVM1");
		HAL_led_delay(1);  // give it time to respond

		// Change to the required communication parameters for EVM1 mode (9600, 8, N, 1)
		MX_USART1_UART_Init(9600, UART_STOPBITS_1, UART_HWCONTROL_NONE);
		MX_LPUART1_UART_Init(9600, UART_STOPBITS_1);

		uart1_flush_inbuffer();
		lpuart1_flush_inbuffer();

		AppendStatusMessage("Configured as pass-thru (9600,8,N,1) for TDC 1000 EVM.");

#endif


        while (1)
        {
  		  process_uart1();
  		  process_lpuart1();
        }
	}
	else if (sd_EVM_460)
	{
		// Turn on power to the Level Board
		// SKYWIRE GEN4: PA7 (VH_PWR_EN_Pin) is used to supply 7.5v to the Level Board.
	    HAL_GPIO_WritePin(GEN45_VH_PWR_EN_Port,GEN45_VH_PWR_EN_Pin, GPIO_PIN_SET);

		//HAL_GPIO_WritePin(GEN5_RS485_PWR_EN_Port,GEN5_RS485_PWR_EN_Pin, GPIO_PIN_SET);

		HAL_led_delay(10);  // give it time to turn on

	    // 115200 may be too high; use 38400
#ifdef USE_UART1_AND_LPUART1  // For pass-thru to EVM for SewerWatch testing
		MX_USART1_UART_Init(38400, UART_STOPBITS_2, UART_HWCONTROL_NONE);  // This initializes uart1 for the initial talking to the PC
		HAL_led_delay(5);  // give the uart a little time

		//uart4_tx_string("Hello from UART1.  (38400,8,N,2) for PGA460 EVM.\n");
		//while (1);

		MX_LPUART1_UART_Init(38400, UART_STOPBITS_2);  // This initializes lpuart1  for the initial talking to the Level Board
		HAL_led_delay(5);  // give the uart a little time

		AppendStatusMessage("Configured as pass-thru UART1 to LPUART1 (onboard PGA460) (38400,8,N,2) for PGA460 EVM.");
		uart1_flush_inbuffer();
		lpuart1_flush_inbuffer();
#endif
#ifdef USE_UART4_AND_LPUART1  // For pass-thru to EVM for SewerWatch testing
		MX_UART4_UART_Init(38400, UART_STOPBITS_2);  // This initializes uart4 (Vel 1) for the initial talking to the PC
		HAL_led_delay(5);  // give the uart a little time

		//uart4_tx_string("Hello from UART4.  (38400,8,N,2) for PGA460 EVM.\n");
		//while (1);

		MX_LPUART1_UART_Init(38400, UART_STOPBITS_2);  // This initializes lpuart1  for the initial talking to the Level Board
		HAL_led_delay(5);  // give the uart a little time

		AppendStatusMessage("Configured as pass-thru UART4 (VEL1) to LPUART1 (onboard PGA460) (38400,8,N,2) for PGA460 EVM.");
		uart4_flush_inbuffer();
		lpuart1_flush_inbuffer();
#endif
#ifdef USE_UART1_AND_UART3
		MX_USART1_UART_Init(38400, UART_STOPBITS_2, UART_HWCONTROL_NONE);  // This initializes uart1 for the initial talking to the PC
		HAL_led_delay(5);  // give the uart a little time

		//uart4_tx_string("Hello from UART4.  (38400,8,N,2) for PGA460 EVM.\n");
		//while (1);

		MX_USART3_UART_Init(38400, UART_STOPBITS_2);  // This initializes uart3 (RS485) for the initial talking to the Level Board
		HAL_led_delay(5);  // give the uart a little time

		AppendStatusMessage("Configured as pass-thru UART1 to UART3 (38400,8,N,2) for PGA460 EVM.");
		uart1_flush_inbuffer();
		uart3_flush_inbuffer();
#endif
#ifdef USE_UART4_AND_UART3   // For pass-thru to EVM for PGA460 Level board testing
		MX_UART4_UART_Init(38400, UART_STOPBITS_2);  // This initializes uart4 (Vel 1) for the initial talking to the PC
		HAL_led_delay(5);  // give the uart a little time

		//uart4_tx_string("Hello from UART4.  (38400,8,N,2) for PGA460 EVM.\n");
		//while (1);

		MX_USART3_UART_Init(38400, UART_STOPBITS_2);  // This initializes uart3 (RS485) for the initial talking to the Level Board
		HAL_led_delay(5);  // give the uart a little time

		AppendStatusMessage("Configured as pass-thru UART4 (VEL1) to UART3 (RS485,LEVEL) (38400,8,N,2) for PGA460 EVM.");
		uart4_flush_inbuffer();
		uart3_flush_inbuffer();
#endif



		reset_state_machine();  // For PGA460 cmd stripping

		while (1)
		{
#ifdef USE_UART1_AND_LPUART1   // For PGA460 PassThru
			process_uart1_with_scan_to_lpuart1();   // perform PGA460 cmd stripping
			process_lpuart1_to_uart1();
#endif
#ifdef USE_UART4_AND_LPUART1   // For PGA460 PassThru
			process_uart4_with_scan_to_lpuart1();   // perform PGA460 cmd stripping
			process_lpuart1_to_uart4();
#endif
#ifdef USE_UART1_AND_UART3   // For PGA460 PassThru
			process_uart1_with_scan_to_uart3();   // perform PGA460 cmd stripping
			process_uart3_to_uart1();
#endif
#ifdef USE_UART4_AND_UART3
			process_uart4_with_scan_to_uart3();   // perform PGA460 cmd stripping
			process_uart3_to_uart4();
#endif
		}
	}
	else if (sd_UART3_Echo)
	{
		// Echo mode for testing end-of-line LevelBoard position with an iTracker that just echos what it gets on uart3 (RS485).
		// This validates the end-to-end communication pc<==>iTracker==>Level via RS485 transmission
		MX_USART3_UART_Init(115200, UART_STOPBITS_2);
		while (1)
		{
			//uart3_loopback();
		}
	}
	else if (sd_UART1_Test)  // CELL
	{

        // Connect to pc first so as to see any power-on messages from the CELL
#if 0
		MX_USART2_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_NONE);
		HAL_led_delay(5);  // give the uart a little time
		uart2_tx_string("Hello from UART1 and UART2\n");
#else
		MX_UART4_UART_Init(115200, UART_STOPBITS_1);
		HAL_led_delay(5);  // give the uart a little time
		uart4_tx_string("Hello from UART4.  Set up as pass-thru to UART1 (CELL)\n");
#endif

#if 1
	    SetCellEnabled();
	    s2c_CellModemFound = S2C_LAIRD_MODEM;
	    S2C_POWERUP();   // Cell is already powered on by this point, but must enforce the power-on delay so it can talk
	    AT_OK("AT+CGMM\r", 1000);

		//s2c_powerup_cell_hardware();
		// MNA (RTS,CTS required) and Laird (RTS, CTS required)
		//uart1_flush_inbuffer();
		//MX_USART1_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_RTS_CTS);  // This initializes uart1 for the initial talking to either the Telit or the Laird Modem
	    //HAL_led_delay(5);  // give the uart a little time
	    //HAL_led_delay(2000);  // give the cell modem some time
#endif


#if 0
	    // set module to a known state
	    CellConfig.ctx = 1;  // varies, depends on carrier. Often 0 = default, 1 = ATT, 5 = Verizon
	    strcpy(CellConfig.apn,"flolive.net");
	    Save_CellConfig();
	    laird_reset();   // This takes a second or two..
#endif

	    while (1)
	    {
	    	uint8_t ch;
	    	while (uart1_getchar(&ch)) uart4_putchar(&ch);
	    	while (uart4_getchar(&ch)) uart1_putchar(&ch);
	      //uart1_echo();
	      //uart1_tx_string("Hello from UART1\n");
	      //wifi_send_string("Hello from UART2\n");
	      //HAL_led_delay(2000);
	    }
	}
	else if (sd_UART2_Test)  // BT & WIFI
	{
		// Low-level test of uart2 to bluetooth enabled ESP32 chip
		MX_USART2_UART_Init(115200, UART_STOPBITS_1, UART_HWCONTROL_NONE);
		HAL_led_delay(5);  // give the uart a little time
		uart2_tx_string("Hello from UART2\n");
	    while (1)
	    {
	      //uart2_tx_string("Hello from UART2\n");
		  //uart2_loopback();  // loopback

	      //wifi_send_string("Hello from UART2\n");
	      //HAL_led_delay(2000);
	    }
	}
	else if (sd_UART3_Test)  // RS-485
	{
		// Low-level test of uart3 to pc
		MX_USART3_UART_Init(115200, UART_STOPBITS_1);
	    while (1)
	    {
	      uart3_tx_string("Hello from UART3\n");
	      HAL_led_delay(2000);
	    }
	}
	else if (sd_UART4_Test)  // VELOCITY 1
	{
		// Low-level test of uart4 to pc
		//MX_UART4_UART_Init(115200, UART_STOPBITS_1);  // For some reason, this needed to be hoisted higher in the call chain, just after hw_init

	    while (1)
	    {
	      char msg[80];
	      sprintf(msg, "Hello from UART4 mcu_id = %lu\n", stm32_DEVID);
	      uart4_tx_string(msg);
	      HAL_led_delay(2000);
	    }
	}
	else if (sd_LPUART1_Test)  // VELOCITY 2
	{
		// Low-level test of lpuart1 to pc
		MX_LPUART1_UART_Init(115200, UART_STOPBITS_1);
	    while (1)
	    {
	      char msg[80];
	      sprintf(msg, "Hello from LPUART1 mcu_id = %lu\n", stm32_DEVID);
	      lpuart1_tx_string(msg);

	      HAL_led_delay(2000);
	    }
	}
	else
	{
		// Default is EASTECH mode, which is not a pass-thru mode
		AppendStatusMessage("Configured as EASTECH mode.");
		return;
	}
}






