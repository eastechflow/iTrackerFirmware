/*
 * uart2.c
 *
 *  Created on: Apr 11, 2020
 *      Author: Zachary
 */

#include "stm32l4xx_hal.h"
#include "usart.h"
#include "uart2.h"
#include "uart4.h"
#include "lpuart1.h"
#include "rtc.h"

#include "trace.h"
#include "factory.h"
#include "debug_helper.h"

// WiFi USART handle
extern UART_HandleTypeDef huart2;

// ISR Receive Buffer
uint8_t uart2_inbuf[UART2_INBUF_SIZE];
volatile uint16_t uart2_inbuf_head = 0;
volatile uint16_t uart2_inbuf_tail = 0;
volatile uint16_t uart2_inbuf_max  = 0;


// ISR Transmit Buffer
uint8_t uart2_outbuf[UART2_OUTBUF_SIZE];
volatile uint16_t uart2_outbuf_head = 0;
volatile uint16_t uart2_outbuf_tail = 0;
volatile uint16_t uart2_outbuf_max  = 0;


volatile uint32_t uart2_tx_start = 0;
volatile uint32_t uart2_tx_end = 0;

__IO uint32_t uart2_timeout = 0;
__IO uint32_t uart2_previous_tx_wait_time_ms = 0;

volatile uint8_t uart2_XON_XOFF_state = XON;
uint8_t uart2_XON_XOFF_prev_state = XON;


int uart2_esp32_version    = ESP32_VERSION_1_3;
int uart2_XON_XOFF_enabled = 0;

uint32_t esp32_serial_rx_buffer_size = 256;
uint32_t esp32_payload_size          = 100;
uint32_t esp32_payload_spacing_ms    =   5;




void USART2_IRQHandler(void)
{
	uint32_t errorflags;
	uint32_t isrflags = READ_REG(huart2.Instance->ISR);
	errorflags = (isrflags & (uint32_t) (USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));

	if (errorflags != RESET)
	{
		if ((isrflags & USART_ISR_ORE) != RESET)
		{
			__HAL_UART_CLEAR_IT(&huart2, UART_CLEAR_OREF);
		}
		if ((isrflags & USART_ISR_FE) != RESET)
		{
			__HAL_UART_CLEAR_IT(&huart2, UART_CLEAR_FEF);
		}
	}

	if ((USART2->ISR & USART_ISR_RXNE) == USART_ISR_RXNE)
	{
		// Read from the UART.
		uint8_t ch = (uint8_t)(USART2->RDR);


		if (uart2_XON_XOFF_enabled)  // cannot do this when bootloading the ESP32
		{
			// Handle XON XOFF
			if (ch == XON)
			{
				uart2_XON_XOFF_state = XON;
				return;
			}
			if (ch == XOFF)
			{
				uart2_XON_XOFF_state = XOFF;
				return;
			}
			if (ch == GOT_HERE)  // for probing code in the ESP32; set a breakpoint on the return statement
			{
				return;
			}
		}


		uart2_inbuf[uart2_inbuf_head] = ch;  // save the incoming character in the array
		uart2_inbuf_head++;             // increment to next position

		if (uart2_inbuf_head > uart2_inbuf_max) uart2_inbuf_max = uart2_inbuf_head;  // Keep track of high watermark
		// Check for overflow.
		if (uart2_inbuf_head >= sizeof(uart2_inbuf))
		{
			uart2_inbuf_head = 0;
		}

		// Check if head has overtaken tail --- that means the firmware has not been able to keep up
		// Not sure what to do about that
		//if (uart2_inbuf_head < uart2_inbuf_tail)
		//{
		//	while(1);  // Bug Hunt.
		//}

		// Always maintain a null-terminated input buffer for parsing and logging contents of RX buffer.
		// A buffer overflow will lose the entire buffer.
		uart2_inbuf[uart2_inbuf_head] = 0;
	}
	else if ((USART2->ISR & USART_ISR_TC) == USART_ISR_TC)
	{
		if ( uart2_outbuf_tail < uart2_outbuf_head)
		{
			// Transmit.
			USART2->TDR = uart2_outbuf[uart2_outbuf_tail];
			uart2_outbuf_tail++;
		}
		else
		{
			// Clear the transmit.
			USART2->ICR = USART_ICR_TCCF;
			uart2_outbuf_head = 0;
			uart2_outbuf_tail = 0;
			uart2_tx_end = HAL_GetTick();
		}
	}
}

