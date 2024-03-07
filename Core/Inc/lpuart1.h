/*
 * lplpuart1.h
 *
 *  Created on: Aug 16, 2021
 *      Author:
 */

#ifndef INC_LPUART1_H_
#define INC_LPUART1_H_

#include "usart.h"   // has define of ENABLE_LPUART1  TODO maybe fix this better?

#ifdef ENABLE_LPUART1

extern UART_HandleTypeDef hlpuart1;    // Velocity 2 (a.k.a. UART5)


// ISR Buffers
//#define LPUART1_OUTBUF_SIZE    256
//#define LPUART1_INBUF_SIZE     256
#define LPUART1_OUTBUF_SIZE    512
#define LPUART1_INBUF_SIZE     512
extern char lpuart1_inbuf[LPUART1_INBUF_SIZE];
extern volatile uint16_t lpuart1_inbuf_head;
extern volatile uint16_t lpuart1_inbuf_tail;
extern volatile uint16_t lpuart1_inbuf_max;

extern char lpuart1_outbuf[LPUART1_OUTBUF_SIZE];
extern volatile uint16_t lpuart1_outbuf_head;
extern volatile uint16_t lpuart1_outbuf_tail;
extern volatile uint16_t lpuart1_outbuf_max;

extern __IO uint32_t lpuart1_timeout;

int  lpuart1_tx(uint32_t txSize, uint8_t trace);
int  lpuart1_tx_bytes(const uint8_t *txBuff, uint32_t txSize, uint8_t trace);
int  lpuart1_tx_string(const char * txBuff);

int  lpuart1_getchar(uint8_t *pRxChar);
void lpuart1_putchar(const uint8_t *TxChar);

void lpuart1_flush_inbuffer();
int  lpuart1_rx_bytes(uint8_t *rxBuff, uint32_t rxSize, uint32_t timeout_ms);


#endif

#endif /* INC_LPUART1_H_ */
