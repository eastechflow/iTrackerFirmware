/*
 * uart2.h
 *
 *  Created on: Apr 11, 2020
 *      Author: Zachary
 */

#ifndef INC_UART2_H_
#define INC_UART2_H_

// CONFIGURATION


#define WIFI_BUF_SIZE 		1024    // zg - note that there are several temporary stack-allocated buffers of this size.  Better to bypass and use uart2_outbuf directly?
#define UART2_INBUF_SIZE   1024
#define UART2_OUTBUF_SIZE  1024


// Input buffer
extern uint8_t uart2_inbuf[UART2_INBUF_SIZE];
extern volatile uint16_t uart2_inbuf_head;
extern volatile uint16_t uart2_inbuf_tail;
extern volatile uint16_t uart2_inbuf_max;

//extern int zentri_response_len;

// Output buffer.
extern uint8_t uart2_outbuf[UART2_OUTBUF_SIZE];
extern volatile uint16_t uart2_outbuf_head;
extern volatile uint16_t uart2_outbuf_tail;
extern volatile uint16_t uart2_outbuf_max;

extern volatile uint32_t uart2_tx_start;
extern volatile uint32_t uart2_tx_end;

extern volatile uint8_t uart2_XON_XOFF_state;


extern __IO uint32_t uart2_timeout;
extern __IO uint32_t uart2_previous_tx_wait_time_ms;

extern int uart2_XON_XOFF_enabled;

#define ESP32_VERSION_1_3    0   // was installed in first 200 units
#define ESP32_VERSION_1_4    1   // only used in R&D testing
#define ESP32_VERSION_1_5    2   // only used in R&D testing
#define ESP32_VERSION_1_6    3   // only used in R&D testing

extern int uart2_esp32_version;

extern uint32_t esp32_serial_rx_buffer_size;
extern uint32_t esp32_payload_size;
extern uint32_t esp32_payload_spacing_ms;

void uart2_set_tx_parameters(void);



int uart2_tx_outbuf(void);  // a null-terminated string must already be in uart2_outbuf

int  uart2_tx_bytes(const uint8_t *txBuff, uint32_t txSize, uint8_t trace);
int  uart2_tx_string(const char * txBuff);
void uart2_flush_inbuffer(void);
//void uart2_flush_outbuffer(void);

int  uart2_getchar(uint8_t *pRxChar);
void uart2_putchar(const uint8_t *TxChar);
void uart2_loopback(void);



#endif /* INC_UART2_H_ */