void uart2_flush_inbuffer()
{
	//for (int i = 0; i < sizeof(uart2_inbuf); i++)
	//{
	//	uart2_inbuf[i] = '\0';
	//}
	//NVIC_DisableIRQ(USART2_IRQn);
	uart2_inbuf_head = 0;
	uart2_inbuf_tail = 0;
	uart2_inbuf[0] = '\0';
	//NVIC_EnableIRQ(USART2_IRQn);

}

#if 0
void uart2_flush_outbuffer()
{
	for (int i = 0; i < sizeof(uart2_outbuf); i++)
	{
		uart2_outbuf[i] = '\0';
	}
	uart2_outbuf_head = 0;
	uart2_outbuf_tail = 0;
}
#endif

//#define BT_PAYLOAD_SIZE   100 // tested and seemed to be working.  Probably overhead and rounding made it match the desired 140 pace?  Or the ESP32 buffer was able to keep up with tested data?
#define BT_PAYLOAD_SIZE   140

#if 0
static void original_delay_code(uint32_t txSize)
{
#ifdef PACE_UART2_TX
	// Corruption (missing records) was observed using iOS BT downloads.  Unclear what is causing this.
	// Is it the iOS App, BT traffic, or the ESP32 BLE UART App ?
	//
	// The BLE specification enforces a packet size payload of 140 bytes, and window to transmit of 5 ms.
	// That is, you have AT MOST, one opportunity to send 140 bytes every 5 ms, assuming you are
	// allocated EVERY possible transmission window.
	//
	// The Gen5 ESP32 BLE UART App enforces this packet size, and time between packets.
	// BT packet size is 140 bytes, and there is 5 ms between packets.
	// So, can wait a variable amount of time, based on number of bytes:
	// (size / 140) * 5 = ms to wait

	// The SPY traffic with no delay is jerky, not fluid, but seems to avoid corruption.
	// This seems to imply that the act of transmitting the same bytes out another COM
	// port produces a delay on the order of what paces the BT App.

	// At 115200 it takes 86.806 us per byte
	// 5 ms = 5000 us = 55 bytes, which is about a third of the max BT packet
	// So, if the iTracker paces itself based on that formula
	// the ESP32 should never be swamped (it has a big buffer)

	//HAL_GetTick




	//if (isGen5() && !sd_UART_4SPY2)  // The SPY will inherently slow it down, so skip if active
	if (isGen5())  // Safer to always put the delay in regardless of if SPY is active or not
	{
		int ms_to_wait;

		int remainder;
		remainder = txSize % BT_PAYLOAD_SIZE;
		if (remainder == 0)
			ms_to_wait = (txSize / BT_PAYLOAD_SIZE) * 1;  // Nearest 1 ms
		else
			ms_to_wait = ((txSize / BT_PAYLOAD_SIZE) + 1) * 1;  // Rounds up to the nearest 5 ms.


#if 1
		HAL_led_delay(ms_to_wait);  // This always adds 1 ms to the requested delay
#else
		hal_led_delay_timer_ms = ms_to_wait + 1;  // to ensure at least the number of requested ms...
		while (hal_led_delay_timer_ms)
		{
			if (sd_UART_4SPY2)
			{
			  // Scan for INDICATE messages or other status messages
              if (uart2_inbuf_head) uart4_tx_string(uart2_inbuf);
			}
			led_heartbeat();
		}
#endif
	}
#endif
}

#endif

void uart2_set_tx_parameters(void)
{
	switch (uart2_esp32_version)
	{
	case ESP32_VERSION_1_6:
		uart2_XON_XOFF_enabled = 1;   // The ESP32 takes care of all comm parameters
		break;

	case ESP32_VERSION_1_5:

		esp32_serial_rx_buffer_size = 8192;
		esp32_payload_size          =  140;
		esp32_payload_spacing_ms    =    0;

		break;
	case ESP32_VERSION_1_4:

		esp32_serial_rx_buffer_size = 8192;
		esp32_payload_size          =  256;
		esp32_payload_spacing_ms    =    4;

		break;
	default: // ESP32_VERSION_1_3

		esp32_serial_rx_buffer_size =  256;
		esp32_payload_size          =  100;
		esp32_payload_spacing_ms    =    5;

		break;
	}
}

