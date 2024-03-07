/*
 * uart4.h
 *
 *  Created on: Aug 16, 2021
 *      Author:
 */

#ifndef INC_UART4_H_
#define INC_UART4_H_

#ifdef ENABLE_UART4

extern UART_HandleTypeDef huart4;    // Velocity 1


// ISR Buffers
#define UART4_OUTBUF_SIZE    512  // 256
#define UART4_INBUF_SIZE     512  // 256
extern uint8_t uart4_inbuf[UART4_INBUF_SIZE];
extern volatile uint16_t uart4_inbuf_head;
extern volatile uint16_t uart4_inbuf_tail;
extern volatile uint16_t uart4_inbuf_max;

extern char uart4_outbuf[UART4_OUTBUF_SIZE];
extern volatile uint16_t uart4_outbuf_head;
extern volatile uint16_t uart4_outbuf_tail;
extern volatile uint16_t uart4_outbuf_max;

extern __IO uint32_t uart4_timeout;

int uart4_tx(uint32_t txSize, uint8_t trace);
int uart4_tx_bytes(const uint8_t *txBuff, uint32_t txSize, uint8_t trace);
int uart4_tx_string(const char * txBuff);

void uart4_flush_inbuffer();
int  uart4_rx_bytes(uint8_t *rxBuff, uint32_t rxSize, uint32_t timeout_ms);
int  uart4_getchar(uint8_t *pRxChar);
void uart4_putchar(const uint8_t *TxChar);


// These are used for bootloading the ESP32; or as a direct pass-thru

//#define ESP32_RECORD_FILE
// ESP32_RECORD_FILE did not allow the bootloader from the pc to work; have to record session on pc side
// using Elitima Software "Serial Port Monitor".
//could record the RX chars from ESP32 without messing up the bootload,  but could not record the TX chars going down.  The bootload would fail
//

void uart2_to_uart4(void);
void uart4_to_uart2(void);


#endif

#endif /* INC_UART4_H_ */
