/*
 * uart4.c
 *
 *  Created on: Aug 16, 2021
 *      Author:
 */

#include "stm32l4xx_hal.h"
#include "usart.h"
#include "uart4.h"


#include "trace.h"
#include "factory.h"
#include "debug_helper.h"

#ifdef ENABLE_UART4


extern UART_HandleTypeDef huart4;

// ISR Receive Buffer
//uint8_t uart4_inbuf[UART4_BUF_SIZE];
uint8_t uart4_inbuf[UART4_INBUF_SIZE];
volatile uint16_t uart4_inbuf_head = 0;
volatile uint16_t uart4_inbuf_tail = 0;
volatile uint16_t uart4_inbuf_max  = 0;


// ISR Transmit Buffer
//uint8_t uart4_outbuf[UART4_BUF_SIZE];
char uart4_outbuf[UART4_OUTBUF_SIZE];
volatile uint16_t uart4_outbuf_head = 0;
volatile uint16_t uart4_outbuf_tail = 0;
volatile uint16_t uart4_outbuf_max  = 0;

__IO uint32_t uart4_timeout = 0;


void UART4_IRQHandler(void)
{
	uint32_t errorflags;
	uint32_t isrflags = READ_REG(huart4.Instance->ISR);
	errorflags = (isrflags & (uint32_t) (USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));

	if (errorflags != RESET)
	{
		if ((isrflags & USART_ISR_ORE) != RESET)
		{
			__HAL_UART_CLEAR_IT(&huart4, UART_CLEAR_OREF);
		}
		if ((isrflags & USART_ISR_FE) != RESET)
		{
			__HAL_UART_CLEAR_IT(&huart4, UART_CLEAR_FEF);
		}
	}

	if ((UART4->ISR & USART_ISR_RXNE) == USART_ISR_RXNE)
	{
		// Read from the UART.
		uint8_t ch = (uint8_t)(UART4->RDR);

		//if (sd_EVM_1000_Enabled) trace_AppendChar(ch);  // ISR ch logging
		//if (count == (4*20)) trace_SaveBuffer();  // ISR ch logging


		uart4_inbuf[uart4_inbuf_head] = ch;  // save the incoming character in the array
		uart4_inbuf_head++;             // increment to next position

		if (uart4_inbuf_head > uart4_inbuf_max) uart4_inbuf_max = uart4_inbuf_head;  // Keep track of high watermark


		// Check for overflow.
		if (uart4_inbuf_head >= sizeof(uart4_inbuf))
		{
			uart4_inbuf_head = 0;
		}

		// Check if head has overtaken tail --- that means the firmware has not been able to keep up
		// Not sure what to do about that

		// Always maintain a null-terminated input buffer for parsing and logging contents of RX buffer.
		// A buffer overflow will lose the entire buffer.
		uart4_inbuf[uart4_inbuf_head] = 0;
	}
	else if ((UART4->ISR & USART_ISR_TC) == USART_ISR_TC)
	{
		if (uart4_outbuf_tail < uart4_outbuf_head)
		{

			//if (sd_EVM_1000_Enabled) trace_AppendChar(uart4_outbuf[uart4_outbuf_tail]);  // ISR ch logging
			//if (count == (4*20)) trace_SaveBuffer();  // ISR ch logging
			// Transmit next character
			UART4->TDR = uart4_outbuf[uart4_outbuf_tail];
			uart4_outbuf_tail++;
		}
		else
		{
			// Clear the transmit complete and enable RX
			UART4->ICR = USART_ICR_TCCF;
			uart4_outbuf_head = 0;
			uart4_outbuf_tail = 0;

		}
	}
}

void uart4_flush_inbuffer()
{
	//for (int i = 0; i < sizeof(uart4_inbuf); i++)
	//{
	//	uart4_inbuf[i] = '\0';
	//}
	NVIC_DisableIRQ(UART4_IRQn);
	uart4_inbuf_head = 0;
	uart4_inbuf_tail = 0;
	uart4_inbuf[0] = '\0';
	NVIC_EnableIRQ(UART4_IRQn);
}

#if 0
void uart4_flush_outbuffer()
{
	for (int i = 0; i < sizeof(uart4_outbuf); i++)
	{
		uart4_outbuf[i] = '\0';
	}
	uart4_outbuf_head = 0;
	uart4_outbuf_tail = 0;
}
#endif



uint32_t uart4_TX_byte_count;
uint32_t uart4_TX_max_size = 0;


