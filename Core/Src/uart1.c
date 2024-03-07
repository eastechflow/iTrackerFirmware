/*
 * uart1.c
 *
 *  Created on: Feb 28, 2020
 *      Author: Zachary
 */

#include "stm32l4xx_hal.h"
#include "usart.h"
#include "uart1.h"
#include "lpuart1.h"
#include "rtc.h"

#include "trace.h"
#include "factory.h"
#include "debug_helper.h"
#include "s2c.h"

// The LTEC Cat M2 serial with GNSS operates at 921.Kkpbs = 115200 bytes/sec = 115 bytes/ms



// ISR Receive Buffer
uint8_t uart1_inbuf[UART1_INBUF_SIZE];
volatile uint16_t uart1_inbuf_head = 0;
volatile uint16_t uart1_inbuf_tail = 0;
volatile uint16_t uart1_inbuf_max  = 0;

// ISR Transmit Buffer
char uart1_outbuf[UART1_OUTBUF_SIZE];
volatile uint16_t uart1_outbuf_head = 0;
volatile uint16_t uart1_outbuf_tail = 0;
volatile uint16_t uart1_outbuf_max  = 0;


__IO uint32_t imei_timeout = 0;
__IO uint32_t uart1_timeout = 0;
__IO uint32_t cell_response_code_received = RC_NONE;
__IO uint32_t cell_urc_received = RC_NONE;
__IO uint32_t uart1_json_string_received   = RC_NONE;


