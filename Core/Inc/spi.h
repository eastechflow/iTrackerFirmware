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

#ifndef __spi_H
#define __spi_H
#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"
#include "main.h"

extern unsigned char spi_txBuff[];
extern unsigned char spi_rxBuff[2];

extern SPI_HandleTypeDef hspi2;
extern void _Error_Handler(char *, int);
int8_t MX_SPI2_Init(void);

#ifdef __cplusplus
}
#endif
#endif