static uint32_t estimate_tx_delay_time_ms(uint32_t tx_size)
{
	uint32_t ms_to_wait;
	uint32_t connection_event_time;
	uint32_t connection_interval_time;


    // A BLE payload is 140 bytes, which occurs during one connection event*. (*Basic BLE.  A larger packet can be negotiated.)
	// Max uart2_tx buffer is 1024 bytes.  1024 / 140 = 7.3 BLE payloads/connection events.
	// a BLE payload takes a MINIMUM of 5 ms for a connection event.
	//
	// Plus, there is a latency between BLE payloads (e.g. you don't always get all available timeslots)
	// Once source states the connection interval between connection events can be negotiated
	// and range from 7.5ms to 4s.
	// Apple requires the connection interval to be a minimum of 15 ms.  In the future, the ESP32 might be able to find out what the interval actually is.
	//
	// How best to estimate this latency?

	// Good reference articles:
	// https://interrupt.memfault.com/blog/ble-throughput-primer
	// The Nordic BLE Sniffer and a nRF52 dev board are good tools.

	// estimate the "connection event" time
	// estimate the "connection interval" time
	// add some margin (by rounding up)

	connection_event_time = ((tx_size / esp32_payload_size) + 1) * 5;  // Rounds up to the nearest 5 ms.

	//connection_interval_time = ((tx_size / esp32_payload_size) + 1) * 7.5;  // Rounds up to the nearest 7.5 ms.
	connection_interval_time = ((tx_size / esp32_payload_size) + 1) * 15;  // Rounds up to the nearest 15 ms.

	//esp32_wait_time = ((tx_size / esp32_payload_size) + 1) * esp32_payload_spacing_ms;  // Rounds up to the nearest x ms.

	// if tx_size > esp32_serial_rx_buffer_size then there is a problem.  In practice, this never occurred?

	// esp32_serial_rx_buffer_size
	// esp32_payload_size
	// esp32_payload_spacing_ms

	// Version 5.0.70 rounded up to nearest 1 ms   used with BLE 1.5   8192/140/0
	// Version 5.0.69 rounded up to nearest 5 ms   used with BLE 1.4   8192/256/4
	// Version 5.0.68 rounded up to nearest 5 ms   used with BLE 1.3    256/100/5

	//ms_to_wait = ((tx_size / BT_PAYLOAD_SIZE) + 1) * 5;  // Rounds up to the nearest 5 ms.  This did not work very well on all phones.
	//ms_to_wait = ((tx_size / BT_PAYLOAD_SIZE) + 1) * 20;  // Rounds up to the nearest 20 ms.  This appeared to work well for Max and Matt

	ms_to_wait = connection_event_time + connection_interval_time;

	return ms_to_wait;
}


static void pace_via_delay(uint32_t tx_size)
{
	uint32_t ms_to_wait;

	ms_to_wait = estimate_tx_delay_time_ms(tx_size);

	uart2_previous_tx_wait_time_ms = uart2_tx_end + ms_to_wait;  // measure from the end of tx to esp32, not from end of any logging or spying

	// The actual delay will occur at the next call to uart2_tx() if it has not fully expired.
	// This allows for other activities to be performed in parallel with the BLE transmit

}


void pace_via_XON_XOFF(void)
{
    // Manage XON/XOFF here

#if 1 // Log when XON and XOFF occur in the data stream
	if (uart2_XON_XOFF_state != uart2_XON_XOFF_prev_state)
	{
		uart2_XON_XOFF_prev_state = uart2_XON_XOFF_state;
		if (sd_UART_4SPY2)   // echo outbound chars to UART4
		{
			if (uart2_XON_XOFF_state == XOFF)
			{
				sprintf(uart4_outbuf, "\nXOFF\n");
				uart4_tx(6, 0);
			}
			else if (uart2_XON_XOFF_state == XON)
			{
				sprintf(uart4_outbuf, "\nXON\n");
				uart4_tx(5, 0);
			}
		}
	}
#endif



	uart2_timeout = 7000;  // XON should occur within 7000 ms (7 s)
    while (uart2_XON_XOFF_state == XOFF)
    {
		if (uart2_timeout == 0)  // XOFF timeout expired
		{
	    	uart2_XON_XOFF_state = XON;  // Clear the XOFF state
			if (sd_WiFiCmdLogEnabled || sd_WiFiDataLogEnabled)
			{
				trace_AppendMsg("\nXOFF Error: timeout\n");
			}
		}
    }
}

static void pace_for_BLE(uint32_t tx_start, uint32_t tx_size)
{
	uint32_t tx_next;
	uint32_t ms_to_wait;

	// max tx buffer is 1024 bytes.  1024 / 140 = 7.3 BLE payloads
	// a BLE payload takes a MINIMUM of 5 ms.
	// Plus, there is a latency between BLE payloads (e.g. you don't always get all available timeslots)
	// How best to estimate this latency?  If on average, it is say, 1 ms per payload
	// and this code waits an extra 2 ms per payload, then the ESP32/BLE has a 2/5 chance of "catching up"
	// in between tx outputs from the iTracker.

	// If the average transfer rate over BT is 6 bytes / ms
	// Then 140 bytes would take 140/6 = 23.3 ms

	//ms_to_wait = ((tx_size / BT_PAYLOAD_SIZE) + 1) * 5;  // Rounds up to the nearest 5 ms.
	ms_to_wait = ((tx_size / BT_PAYLOAD_SIZE) + 1) * 20;  // Rounds up to the nearest 20 ms.

	tx_next = tx_start + ms_to_wait;

	while (HAL_GetTick() < tx_next)
	{
		led_heartbeat();
	}

}



