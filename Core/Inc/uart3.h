/*
 * uart3.h
 *
 *  Created on: Jul 3, 2021
 *      Author: Zachary
 */

#ifndef INC_UART3_H_
#define INC_UART3_H_

// CONFIGURATION



#define UART3_INBUF_SIZE 		256
#define UART3_OUTBUF_SIZE 		256


// Input buffer
extern uint8_t uart3_inbuf[UART3_INBUF_SIZE];
extern volatile uint16_t uart3_inbuf_head;
extern volatile uint16_t uart3_inbuf_tail;
extern volatile uint16_t uart3_inbuf_max;


// Output buffer.

extern uint8_t uart3_outbuf[UART3_OUTBUF_SIZE];
extern volatile uint16_t uart3_outbuf_head;
extern volatile uint16_t uart3_outbuf_tail;
extern volatile uint16_t uart3_outbuf_max;


extern __IO uint32_t uart3_timeout;
extern __IO uint32_t uart3_parse_rx;
extern __IO uint32_t uart3_parse_rx_complete;

void uart3_flush_inbuffer();
int  uart3_tx_bytes(const uint8_t *txBuff, uint32_t txSize, uint8_t trace);
int  uart3_tx_string(const char * txBuff);

void uart3_enter_low_power_mode(void);

int  uart3_rx_bytes(uint8_t *rxBuff, uint32_t rxSize, uint32_t timeout_ms);
void uart3_loopback(void);
void uart3_trace_ch(uint8_t ch);


#endif /* INC_UART3_H_ */