int uart4_tx(uint32_t txSize, uint8_t trace)
{

	// Update the buffer indices.
	uart4_outbuf_head = txSize;        // this is used to stop TX
	uart4_outbuf_tail = 0;                 // this is used to TX each character in order

	if (uart4_outbuf_head > uart4_outbuf_max) uart4_outbuf_max = uart4_outbuf_head;  // Keep track of high watermark

	// Start the transmission
	if ((UART4->ISR & USART_ISR_IDLE) == USART_ISR_IDLE)  // zg - why is this needed?
	{
		if ((UART4->ISR & USART_ISR_TXE) == USART_ISR_TXE)  // zg - why is this needed?
		{
			// Transmit - fires the first one. The ISR takes over from there.
			UART4->TDR = uart4_outbuf[uart4_outbuf_tail]; // Send the first char to start.
			uart4_outbuf_tail++;
		}
		else
		{
			if (trace)
			{
				trace_AppendMsg("\nTX Error: TXE\n");
			}
		}
	}
	else
	{
		if (trace)
		{
			trace_AppendMsg("\nTX Error: IDLE\n");
		}
	}


	// Must confirm the transmit completed before proceeding.
	// The intention is to block until transmission is done.
	// Use an escape mechanism to avoid getting trapped here.
	//TODO may need two timeouts:  1) to start TX, 2) to complete TX.  Only start TX is implemented

	uart4_timeout = 2000;  // TX should start within 2000 ms

	while (uart4_outbuf_head != 0)   // wait for all characters to transmit
	{
		led_heartbeat();

		// Check that a transmit starts within the timeout period
		if (uart4_outbuf_tail == 1)  // no character has been sent yet
		{
			if (uart4_timeout == 0)  // TX start timeout expired
			{
				// Clear the transmit.
				//UART4->ICR = USART_ICR_TCCF;  // zg - let the ISR do this
				uart4_outbuf_head = 0;
				uart4_outbuf_tail = 0;
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
		  trace_AppendChar(uart4_outbuf[i]);
		}
	}

	uart4_TX_byte_count += txSize;  // count all bytes transmitted
	if (txSize > uart4_TX_max_size) uart4_TX_max_size = txSize;  // keep track of largest TX packet

	return txSize; // TX completed OK
}


int uart4_tx_bytes(const uint8_t *txBuff, uint32_t txSize, uint8_t trace)
{
	// Make sure the array isn't empty and send.
	if (txSize == 0) return 0; // nothing to send
	if (txSize > sizeof(uart4_outbuf)) txSize = sizeof(uart4_outbuf);

	// Copy the input array into the ISR output buffer.

	for (int i = 0; i < txSize; i++)
	{
		uart4_outbuf[i] = txBuff[i];
	}

	return uart4_tx(txSize, trace);
}



// Sends the string to UART4
int uart4_tx_string(const char * txBuff)
{
	uint32_t txSize;

	txSize = strlen(txBuff);

	return uart4_tx_bytes((const uint8_t *)txBuff, txSize, sd_EVM_460_Trace);

}



int uart4_rx_bytes(uint8_t *rxBuff, uint32_t rxSize, uint32_t timeout_ms)
{
	uint32_t count = 0;
	uint8_t ch;

	uart4_timeout = timeout_ms;

	while ((count < rxSize) && (uart4_timeout > 0))
	{
		while (uart4_inbuf_tail != uart4_inbuf_head)
		{
			// Extract the next character to be processed
			ch = uart4_inbuf[uart4_inbuf_tail];
			rxBuff[count] = ch;
			count++;

			// Advance to next character in buffer (if any)
			uart4_inbuf_tail++;

			// Wrap (circular buffer)
			if (uart4_inbuf_tail >= sizeof(uart4_inbuf))
				uart4_inbuf_tail = 0;
		}
	}
	if (count == 0)
	{
		if (sd_EVM_460_Trace)
		{
			char msg[30];
			sprintf(msg,"\nrx timeout\n");
			trace_AppendMsg(msg);
		}
	}
	return count;
}

int uart4_getchar(uint8_t *pRxChar)
{
	if (uart4_inbuf_tail != uart4_inbuf_head)
	{
		// Extract the next character to be processed
		*pRxChar = uart4_inbuf[uart4_inbuf_tail];

		// Advance to next character in buffer (if any)
		uart4_inbuf_tail++;

		// Wrap (circular buffer)
		if (uart4_inbuf_tail == sizeof(uart4_inbuf))
			uart4_inbuf_tail = 0;

		return 1;
	}
	return 0;
}

void uart4_putchar(const uint8_t *TxChar)
{
	uart4_tx_bytes(TxChar, 1, 0);
}


#endif  // ENABLE_UART4
