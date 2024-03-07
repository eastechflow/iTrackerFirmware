/*************************************************************************
* COPYRIGHT - All Rights Reserved.
* 2019 Eastech Flow Controls, Inc.
*
* OWNER:
* Eastech Flow Controls, Inc.
* 4250 S. 76th E. Avenue
* Tulsa, Oklahoma 74145
*
* toll-free: 	(800) 226-3569
* phone: 	(918) 664-1212
* fax: 	(918) 664-8494
* email: 	info@eastechflow.com
* website: 	http://eastechflow.com/
*
* OBLIGATORY: All information contained herein is, and remains
* the property of EASTECH FLOW CONTROLS, INC. The intellectual
* and technical concepts contained herein are proprietary to
* EASTECH FLOW CONTROLS, INC. and its suppliers and may be covered
* by U.S. and Foreign Patents, patents in process, and are protected
* by trade secret or copyright law.
*
* Dissemination, transmission, forwarding or reproduction of this
* content in any way is strictly forbidden.
*************************************************************************/

#ifndef USART_H
#define USART_H

#include "stm32l4xx_hal.h"



#define ENABLE_UART4
#define ENABLE_LPUART1
#define ENABLE_PGA460


//#define USE_UART1_AND_LPUART1   // for SewerWatch boards the PGA460 is connected to LPUART1, use CELL (UART1) to talk to pc
#define USE_UART4_AND_LPUART1   // for SewerWatch boards the PGA460 is connected to LPUART1, use VEL1 (UART4) to talk to pc
//#define USE_UART1_AND_UART3
//#define USE_UART4_AND_UART3



#ifdef ENABLE_PGA460
#define ENABLE_PGA460_USE_UART3
#endif

extern UART_HandleTypeDef huart1;  // cell modem
extern UART_HandleTypeDef huart2;  // wifi modem
extern UART_HandleTypeDef huart3;  // Level - RS-485 - PGA460


#define XON  17  // DC1
#define XOFF 19  // DC3

#define GOT_HERE  18 // DC2


extern void _Error_Handler(char *, int);


void MX_USART1_UART_Init(uint32_t BaudRate, uint32_t StopBits, uint32_t HwFlowCtl);   // cell
void MX_USART2_UART_Init(uint32_t BaudRate, uint32_t StopBits, uint32_t HwFlowCtl);   // WiFi
void MX_USART3_UART_Init(uint32_t BaudRate, uint32_t StopBits);                       // Level (RS-485)

#ifdef ENABLE_UART4
extern UART_HandleTypeDef huart4;                                                     // Velocity 1
void MX_UART4_UART_Init(uint32_t BaudRate, uint32_t StopBits);
#endif

#ifdef ENABLE_LPUART1
extern UART_HandleTypeDef hlpuart1;                                                    // Velocity 2 or SewerWatch PG460
void MX_LPUART1_UART_Init(uint32_t BaudRate, uint32_t StopBits);
#endif

void check_and_report_uart_buffer_overflow(void);


#endif