static void laird_scan_for_tokens(void)
{
	// For Laird:
	// The "AT" or "at" prefix must be set at the beginning of each line. To terminate a command line, a
	// <CR> character must be inserted.
	// Commands are usually followed by a response that includes ‘<CR><LF><response><CR><LF>’.

	//if (uart1_inbuf[uart1_inbuf_head] != '\n') return;  // only scan when last character received is LF
	if (cell_response_code_received != RC_NONE) return;  // already scanned a response code

	// scan for urc \0<cr><lf>+KSUP: <cr><lf>    why is there a leading null-terminator? is there an issue with the very first byte received?
	if ((uart1_inbuf_head > 7) && (cell_urc_received == RC_NONE))
	{
		if ((uart1_inbuf[uart1_inbuf_head -  8] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head -  7] == '\n') &&
			(uart1_inbuf[uart1_inbuf_head -  6] == '+')  &&
			(uart1_inbuf[uart1_inbuf_head -  5] == 'K')  &&
			(uart1_inbuf[uart1_inbuf_head -  4] == 'S')  &&
			(uart1_inbuf[uart1_inbuf_head -  3] == 'U')  &&
			(uart1_inbuf[uart1_inbuf_head -  2] == 'P')  &&
			(uart1_inbuf[uart1_inbuf_head -  1] == ':')  &&
			(uart1_inbuf[uart1_inbuf_head     ] == ' ')  )  // not parsing <stat>
		{
			cell_urc_received = RC_KSUP;
		}
	}

	// scan for urc \0<cr><lf>NO CARRIER<cr><lf>
	if ((uart1_inbuf_head > 12) && (cell_urc_received == RC_NONE))
	{
		if ((uart1_inbuf[uart1_inbuf_head - 13] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head - 12] == '\n') &&
			(uart1_inbuf[uart1_inbuf_head - 11] == 'N')  &&
			(uart1_inbuf[uart1_inbuf_head - 10] == 'O')  &&
			(uart1_inbuf[uart1_inbuf_head -  9] == ' ')  &&
			(uart1_inbuf[uart1_inbuf_head -  8] == 'C')  &&
			(uart1_inbuf[uart1_inbuf_head -  7] == 'A')  &&
			(uart1_inbuf[uart1_inbuf_head -  6] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head -  5] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head -  4] == 'I')  &&
			(uart1_inbuf[uart1_inbuf_head -  3] == 'E')  &&
			(uart1_inbuf[uart1_inbuf_head -  2] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head -  1] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head     ] == '\n'))
		{
			cell_urc_received = RC_NO_CARRIER;
		}
	}

	// scan for <cr><lf>OK<cr><lf>
	if ((uart1_inbuf_head > 4)  && (cell_response_code_received == RC_NONE))
	{
		if ((uart1_inbuf[uart1_inbuf_head - 5] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head - 4] == '\n') &&
			(uart1_inbuf[uart1_inbuf_head - 3] == 'O')  &&
			(uart1_inbuf[uart1_inbuf_head - 2] == 'K')  &&
			(uart1_inbuf[uart1_inbuf_head - 1] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head    ] == '\n'))
		{
			cell_response_code_received = RC_OK;
		}
	}

	// scan for <cr><lf>ERROR<cr><lf>
	if ((uart1_inbuf_head > 7) && (cell_response_code_received == RC_NONE))
	{
		if ((uart1_inbuf[uart1_inbuf_head - 8] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head - 7] == '\n') &&
			(uart1_inbuf[uart1_inbuf_head - 6] == 'E')  &&
			(uart1_inbuf[uart1_inbuf_head - 5] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head - 4] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head - 3] == 'O')  &&
			(uart1_inbuf[uart1_inbuf_head - 2] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head - 1] == '\r')  &&
			(uart1_inbuf[uart1_inbuf_head    ] == '\n'))
		{
			cell_response_code_received = RC_ERROR;   // technically the response still has some more characters coming, how to wait for them?
		}
	}

	// scan for <cr><lf>+CME ERROR<cr><lf>
	if ((uart1_inbuf_head > 12) && (cell_response_code_received == RC_NONE))
	{
		if ((uart1_inbuf[uart1_inbuf_head - 13] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head - 12] == '\n') &&
			(uart1_inbuf[uart1_inbuf_head - 11] == '+')  &&
			(uart1_inbuf[uart1_inbuf_head - 10] == 'C')  &&
			(uart1_inbuf[uart1_inbuf_head - 9] == 'M')  &&
			(uart1_inbuf[uart1_inbuf_head - 8] == 'E')  &&
			(uart1_inbuf[uart1_inbuf_head - 7] == ' ')  &&
			(uart1_inbuf[uart1_inbuf_head - 6] == 'E')  &&
			(uart1_inbuf[uart1_inbuf_head - 5] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head - 4] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head - 3] == 'O')  &&
			(uart1_inbuf[uart1_inbuf_head - 2] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head - 1] == ':')  &&
			(uart1_inbuf[uart1_inbuf_head    ] == ' '))
		{
			cell_response_code_received = RC_ERROR;   // technically the response still has some more characters coming, how to wait for them?
		}
	}

	// scan for <cr><lf>CONNECT<cr><lf>
	if ((uart1_inbuf_head > 9) && (cell_response_code_received == RC_NONE))
	{
		if ((uart1_inbuf[uart1_inbuf_head - 10] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head -  9] == '\n') &&
			(uart1_inbuf[uart1_inbuf_head -  8] == 'C')  &&
			(uart1_inbuf[uart1_inbuf_head -  7] == 'O')  &&
			(uart1_inbuf[uart1_inbuf_head -  6] == 'N')  &&
			(uart1_inbuf[uart1_inbuf_head -  5] == 'N')  &&
			(uart1_inbuf[uart1_inbuf_head -  4] == 'E')  &&
			(uart1_inbuf[uart1_inbuf_head -  3] == 'C')  &&
			(uart1_inbuf[uart1_inbuf_head -  2] == 'T')  &&
			(uart1_inbuf[uart1_inbuf_head -  1] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head     ] == '\n'))
		{
			cell_response_code_received = RC_CONNECT;
		}
	}
}

