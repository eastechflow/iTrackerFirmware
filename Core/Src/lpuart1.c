/*
 * lpuart1.c
 *
 *  Created on: Aug 16, 2021
 *      Author:
 */

#include "stm32l4xx_hal.h"
#include "usart.h"
#include "lpuart1.h"


#include "trace.h"
#include "factory.h"
#include "debug_helper.h"

#ifdef ENABLE_LPUART1


extern UART_HandleTypeDef hlpuart1;

// ISR Receive Buffer
//uint8_t lpuart1_inbuf[LPUART1_BUF_SIZE];
char lpuart1_inbuf[LPUART1_INBUF_SIZE];
volatile uint16_t lpuart1_inbuf_head = 0;
volatile uint16_t lpuart1_inbuf_tail = 0;
volatile uint16_t lpuart1_inbuf_max  = 0;


// ISR Transmit Buffer
//uint8_t lpuart1_outbuf[LPUART1_BUF_SIZE];
char lpuart1_outbuf[LPUART1_OUTBUF_SIZE];
volatile uint16_t lpuart1_outbuf_head = 0;
volatile uint16_t lpuart1_outbuf_tail = 0;
volatile uint16_t lpuart1_outbuf_max  = 0;

__IO uint32_t lpuart1_timeout = 0;


void LPUART1_IRQHandler(void)
{
	uint32_t errorflags;
	uint32_t isrflags = READ_REG(hlpuart1.Instance->ISR);
	errorflags = (isrflags & (uint32_t) (USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));

	if (errorflags != RESET)
	{
		if ((isrflags & USART_ISR_ORE) != RESET)
		{
			__HAL_UART_CLEAR_IT(&hlpuart1, UART_CLEAR_OREF);
		}
		if ((isrflags & USART_ISR_FE) != RESET)
		{
			__HAL_UART_CLEAR_IT(&hlpuart1, UART_CLEAR_FEF);
		}
	}

	if ((LPUART1->ISR & USART_ISR_RXNE) == USART_ISR_RXNE)
	{
		// Read from the UART.
		uint8_t ch = (uint8_t)(LPUART1->RDR);

		//if (sd_EVM_1000_Enabled) trace_AppendChar(ch);  // ISR ch logging
		//if (count == (4*20)) trace_SaveBuffer();  // ISR ch logging


		lpuart1_inbuf[lpuart1_inbuf_head] = ch;  // save the incoming character in the array
		lpuart1_inbuf_head++;             // increment to next position

		if (lpuart1_inbuf_head > lpuart1_inbuf_max) lpuart1_inbuf_max = lpuart1_inbuf_head;  // Keep track of high watermark


		// Check for overflow.
		if (lpuart1_inbuf_head >= sizeof(lpuart1_inbuf))
		{
			lpuart1_inbuf_head = 0;
		}

		// Check if head has overtaken tail --- that means the firmware has not been able to keep up
		// Not sure what to do about that

		// Always maintain a null-terminated input buffer for parsing and logging contents of RX buffer.
		// A buffer overflow will lose the entire buffer.
		lpuart1_inbuf[lpuart1_inbuf_head] = 0;
	}
	else if ((LPUART1->ISR & USART_ISR_TC) == USART_ISR_TC)
	{
		if (lpuart1_outbuf_tail < lpuart1_outbuf_head)
		{
			//if (sd_EVM_1000_Enabled) trace_AppendChar(lpuart1_outbuf[lpuart1_outbuf_tail]);  // ISR ch logging
			//if (count == (4*20)) trace_SaveBuffer();  // ISR ch logging
			// Transmit next character
			LPUART1->TDR = lpuart1_outbuf[lpuart1_outbuf_tail];
			lpuart1_outbuf_tail++;
		}
		else
		{
			// Clear the transmit complete and enable RX
			LPUART1->ICR = USART_ICR_TCCF;
			lpuart1_outbuf_head = 0;
			lpuart1_outbuf_tail = 0;

		}
	}
}

void lpuart1_flush_inbuffer()
{
	//for (int i = 0; i < sizeof(lpuart1_inbuf); i++)
	//{
	//	lpuart1_inbuf[i] = '\0';
	//}

	NVIC_DisableIRQ(LPUART1_IRQn);
	lpuart1_inbuf_head = 0;
	lpuart1_inbuf_tail = 0;
	lpuart1_inbuf[0] = '\0';
	NVIC_EnableIRQ(LPUART1_IRQn);

}

#if 0
void lpuart1_flush_outbuffer()
{
	for (int i = 0; i < sizeof(lpuart1_outbuf); i++)
	{
		lpuart1_outbuf[i] = '\0';
	}
	lpuart1_outbuf_head = 0;
	lpuart1_outbuf_tail = 0;
}
#endif



uint32_t lpuart1_TX_byte_count;
uint32_t lpuart1_TX_max_size = 0;

//#define LOG_TIMESTAMPS