int uart2_tx(uint32_t txSize, uint8_t trace)
{
	// Update the buffer indices.
	uart2_outbuf_head = txSize;        // this is used to stop TX
	uart2_outbuf_tail = 0;             // this is used to TX each character in order

	if (uart2_outbuf_head > uart2_outbuf_max) uart2_outbuf_max = uart2_outbuf_head;  // Keep track of high watermark

	// Wait for previous tx to complete on esp32/BLE
	while (uart2_previous_tx_wait_time_ms)
	{
		led_heartbeat();
	}

	// Start the transmission
	if ((USART2->ISR & USART_ISR_IDLE) == USART_ISR_IDLE)  // zg - why is this needed?
	{
		if ((USART2->ISR & USART_ISR_TXE) == USART_ISR_TXE)  // zg - why is this needed?
		{
			// Transmit - fires the first one. The ISR takes over from there.
			USART2->TDR = uart2_outbuf[uart2_outbuf_tail]; // Send the first char to start.
			uart2_outbuf_tail++;
			uart2_tx_start = HAL_GetTick();
			uart2_tx_end   = uart2_tx_start;  // end can never precede start
		}
		else
		{
			if (sd_WiFiCmdLogEnabled || sd_WiFiDataLogEnabled)
			{
				trace_AppendMsg("\nTX Error: TXE\n");
			}
		}
	}
	else
	{
		if (sd_WiFiCmdLogEnabled || sd_WiFiDataLogEnabled)
		{
			trace_AppendMsg("\nTX Error: IDLE\n");
		}
	}

	// Must confirm the transmit completed before proceeding.
	// The intention is to block until transmission is done.
	// Use an escape mechanism to avoid getting trapped here.
	//TODO may need two timeouts:  1) to start TX, 2) to complete TX.  Only start TX is implemented

	uart2_timeout = 2000;  // TX should start within 2000 ms

	while (uart2_outbuf_head != 0)   // wait for all characters to transmit
	{
		led_heartbeat();

		// Check that a transmit starts within the timeout period
		if (uart2_outbuf_tail == 1)  // no character has been sent yet
		{
			if (uart2_timeout == 0)  // TX start timeout expired
			{
				// Clear the transmit.
				//USART2->ICR = USART_ICR_TCCF;  // zg - let the ISR do this
				uart2_outbuf_head = 0;
				uart2_outbuf_tail = 0;
				if (sd_WiFiCmdLogEnabled || sd_WiFiDataLogEnabled)
				{
					trace_AppendMsg("\nTX Error: timeout\n");
				}
				return 0;  // TX was not started, string not sent
			}
		}
	}

	// ASSERT:
	// At this point the transmit is complete and uart2_tx_start and uart2_tx_end hold the actual transmission time.
	// Any additional output to a trace file or to the SPY uart simply adds spacing time between subsequent TX bytes.
	// When pacing for BT considerations, all of these delays are important.

	uint32_t trace_output_start;
	uint32_t trace_output_end;

	trace_output_start = HAL_GetTick();

	if (trace || sd_UART_4SPY2)
	{
		int i;
		for (i=0; i < txSize; i++)
		{
			if (trace) trace_AppendChar(uart2_outbuf[i]);

			if (sd_UART_4SPY2)   // echo outbound chars to UART4
			{
				uint8_t lf;
				lf = '\n';
				if (uart2_outbuf[i]=='{') uart4_putchar(&lf);  // to visually separate JSON strings

				uart4_putchar(&uart2_outbuf[i]);

				if (uart2_outbuf[i]=='}') uart4_putchar(&lf);  // to visually separate JSON strings
			}
		}
	}
	trace_output_end = HAL_GetTick();
	UNUSED(trace_output_start);
	UNUSED(trace_output_end);

    //original_delay_code(txSize);
	//UNUSED(original_delay_code);

	if (uart2_XON_XOFF_enabled)
	{
		pace_via_XON_XOFF();
	}
	else
	{
	    //pace_via_delay(txSize);
		UNUSED(pace_via_delay);
	    pace_for_BLE(uart2_tx_start, txSize); // Exactly as in V5.0.70
	}

	return 1; // TX completed OK
}