// Scan for Result Code tokens OK, ERROR and CONNECT (must end with LF)
static void telit_scan_for_tokens(void)
{
	if (uart1_inbuf[uart1_inbuf_head] != '\n') return;  // only scan when last character received is LF
	if (cell_response_code_received != RC_NONE) return;  // already scanned a response code

	// scan for <cr><lf>OK<cr><lf>
	if ((uart1_inbuf_head > 4)  && (cell_response_code_received == RC_NONE))
	{
		if ((uart1_inbuf[uart1_inbuf_head - 5] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head - 4] == '\n') &&
			(uart1_inbuf[uart1_inbuf_head - 3] == 'O')  &&
			(uart1_inbuf[uart1_inbuf_head - 2] == 'K')  &&
			(uart1_inbuf[uart1_inbuf_head - 1] == '\r'))
		{
			cell_response_code_received = RC_OK;  // the trailing LF was tested first
		}
	}

	// scan for <cr><lf>ERROR<cr><lf>
	if ((uart1_inbuf_head > 7) && (cell_response_code_received == RC_NONE))
	{
		if ((uart1_inbuf[uart1_inbuf_head - 8] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head - 7] == '\n') &&
			(uart1_inbuf[uart1_inbuf_head - 6] == 'E')  &&
			(uart1_inbuf[uart1_inbuf_head - 5] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head - 4] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head - 3] == 'O')  &&
			(uart1_inbuf[uart1_inbuf_head - 2] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head - 1] == '\r'))
		{
			cell_response_code_received = RC_ERROR;  // the trailing LF was tested first
		}
	}

	// scan for <cr><lf>CONNECT<cr><lf>
	if ((uart1_inbuf_head > 9) && (cell_response_code_received == RC_NONE))
	{
		if ((uart1_inbuf[uart1_inbuf_head - 10] == '\r') &&
			(uart1_inbuf[uart1_inbuf_head -  9] == '\n') &&
			(uart1_inbuf[uart1_inbuf_head -  8] == 'C')  &&
			(uart1_inbuf[uart1_inbuf_head -  7] == 'O')  &&
			(uart1_inbuf[uart1_inbuf_head -  6] == 'N')  &&
			(uart1_inbuf[uart1_inbuf_head -  5] == 'N')  &&
			(uart1_inbuf[uart1_inbuf_head -  4] == 'E')  &&
			(uart1_inbuf[uart1_inbuf_head -  3] == 'C')  &&
			(uart1_inbuf[uart1_inbuf_head -  2] == 'T')  &&
			(uart1_inbuf[uart1_inbuf_head -  1] == '\r'))
		{
			cell_response_code_received = RC_CONNECT;  // the trailing LF was tested first
		}
	}
}


// Scan for Result Code tokens OK, ERROR
static void xbee3_scan_for_tokens(void)
{
	if (uart1_inbuf[uart1_inbuf_head] != '\r') return;  // only scan when last character received is CR
	if (cell_response_code_received != RC_NONE) return;  // already scanned a response code

	// scan for OK<cr>
	if ((uart1_inbuf_head > 2)  && (cell_response_code_received == RC_NONE))
	{
		if ((uart1_inbuf[uart1_inbuf_head - 2] == 'O')  &&
			(uart1_inbuf[uart1_inbuf_head - 1] == 'K'))
		{
			cell_response_code_received = RC_OK;  // the trailing CR was tested first
			return;
		}
	}

	// scan for ERROR<cr>
	if ((uart1_inbuf_head > 7) && (cell_response_code_received == RC_NONE))
	{
		if ((uart1_inbuf[uart1_inbuf_head - 5] == 'E')  &&
			(uart1_inbuf[uart1_inbuf_head - 4] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head - 3] == 'R')  &&
			(uart1_inbuf[uart1_inbuf_head - 2] == 'O')  &&
			(uart1_inbuf[uart1_inbuf_head - 1] == 'R'))
		{
			cell_response_code_received = RC_ERROR;  // the trailing CR was tested first
			return;
		}
	}

	//Any response ending with a CR is OK
	cell_response_code_received = RC_OK;  // the trailing CR was tested first
}





