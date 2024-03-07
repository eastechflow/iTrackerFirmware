/*
 * uart3.c
 *
 *  Created on: Jul 3, 2021
 */

#include "stm32l4xx_hal.h"
#include "usart.h"
#include "uart3.h"


#include "trace.h"
#include "factory.h"
#include "debug_helper.h"


// ISR Receive Buffer
uint8_t uart3_inbuf[UART3_INBUF_SIZE];
volatile uint16_t uart3_inbuf_head = 0;
volatile uint16_t uart3_inbuf_tail = 0;
volatile uint16_t uart3_inbuf_max = 0;


// ISR Transmit Buffer
uint8_t uart3_outbuf[UART3_OUTBUF_SIZE];
volatile uint16_t uart3_outbuf_head = 0;
volatile uint16_t uart3_outbuf_tail = 0;
volatile uint16_t uart3_outbuf_max  = 0;

__IO uint32_t uart3_timeout = 0;


static int uart3_rx_active = 1;

void USART3_IRQHandler(void)
{
	uint32_t errorflags;
	uint32_t isrflags = READ_REG(huart3.Instance->ISR);
	errorflags = (isrflags & (uint32_t) (USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));

	if (errorflags != RESET)
	{
		if ((isrflags & USART_ISR_ORE) != RESET)
		{
			__HAL_UART_CLEAR_IT(&huart3, UART_CLEAR_OREF);
		}
		if ((isrflags & USART_ISR_FE) != RESET)
		{
			__HAL_UART_CLEAR_IT(&huart3, UART_CLEAR_FEF);
		}
	}

	if ((USART3->ISR & USART_ISR_RXNE) == USART_ISR_RXNE)
	{
		// Read from the UART.
		uint8_t ch = (uint8_t)(USART3->RDR);

		//if (sd_EVM_460_Enabled) trace_AppendChar(ch);  // ISR ch logging
		//if (count == (4*20)) trace_SaveBuffer();  // ISR ch logging

		if (uart3_rx_active)
		{
		  uart3_inbuf[uart3_inbuf_head] = ch;  // save the incoming character in the array
		  uart3_inbuf_head++;             // increment to next position
		  if (uart3_inbuf_head > uart3_inbuf_max) uart3_inbuf_max = uart3_inbuf_head;  // Keep track of high watermark
		}


		// Check for overflow.
		if (uart3_inbuf_head >= sizeof(uart3_inbuf))
		{
			uart3_inbuf_head = 0;
		}

		// Check if head has overtaken tail --- that means the firmware has not been able to keep up
		// Not sure what to do about that

		// Always maintain a null-terminated input buffer for parsing and logging contents of RX buffer.
		// A buffer overflow will lose the entire buffer.
		uart3_inbuf[uart3_inbuf_head] = 0;
	}
	else if ((USART3->ISR & USART_ISR_TC) == USART_ISR_TC)
	{
		if (uart3_outbuf_tail < uart3_outbuf_head)
		{
			//if (sd_EVM_460_Enabled) trace_AppendChar(uart3_outbuf[uart3_outbuf_tail]);  // ISR ch logging
			//if (count == (4*20)) trace_SaveBuffer();  // ISR ch logging

			// Transmit next character
			USART3->TDR = uart3_outbuf[uart3_outbuf_tail];
			uart3_outbuf_tail++;
		}
		else
		{
			// Clear the transmit complete and enable RX
			USART3->ICR = USART_ICR_TCCF;
			uart3_outbuf_head = 0;
			uart3_outbuf_tail = 0;
			uart3_rx_active = 1;
			HAL_GPIO_WritePin(GEN45_RS485_nRE_Port, GEN45_RS485_nRE_Pin, GPIO_PIN_RESET);  // enable RX (active low)
		}
	}
}

void uart3_flush_inbuffer()
{
	//for (int i = 0; i < sizeof(uart3_inbuf); i++)
	//{
	//	uart3_inbuf[i] = '\0';
	//}
	NVIC_DisableIRQ(USART3_IRQn);
	uart3_inbuf_head = 0;
	uart3_inbuf_tail = 0;
	uart3_inbuf[0] = '\0';
	NVIC_EnableIRQ(USART3_IRQn);
}

#if 0
void uart3_flush_outbuffer()
{
	for (int i = 0; i < sizeof(uart3_outbuf); i++)
	{
		uart3_outbuf[i] = '\0';
	}
	uart3_outbuf_head = 0;
	uart3_outbuf_tail = 0;
}
#endif



uint32_t uart3_TX_byte_count;
uint32_t uart3_TX_max_size = 0;


