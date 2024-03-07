/*
 * uart1.h
 *
 *  Created on: Feb 28, 2020
 *      Author: Zachary
 */

#ifndef INC_UART1_H_
#define INC_UART1_H_

#include "version.h"


// Cellular USART handle
extern UART_HandleTypeDef huart1;



// Response Codes
#define RC_NONE     0
#define RC_OK       1
#define RC_CONNECT  2
#define RC_ERROR    3
#define RC_TIMEOUT  4
#define RC_JSON_OPEN      5
#define RC_JSON_CLOSE     6
#define RC_KSUP           7
#define RC_NO_CARRIER     8

// ISR Receive Buffer CELL
//#define UART1_OUTBUF_SIZE 	 1024
//#define UART1_INBUF_SIZE     1024
#define UART1_OUTBUF_SIZE 	 512
#define UART1_INBUF_SIZE     512

extern uint8_t uart1_inbuf[UART1_INBUF_SIZE];
extern volatile uint16_t uart1_inbuf_head;
extern volatile uint16_t uart1_inbuf_tail;
extern volatile uint16_t uart1_inbuf_max;

extern char uart1_outbuf[UART1_OUTBUF_SIZE];
extern volatile uint16_t uart1_outbuf_head;
extern volatile uint16_t uart1_outbuf_tail;
extern volatile uint16_t uart1_outbuf_max;

extern __IO uint32_t uart1_timeout;
extern __IO uint32_t cell_response_code_received;
extern __IO uint32_t cell_urc_received;
extern __IO uint32_t uart1_json_string_received;

int uart1_tx(uint32_t txSize);

void uart1_flush_inbuffer();

int uart1_tx_bytes(const uint8_t *txBuff, uint32_t txSize);
int uart1_tx_string(const char * txBuff);

int  uart1_getchar(uint8_t *pRxChar);
void uart1_putchar(const uint8_t *TxChar);

int  cell_send_string(const char * txBuff);

#endif /* INC_UART1_H_ */