void USART1_IRQHandler(void)
{
	uint32_t errorflags;
	uint32_t isrflags = READ_REG(huart1.Instance->ISR);
	errorflags = (isrflags & (uint32_t) (USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));

	if (errorflags != RESET)
	{
		if ((isrflags & USART_ISR_ORE) != RESET)
		{
			__HAL_UART_CLEAR_IT(&huart1, UART_CLEAR_OREF);
		}
		if ((isrflags & USART_ISR_FE) != RESET)
		{
			__HAL_UART_CLEAR_IT(&huart1, UART_CLEAR_FEF);
		}
	}

	if ((USART1->ISR & USART_ISR_RXNE) == USART_ISR_RXNE)
	{
		// Read from the UART.
		uint8_t ch = (uint8_t)(USART1->RDR);

#if 0
		// Auto-flush the input buffer so c-style strings don't wrap
		if (uart1_inbuf_head)
		{
			if (uart1_inbuf_tail == uart1_inbuf_head)  // the client has consumed all characters so far
			{
				uart1_flush_inbuffer();
			}
		}
#endif

		uart1_inbuf[uart1_inbuf_head] = ch;  // save the incoming character in the array

		switch (s2c_CellModemFound)
		{
		  default:
		  case S2C_NO_MODEM:    break;
		  case S2C_TELIT_MODEM: telit_scan_for_tokens(); break;
		  case S2C_XBEE3_MODEM: xbee3_scan_for_tokens(); break;
		  case S2C_LAIRD_MODEM: laird_scan_for_tokens(); break;
		}


		if ((ch == '{') && (uart1_json_string_received == RC_NONE))       uart1_json_string_received = RC_JSON_OPEN;
		if ((ch == '}') && (uart1_json_string_received == RC_JSON_OPEN))  uart1_json_string_received = RC_JSON_CLOSE;

		// increment to next position
		uart1_inbuf_head++;
		if (uart1_inbuf_head > uart1_inbuf_max)  uart1_inbuf_max = uart1_inbuf_head;  // Keep track of high watermark

		// Check for overflow.
		if (uart1_inbuf_head >= UART1_INBUF_SIZE)
		{
			uart1_inbuf_head = 0;
		}


		// Always maintain a null-terminated input buffer for parsing and logging contents of RX buffer.
		// A buffer overflow will lose the entire buffer.
		uart1_inbuf[uart1_inbuf_head] = 0;

	}
	else if ((USART1->ISR & USART_ISR_TC) == USART_ISR_TC)
	{
		if (uart1_outbuf_tail < uart1_outbuf_head)
		{
			// Previous character is done, transmit next character, increment to next position
			USART1->TDR = uart1_outbuf[uart1_outbuf_tail];
			uart1_outbuf_tail++;
		}
		else
		{
			// Clear the transmit.
			USART1->ICR = USART_ICR_TCCF;
			uart1_outbuf_head = 0;
			uart1_outbuf_tail = 0;
		}
	}
}

void uart1_flush_inbuffer()
{
#if 0
	for (int i = 0; i < sizeof(uart1_inbuf); i++)
	{
		uart1_inbuf[i] = '\0';  // null-terminate the entire buffer...
	}
#endif

	NVIC_DisableIRQ(USART1_IRQn);

	uart1_inbuf_head = 0;
	uart1_inbuf_tail = 0;
	uart1_inbuf[0] = '\0';
	cell_urc_received = RC_NONE;
	cell_response_code_received = RC_NONE;   // maybe do this only on outbound AT commands to avoid parsing data streams?
	uart1_json_string_received = RC_NONE;

	NVIC_EnableIRQ(USART1_IRQn);
}



#if 0
void cell_flush_outbuffer()
{
	for (int i = 0; i < sizeof(uart1_outbuf); i++)
	{
		uart1_outbuf[i] = '\0';
	}
	uart1_outbuf_head = 0;
	uart1_outbuf_tail = 0;
}
#endif

uint32_t uart1_TX_byte_count;
uint32_t uart1_TX_max_size = 0;

