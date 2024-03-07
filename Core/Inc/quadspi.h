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

#ifndef __quadspi_H
#define __quadspi_H
#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"
#include "main.h"

extern QSPI_HandleTypeDef QSPIHandle;

/* USER CODE BEGIN Private defines */
/* QSPI Error codes */
#define QSPI_OK            ((uint8_t)0x00)
#define QSPI_ERROR         ((uint8_t)0x01)
#define QSPI_BUSY          ((uint8_t)0x02)
#define QSPI_NOT_SUPPORTED ((uint8_t)0x04)
#define QSPI_SUSPENDED     ((uint8_t)0x08)

extern void _Error_Handler(char *, int);



/* End address of the QSPI memory */
//#define QSPI_END_ADDR              (1 << QSPI_FLASH_SIZE)

//int8_t BSP_QSPI_DeInit(void);
//int8_t BSP_QSPI_Erase_Block(uint32_t BlockAddress);
//int8_t BSP_QSPI_GetInfo(QSPI_Info* pInfo);
//int8_t BSP_QSPI_EnableMemoryMappedMode(void);

int8_t BSP_QSPI_Init(void);

int8_t BSP_QSPI_Read(uint8_t* pData, uint32_t ReadAddr, uint32_t Size);
int8_t BSP_Log_Track_Write(int batt_info);
int8_t BSP_Log_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
int8_t BSP_QSPI_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size);

int8_t BSP_QSPI_Erase_Sector(uint32_t BlockAddress);
int8_t BSP_QSPI_Erase_Sector2(uint32_t BlockAddress, uint32_t Instruction);
int8_t BSP_QSPI_Erase_Chip(void);




#ifdef __cplusplus
}
#endif
#endif