int lpuart1_tx(uint32_t txSize, uint8_t trace)
{

	// Update the buffer indices.
	lpuart1_outbuf_head = txSize;        // this is used to stop TX
	lpuart1_outbuf_tail = 0;             // this is used to TX each character in order

	if (lpuart1_outbuf_head > lpuart1_outbuf_max) lpuart1_outbuf_max = lpuart1_outbuf_head;  // Keep track of high watermark

	if (trace)
	{
		char timestamp[30];
#ifdef LOG_TIMESTAMPS
		sprintf(timestamp, "\n%lu(", HAL_GetTick());
#else
		sprintf(timestamp, "\n(");
#endif
		trace_AppendMsg(timestamp);
	}

	// Start the transmission
	if ((LPUART1->ISR & USART_ISR_IDLE) == USART_ISR_IDLE)  // zg - why is this needed?
	{
		if ((LPUART1->ISR & USART_ISR_TXE) == USART_ISR_TXE)  // zg - why is this needed?
		{
			// Transmit - fires the first one. The ISR takes over from there.
			LPUART1->TDR = lpuart1_outbuf[lpuart1_outbuf_tail]; // Send the first char to start.
			lpuart1_outbuf_tail++;
		}
		else
		{
			if (trace)
			{
				trace_AppendMsg("\nTX Error: TXE\n");
			}
			return 0;  // TX was not started, string not sent
		}
	}
	else
	{
		if (trace)
		{
			trace_AppendMsg("\nTX Error: IDLE\n");
		}
		return 0;  // TX was not started, string not sent
	}


	// Must confirm the transmit completed before proceeding.
	// The intention is to block until transmission is done.
	// Use an escape mechanism to avoid getting trapped here.
	//TODO may need two timeouts:  1) to start TX, 2) to complete TX.  Only start TX is implemented

	lpuart1_timeout = 2000;  // TX should start within 2000 ms

	while (lpuart1_outbuf_head != 0)   // wait for all characters to transmit
	{
		led_heartbeat();

		// Check that a transmit starts within the timeout period
		if (lpuart1_outbuf_tail >= 1)  // no character has been sent yet
		{
			if (lpuart1_timeout == 0)  // TX start timeout expired
			{
				// Clear the transmit.
				//LPUART1->ICR = USART_ICR_TCCF;  // zg - let the ISR do this
				lpuart1_outbuf_head = 0;
				lpuart1_outbuf_tail = 0;
				if (trace)
				{
					trace_AppendMsg("\nTX Error: timeout\n");
				}

				return 0;  // TX was not started, string not sent
			}
		}
	}

	if (trace)
	{
		int i;
		for (i=0; i < txSize; i++)
		{
		  trace_AppendCharAsHex(lpuart1_outbuf[i]);
		}

		trace_AppendMsg(");");
	}

	lpuart1_TX_byte_count += txSize;  // count all bytes transmitted
	if (txSize > lpuart1_TX_max_size) lpuart1_TX_max_size = txSize;  // keep track of largest TX packet

	return txSize; // TX completed OK
}





int lpuart1_tx_bytes(const uint8_t *txBuff, uint32_t txSize, uint8_t trace)
{
	// Make sure the array isn't empty and send.
	if (txSize == 0) return 0; // nothing to send
	if (txSize > sizeof(lpuart1_outbuf)) txSize = sizeof(lpuart1_outbuf);

	// Copy the input array into the ISR output buffer.
	for (int i = 0; i < txSize; i++)
	{
		lpuart1_outbuf[i] = txBuff[i];
	}

	return lpuart1_tx(txSize, trace);
}



// Sends the string to LPUART1
int lpuart1_tx_string(const char * txBuff)
{
	uint32_t txSize;

	txSize = strlen(txBuff);

	return lpuart1_tx_bytes((const uint8_t *)txBuff, txSize, 0);
}



int lpuart1_rx_bytes(uint8_t *rxBuff, uint32_t rxSize, uint32_t timeout_ms)
{
	uint32_t count = 0;
	uint8_t ch;

	lpuart1_timeout = timeout_ms + 1; // Add one to ensure a minimum wait time given unknown state of ISR ticker

	if (sd_EVM_460_Trace)
	{
		char timestamp[30];
#ifdef LOG_TIMESTAMPS
		sprintf(timestamp, "\n%lu[", HAL_GetTick());
#else
		sprintf(timestamp, "\n[");
#endif

		trace_AppendMsg(timestamp);

	}

	while ((count < rxSize) && (lpuart1_timeout > 0))
	{
		if (lpuart1_inbuf_tail != lpuart1_inbuf_head)
		{
			// Extract the next character to be processed
			ch = lpuart1_inbuf[lpuart1_inbuf_tail];
			rxBuff[count] = ch;
			count++;

			if (sd_EVM_460_Trace)
			{
			  trace_AppendCharAsHex(ch);
			}

			// Advance to next character in buffer (if any)
			lpuart1_inbuf_tail++;

			// Wrap (circular buffer)
			if (lpuart1_inbuf_tail >= sizeof(lpuart1_inbuf))
				lpuart1_inbuf_tail = 0;
		}
	}

#if 1
	//if (sd_EVM_460_Trace)
	//{
	//	trace_AppendMsg("]");   // omit closing square brace because EVM code does not output it
	//}
	if ((count < rxSize) && (lpuart1_timeout == 0))
	{
		char msg[30];
		sprintf(msg,"\nrx %lu before timeout\n", count);
		trace_AppendMsg(msg);
	}
#endif
	return count;
}

int lpuart1_getchar(uint8_t *pRxChar)
{
	if (lpuart1_inbuf_tail != lpuart1_inbuf_head)
	{
		// Extract the next character to be processed
		*pRxChar = lpuart1_inbuf[lpuart1_inbuf_tail];

		// Advance to next character in buffer (if any)
		lpuart1_inbuf_tail++;

		// Wrap (circular buffer)
		if (lpuart1_inbuf_tail == sizeof(lpuart1_inbuf))
			lpuart1_inbuf_tail = 0;

		return 1;
	}
	return 0;
}

void lpuart1_putchar(const uint8_t *TxChar)
{
	lpuart1_tx_bytes(TxChar, 1, 0);
}


#endif  // ENABLE_LPUART1