int uart1_tx(uint32_t txSize)
{
	// Make sure the array isn't empty and send.
	if (txSize == 0) return 0; // nothing to send

	// Update the buffer indices.
	uart1_outbuf_head = txSize;  // this is used to stop TX
	uart1_outbuf_tail = 0;       // this is used to TX each character in order

	if (uart1_outbuf_head > uart1_outbuf_max) uart1_outbuf_max = uart1_outbuf_head;  // Keep track of high watermark

	// Start the transmission
	if ((USART1->ISR & USART_ISR_IDLE) == USART_ISR_IDLE) // zg - why is this needed?
	{
		if ((USART1->ISR & USART_ISR_TXE) == USART_ISR_TXE) // zg - why is this needed?
		{
			// Transmit - fires the first one. The ISR takes over from there.
			USART1->TDR = uart1_outbuf[uart1_outbuf_tail]; // Send the first char to start.
			uart1_outbuf_tail++;
		}
		else
		{
			if (sd_CellCmdLogEnabled)
			{
				trace_AppendMsg("\nTX Error: TXE\n");
			}
		}
	}
	else
	{
		if (sd_CellCmdLogEnabled)
		{
			trace_AppendMsg("\nTX Error: IDLE\n");
		}
	}


	// Must confirm the transmit completed before proceeding.
	// The intention is to block until transmission is done.
	// Use an escape mechanism to avoid getting trapped here.
	//TODO may need two timeouts:  1) to start TX, 2) to complete TX.  Only start TX is implemented

	uart1_timeout = 2000;  // TX should start within 2000 ms

	while (uart1_outbuf_head != 0)   // ISR will set to zero when TX is complete, might already be done by the time we get here.
	{
		led_heartbeat();

		// Check that a transmit starts within the timeout period
		if (uart1_outbuf_tail == 1)  // no character has been sent yet
		{
			if (uart1_timeout == 0)  // TX start timeout expired
			{
				// Clear the transmit.
				//USART1->ICR = USART_ICR_TCCF;   // zg - let the ISR do this
				uart1_outbuf_head = 0;
				uart1_outbuf_tail = 0;
				if (sd_CellCmdLogEnabled)
				{
					trace_AppendMsg("\nTX Error: timeout\n");
				}
				return 0;  // TX was not started, cmd not sent
			}
		}
	}

	if (sd_CellDataLogEnabled)
	{
		trace_AppendMsg("\nTX ");
		trace_AppendMsg(uart1_outbuf);  // <=== this assumes the buffer is null-terminated
	}

	uart1_TX_byte_count += txSize;  // count all bytes transmitted
	if (txSize > uart1_TX_max_size) uart1_TX_max_size = txSize;  // keep track of largest packet

	return 1; // TX completed OK
}


int uart1_tx_bytes(const uint8_t *txBuff, uint32_t txSize)
{
	// Make sure the array isn't empty and send.
	if (txSize == 0) return 0; // nothing to send

	// Copy the input array into the ISR output buffer.

	for (int i = 0; i < txSize; i++)
	{
		if (i < sizeof(uart1_outbuf))
		{
		  uart1_outbuf[i] = (char) txBuff[i];
		}

		if (sd_UART_5SPY1)
		{
			if (i < sizeof(lpuart1_outbuf))
			{
			  lpuart1_outbuf[i] = (char) txBuff[i]; // echo outbound chars to UART5
			}
		}
	}

	if (sd_UART_5SPY1) lpuart1_tx(txSize,0);  // echo outbound chars to UART5

	return uart1_tx(txSize);
}



// Sends the string to UART1
int uart1_tx_string(const char * txBuff)
{
	uint32_t txSize;

	txSize = strlen(txBuff);

	return uart1_tx_bytes((const uint8_t *)txBuff, txSize);

}





// Sends the string to UART1
int cell_send_string(const char * txBuff)
{
	int result;
	int data_log_flag;

	// temporarily defeat Data logging so either Cmds, or Data, or both can occur without repeats
	data_log_flag = sd_CellDataLogEnabled;
	sd_CellDataLogEnabled = 0;  // defeat data logging

	result = uart1_tx_string(txBuff);

	sd_CellDataLogEnabled = data_log_flag;  // restore data logging, if any

	return result;
}

int uart1_getchar(uint8_t *pRxChar)
{
	if (uart1_inbuf_tail != uart1_inbuf_head)
	{


		// Extract the next character to be processed
		*pRxChar = uart1_inbuf[uart1_inbuf_tail];

		// Advance to next character in buffer (if any)
		uart1_inbuf_tail++;

		// Wrap (circular buffer)
		if (uart1_inbuf_tail == sizeof(uart1_inbuf))
			uart1_inbuf_tail = 0;




		if (sd_UART_5SPY1) lpuart1_putchar(pRxChar);  // echo inbound chars to UART5

		return 1;
	}
	return 0;
}

void uart1_putchar(const uint8_t *TxChar)
{
	uart1_tx_bytes(TxChar, 1);
}
