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

#include "main.h"
#include "sdmmc.h"
#include "sd_diskio.h"
#include "bsp_driver_sd.h"
#include "gpio.h"
#include "fatfs.h"

SD_HandleTypeDef hsd1;

#if 0

#define FATFS_MKFS_ALLOWED 1
FATFS SDFatFs; /* File system object for SD card logical drive */
FIL MyFile; /* File object */
char SDPath[4]; /* SD card logical drive path */

typedef enum {
	APPLICATION_IDLE = 0, APPLICATION_RUNNING, APPLICATION_SD_UNPLUGGED,
} FS_FileOperationsTypeDef;

FS_FileOperationsTypeDef Appli_state = APPLICATION_IDLE;

uint8_t workBuffer[_MAX_SS];

int8_t sd_test(void) {
	uint8_t tmp8;
	FRESULT res; /* FatFs function common result code */
	uint32_t byteswritten, bytesread; /* File write/read counts */
	uint8_t wtext[] = "stm32l476g_eval : This is STM32 working with FatFs and uSD diskio driver"; /* File write buffer */
	uint8_t rtext[100]; /* File read buffer */

	/* 1- Link the micro SD disk I/O driver */
	tmp8 = FATFS_GetAttachedDriversNbr();
	if ((FATFS_LinkDriver(&SD_Driver, SDPath) == 0) || tmp8) {
		if (BSP_SD_IsDetected()) {

#if FATFS_MKFS_ALLOWED
			FRESULT res;

			res = f_mkfs(SDPath, FM_ANY, 0, workBuffer, sizeof(workBuffer));

			if (res != FR_OK) {
				return -1;
			}
#endif
			Appli_state = APPLICATION_RUNNING;
		}
	} else {
		return -1;
	}

	/* Register the file system object to the FatFs module */
	if (f_mount(&SDFatFs, (TCHAR const*) SDPath, 0) == FR_OK) {
		/* Create and Open a new text file object with write access */
		if (f_open(&MyFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
			/* Write data to the text file */
			res = f_write(&MyFile, wtext, sizeof(wtext), (void *) &byteswritten);

			if ((byteswritten > 0) && (res == FR_OK)) {
				/* Close the open text file */
				f_close(&MyFile);

				/* Open the text file object with read access */
				if (f_open(&MyFile, "STM32.TXT", FA_READ) == FR_OK) {
					/* Read data from the text file */
					res = f_read(&MyFile, rtext, sizeof(rtext), (void *) &bytesread);

					if ((bytesread > 0) && (res == FR_OK)) {
						/* Close the open text file */
						f_close(&MyFile);

						/* Compare read data with the expected data */
						if ((bytesread == byteswritten)) {
						}
					}
				}
			}
		}
	}

	return 0;
}

#endif

int8_t MX_SDMMC1_SD_Init(void)
{
	uint8_t sd_state = MSD_OK;

	hsd1.Instance                 = SDMMC1;
	hsd1.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
	hsd1.Init.ClockBypass         = SDMMC_CLOCK_BYPASS_DISABLE;
	hsd1.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	hsd1.Init.BusWide             = SDMMC_BUS_WIDE_1B;
	hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
	hsd1.Init.ClockDiv            = 0;

	if (BSP_SD_IsDetected() != SD_PRESENT)
	{
		return MSD_ERROR_SD_NOT_PRESENT;
	}

	if (SD_initialize(1) != HAL_OK)
	{
		sd_state = MSD_ERROR;
	}


	return sd_state;
}

void HAL_SD_MspInit(SD_HandleTypeDef* sdHandle)
{

	GPIO_InitTypeDef GPIO_InitStruct;
	if (sdHandle->Instance == SDMMC1)
	{
		__HAL_RCC_SDMMC1_CLK_ENABLE();

		GPIO_InitStruct.Pin       = GEN45_uSD_D0_Pin | GEN45_uSD_D1_Pin | GEN45_uSD_D2_Pin | GEN45_uSD_D3_Pin | GEN45_uSD_CLK_Pin;
		GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull      = GPIO_NOPULL;
		GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
		HAL_GPIO_Init(GEN45_uSD_D0_Port, &GPIO_InitStruct);



		GPIO_InitStruct.Pin       = GEN45_uSD_CMD_Pin;
		GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull      = GPIO_NOPULL;
		GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
		HAL_GPIO_Init(GEN45_uSD_CMD_Port, &GPIO_InitStruct);
	}
}

void HAL_SD_MspDeInit(SD_HandleTypeDef* sdHandle)
{

	if (sdHandle->Instance == SDMMC1)
	{
		/* Peripheral clock disable */
		__HAL_RCC_SDMMC1_CLK_DISABLE();
		HAL_GPIO_DeInit(GEN45_uSD_D0_Port, GEN45_uSD_D0_Pin | GEN45_uSD_D1_Pin | GEN45_uSD_D2_Pin | GEN45_uSD_D3_Pin | GEN45_uSD_CLK_Pin);
		HAL_GPIO_DeInit(GEN45_uSD_CMD_Port, GEN45_uSD_CMD_Pin);
	}
}

#define SD_NOT_INITIALIZED  0
#define SD_INITIALIZED      1
#define SD_REMOVED          2

static int sd_state = SD_NOT_INITIALIZED;

int8_t sd_init(void)  // returns 0 if success, non-zero otherwise
{
	if (sd_state == SD_REMOVED) return 3;
	if (sd_state == SD_INITIALIZED)
	{
		if (BSP_SD_IsDetected() != SD_PRESENT)
		{
			sd_state = SD_REMOVED;  // once removed, a power cycle is required to re-initialize
			return 3;
		}
		else
		{
			// Card found and previously initialized
			return 0;
		}
	}

	// Attempt to initialize, but just once per power-cycle

	if (MX_SDMMC1_SD_Init() != 0) return 1;
	if (MX_FATFS_Init() != 0) return 2;
	sd_state = SD_INITIALIZED;
	return 0;
}