#if 0  // TODO
int uart2_tx(uint32_t txSize, uint8_t trace)
{
	// if txSize > esp32_serial_rx_buffer_size then there is a problem.  In practice, this never occurred?
	// Best to send chunks that match the esp32...

	int uart2_tx(uint32_t txSize, uint8_t trace);
}
#endif


int uart2_tx_bytes(const uint8_t *txBuff, uint32_t txSize, uint8_t trace)
{
	// Make sure the string isn't empty and send.
	if (txSize == 0) return 1; // nothing to send, so say OK

	if (txSize > sizeof(uart2_outbuf)) txSize = sizeof(uart2_outbuf);

	// Copy the input array into the ISR output buffer.
	for (int i = 0; i < txSize; i++)
	{
	  uart2_outbuf[i] = txBuff[i];
	}

	 return uart2_tx(txSize, trace);
}

// Sends a null-terminated string already in the outbuffer
int uart2_tx_outbuf(void)
{
	uint32_t txSize;

	txSize = strlen((char *)uart2_outbuf);

	if (txSize == 0) return 1; // nothing to send, so say OK

	return uart2_tx(txSize, sd_WiFiDataLogEnabled);
}

// Sends a string to UART2
int uart2_tx_string(const char * txBuff)
{
	uint32_t txSize;

	txSize = strlen(txBuff);

	return uart2_tx_bytes((const uint8_t *)txBuff, txSize, sd_WiFiDataLogEnabled);
}


int uart2_getchar(uint8_t *pRxChar)
{
	if (uart2_inbuf_tail != uart2_inbuf_head)
	{
		// Extract the next character to be processed
		*pRxChar = uart2_inbuf[uart2_inbuf_tail];

		// Advance to next character in buffer (if any)
		uart2_inbuf_tail++;

		// Wrap (circular buffer)
		if (uart2_inbuf_tail == sizeof(uart2_inbuf))
			uart2_inbuf_tail = 0;

		if (sd_UART_4SPY2) uart4_putchar(pRxChar);  // echo inbound chars to UART4

		return 1;
	}
	return 0;
}





void uart2_to_uart4(void)
{
	uart4_outbuf_head = 0;        // this is used to stop TX
	uart4_outbuf_tail = 0;        // this is used to TX each character in order
	uint8_t ch;


	while (uart2_inbuf_tail != uart2_inbuf_head)
	{
		// Start the transmission
		if ((UART4->ISR & USART_ISR_IDLE) == USART_ISR_IDLE)
		{
			if ((UART4->ISR & USART_ISR_TXE) == USART_ISR_TXE)
			{
				// Extract the next character to be processed
				ch = uart2_inbuf[uart2_inbuf_tail];

				// Transmit
				UART4->TDR = ch; // Send the next char


#ifdef ESP32_RECORD_FILE

				lpuart1_outbuf[0] = 'R';
				lpuart1_outbuf[1] = ch;

				lpuart1_tx(2, 0); // echo outbound chars to UART5

#endif

				// Advance to next character in buffer (if any)
				uart2_inbuf_tail++;

				// Wrap (circular buffer)
				if (uart2_inbuf_tail == sizeof(uart2_inbuf))
					uart2_inbuf_tail = 0;
			}
		}
	}
}

void uart4_to_uart2(void)
{
	uart2_outbuf_head = 0;        // this is used to stop TX
	uart2_outbuf_tail = 0;        // this is used to TX each character in order
	uint8_t ch;

	while (uart4_inbuf_tail != uart4_inbuf_head)
	{
		// Start the transmission
		if ((USART2->ISR & USART_ISR_IDLE) == USART_ISR_IDLE)
		{
			if ((USART2->ISR & USART_ISR_TXE) == USART_ISR_TXE)
			{
				// Extract next character to be processed
				ch = uart4_inbuf[uart4_inbuf_tail]; // extract the next char

				// Transmit
				USART2->TDR = ch; // Send the next char

#ifdef ESP32_RECORD_FILE

				//lpuart1_outbuf[0] = 'T';
				//lpuart1_outbuf[1] = ch;

				//lpuart1_tx(2, 0); // echo outbound chars to UART5
#endif


				// Advance to next character in buffer (if any)
				uart4_inbuf_tail++;

				// Wrap (circular buffer)
				if (uart4_inbuf_tail == sizeof(uart4_inbuf))
					uart4_inbuf_tail = 0;
			}
		}
	}
}