int uart3_tx(uint32_t txSize, uint8_t trace)
{

	uart3_rx_active = 0;
	HAL_GPIO_WritePin(GEN45_RS485_nRE_Port, GEN45_RS485_nRE_Pin, GPIO_PIN_SET);  // disable RX (active low)


	// Update the buffer indices.
	uart3_outbuf_head = txSize;        // this is used to stop TX
	uart3_outbuf_tail = 0;             // this is used to TX each character in order

	if (uart3_outbuf_head > uart3_outbuf_max) uart3_outbuf_max = uart3_outbuf_head;  // Keep track of high watermark

	// Start the transmission
	if ((USART3->ISR & USART_ISR_IDLE) == USART_ISR_IDLE)  // zg - why is this needed?
	{
		if ((USART3->ISR & USART_ISR_TXE) == USART_ISR_TXE)  // zg - why is this needed?
		{
			// Transmit - fires the first one. The ISR takes over from there.
			USART3->TDR = uart3_outbuf[uart3_outbuf_tail]; // Send the first char to start.
			uart3_outbuf_tail++;  // point to next char to transmit
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

	uart3_timeout = 2000;  // TX should start within 2000 ms

	while (uart3_outbuf_head != 0)   // wait for all characters to transmit
	{
		led_heartbeat();

		// Check that a transmit starts within the timeout period
		if (uart3_outbuf_tail == 1)  // no character has been sent yet
		{
			if (uart3_timeout == 0)  // TX start timeout expired
			{
				// Clear the transmit.
				//USART3->ICR = USART_ICR_TCCF;  // zg - let the ISR do this
				uart3_outbuf_head = 0;
				uart3_outbuf_tail = 0;
				if (trace)
				{
					trace_AppendMsg("\nTX Error: timeout\n");
				}
				uart3_rx_active = 1;
				HAL_GPIO_WritePin(GEN45_RS485_nRE_Port, GEN45_RS485_nRE_Pin, GPIO_PIN_RESET);  // enable RX (active low)

				return 0;  // TX was not started, string not sent
			}
		}
	}

	if (trace)
	{
		int i;
		for (i=0; i < txSize; i++)
		{
		  trace_AppendChar(uart3_outbuf[i]);
		}
	}

	uart3_TX_byte_count += txSize;  // count all bytes transmitted
	if (txSize > uart3_TX_max_size) uart3_TX_max_size = txSize;  // keep track of largest packet

	return txSize; // TX completed OK
}


int uart3_tx_bytes(const uint8_t *txBuff, uint32_t txSize, uint8_t trace)
{
	// Make sure the array isn't empty and send.
	if (txSize == 0) return 0; // nothing to send

	// Copy the input array into the ISR output buffer.
	if (txSize > sizeof(uart3_outbuf)) txSize = sizeof(uart3_outbuf);

	for (int i = 0; i < txSize; i++)
	{
		uart3_outbuf[i] = txBuff[i];
	}

	return uart3_tx(txSize, trace);
}



// Sends the string to UART3
int uart3_tx_string(const char * txBuff)
{
	uint32_t txSize;

	txSize = strlen(txBuff);

	return uart3_tx_bytes((const uint8_t *)txBuff, txSize, sd_EVM_1000);

}

void uart3_enter_low_power_mode(void)
{
	HAL_GPIO_WritePin(GEN45_RS485_DE_Port, GEN45_RS485_DE_Pin, GPIO_PIN_RESET);  // disable TX here for low-power mode
	HAL_GPIO_WritePin(GEN45_RS485_nRE_Port, GEN45_RS485_nRE_Pin, GPIO_PIN_SET);    // disable RX here for low-power mode
}

int uart3_rx_bytes(uint8_t *rxBuff, uint32_t rxSize, uint32_t timeout_ms)
{
	uint32_t count = 0;
	uint8_t ch;

	uart3_timeout = timeout_ms;

	while ((count < rxSize) && (uart3_timeout > 0))
	{
		while (uart3_inbuf_tail != uart3_inbuf_head)
		{
			// Extract the next character to be processed
			ch = uart3_inbuf[uart3_inbuf_tail];
			rxBuff[count] = ch;
			count++;

			// Advance to next character in buffer (if any)
			uart3_inbuf_tail++;

			// Wrap (circular buffer)
			if (uart3_inbuf_tail >= sizeof(uart3_inbuf))
				uart3_inbuf_tail = 0;
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

void uart3_trace_ch(uint8_t ch)
{
#if 0
	if (sd_EVM_460_Trace)
	{
		char msg[20];

		sprintf(msg,"\n %X", ch);  // add space to classify as RX in trace file
		trace_AppendMsg(msg);
	}
#endif
}


