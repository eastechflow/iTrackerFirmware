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

#include "quadspi.h"
#include "flash_regs.h"
#include "s25fl.h"
#include "gpio.h"
#include "Log.h"
#include "Common.h"

QSPI_HandleTypeDef QSPIHandle;




int8_t BSP_QSPI_Init(void)
{
	QSPIHandle.Instance = QUADSPI;

#if 0  // zg - do not de-init before init
	/* Call the DeInit function to reset the driver */
	if (HAL_QSPI_DeInit(&QSPIHandle) != HAL_OK)
	{
		//return QSPI_ERROR;
		return -1;
	}
#endif

	/* System level initialization */
	/* QSPI initialization */
	QSPIHandle.Init.ClockPrescaler     = 1;
	QSPIHandle.Init.FifoThreshold      = 4;
	QSPIHandle.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_NONE;
	QSPIHandle.Init.FlashSize          = 23;
	QSPIHandle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
	QSPIHandle.Init.ClockMode          = QSPI_CLOCK_MODE_0;

	if (HAL_QSPI_Init(&QSPIHandle) != HAL_OK)
	{
		return -2;
	}

	return QSPI_OK;
}

#if 0
/**
 * @brief  De-Initializes the QSPI interface.
 * @retval QSPI memory status
 */
int8_t BSP_QSPI_DeInit(void) {
	QSPIHandle.Instance = QUADSPI;

	/* Call the DeInit function to reset the driver */
	if (HAL_QSPI_DeInit(&QSPIHandle) != HAL_OK) {
		return -1;
	}

	/* System level De-initialization */
	return QSPI_OK;
}
#endif
/**
 * @brief  Reads current status of the QSPI memory.
 * @retval QSPI memory status
 */
static void Read_SR1(uint8_t * pReg)
{
	HAL_StatusTypeDef HAL_status;
	QSPI_CommandTypeDef sCommand;

	/* Initialize the read status register 1 command - zg */
	sCommand.Instruction       = FLASH_CMD_RDSR1;
	sCommand.Address           = 0;
	sCommand.AlternateBytes    = 0;
	sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
	sCommand.AlternateBytesSize= 0;
	sCommand.DummyCycles       = 0;
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_1_LINE;
	sCommand.NbData            = 1;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;


	while (1)
	{
		/* Send the command */
		HAL_status = HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
		if (HAL_status == HAL_OK)
		{
			/* Receive the data */
			HAL_status = HAL_QSPI_Receive(&QSPIHandle, pReg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
			if (HAL_status == HAL_OK)
			{
				return;
			}
		}

		// An unexpected SPI error has occurred --- attempt to reset the SPI on our side?

		_Error_Handler(__FILE__, __LINE__);

		BSP_QSPI_Init();

	}

	// We should never get here...

}

static void ResetMemory(QSPI_HandleTypeDef *hqspi)
{

	QSPI_CommandTypeDef sCommand;

	// This function attempts to reset the flash device.
	// 1. Send BMR (Bit mode reset)
	// 2. Send RESET (software reset)

	/* Initialize command - zg */
	sCommand.Instruction       = FLASH_CMD_MBR;
	sCommand.Address           = 0;
	sCommand.AlternateBytes    = 0;
	sCommand.AddressSize       = 0;
	sCommand.AlternateBytesSize= 0;
	sCommand.DummyCycles       = 0;
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_NONE;
	sCommand.NbData            = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);

	HAL_Delay(1);


	/* Initialize command - zg */
	sCommand.Instruction       = FLASH_CMD_RESET;
	sCommand.Address           = 0;
	sCommand.AlternateBytes    = 0;
	sCommand.AddressSize       = 0;
	sCommand.AlternateBytesSize= 0;
	sCommand.DummyCycles       = 0;
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_NONE;
	sCommand.NbData            = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);

	HAL_Delay(1);

}

static void WaitUntilDeviceIsIdle(void)
{

	uint8_t sr1;

	while (1)
	{

		// Read the SR1 register - this is specifically allowed at any time
		Read_SR1(&sr1);


		// Check for errors on the flash chip
		if (sr1 & (SR1_E_ERR | SR1_P_ERR))
		{
			// Clear error condition so chip will respond to future commands
			ResetMemory(&QSPIHandle);
		}
		else
		{
			// Check if device is idle
			if ((sr1 & SR1_WIP) == 0)
			{
				// device is idle
				return;
			}
		}

		// Provide some sort of indication that things are in progress..
		// Blink_LED();

	}

	// We should never get here...
}

static HAL_StatusTypeDef sendWREN(void)
{

	QSPI_CommandTypeDef sCommand;

	/* Initialize WREN cmd - zg */
	sCommand.Instruction       = FLASH_CMD_WREN;
	sCommand.Address           = 0;
	sCommand.AlternateBytes    = 0;
	sCommand.AddressSize       = 0;
	sCommand.AlternateBytesSize= 0;
	sCommand.DummyCycles       = 0;
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_NONE;
	sCommand.NbData            = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;


	return HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);

}

#if 0
// zg - in practice this defeats the WREN command...
static void WaitUntilWriteIsEnabled(void)
{

	uint8_t sr1;

	while (1)
	{

		// Read the SR1 register - this is specifically allowed at any time
		Read_SR1(&sr1);


		// Check for errors on the flash chip
		if (sr1 & (SR1_E_ERR | SR1_P_ERR))
		{
			// Clear error condition so chip will respond to future commands
			ResetMemory(&QSPIHandle);

			WaitUntilDeviceIsIdle();

			sendWREN();  // send the WREN command again
		}
		else
		{
			// Check if device is enabled for writing
			if ((sr1 & SR1_WEL) == 0)
			{
				// device is ready for writing
				return;
			}
		}

		// Provide some sort of indication that things are in progress..
		// Blink_LED();

	}

	// We should never get here...
}
#endif

/**
 * @brief  This function sends a Write Enable and waits until it is effective.
 * @param  hqspi: QSPI handle
 * @retval None
 */
static void WriteEnable(QSPI_HandleTypeDef *hqspi)
{

	WaitUntilDeviceIsIdle();

	sendWREN();

	//WaitUntilWriteIsEnabled();
}



/**
 * @brief  Reads an amount of data from the QSPI memory.
 * @param  pData: Pointer to data to be read
 * @param  ReadAddr: Read start address
 * @param  Size: Size of data to read
 * @retval QSPI memory status
 */
int8_t BSP_QSPI_Read(uint8_t* pData, uint32_t ReadAddr, uint32_t Size)
{
	HAL_StatusTypeDef HAL_status;
	QSPI_CommandTypeDef sCommand;

	/* Initialize the read command - zg */
	sCommand.Instruction       = READ_CMD;
	sCommand.Address           = ReadAddr;
	sCommand.AlternateBytes    = 0;
	sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
	sCommand.AlternateBytesSize= 0;
	sCommand.DummyCycles       = 0;
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_1_LINE;
	sCommand.NbData            = Size;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	WaitUntilDeviceIsIdle();

	/* Send the command */
	HAL_status = HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
	switch (HAL_status)
	{
		case HAL_OK: break;
		default:
		case HAL_ERROR: return 1;
		case HAL_BUSY:  return 2;
		case HAL_TIMEOUT: return 3;
	}


	/* Reception of the data */
	HAL_status = HAL_QSPI_Receive(&QSPIHandle, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
	switch (HAL_status)
	{
		case HAL_OK: break;
		default:
		case HAL_ERROR: return 4;
		case HAL_BUSY:  return 5;
		case HAL_TIMEOUT: return 6;
	}

	// zg - I do not understand the following comment.  A delay here should have
	//      no bearing whatsoever on corruption elsewhere.  Maybe there is some
	//      other defect in the code.  Not changing until further understood.

	// NOTE: was originally 100. Causes slow downloads in
	// sewerwatch and slow analysis graphs. CAREFUL! This
	// value, if incorrect, may cause issues with read-write
	// flash setting/config corruption.
	//HAL_Delay(2);

	return QSPI_OK;
}

int8_t BSP_Log_Track_Write(int batt_info)
{
	int8_t status;

	status = BSP_QSPI_Erase_Sector(LOG_TRACK);

	LogTrack.write_times++;

	if (batt_info == LOG_WRITE_LOG_TRACK)
	{
		ltc2943_update_log_track();  // Note: can take 68 ms; this will only update LogTrack.accumulator if on batteries and if the ltc2943 has meaningful information
	}

	ltc2943_update_percentage();

	LogTrack.checksum = 0;
	LogTrack.checksum = GetChkSum((uint8_t *) &LogTrack, sizeof(LogTrack));
	status = BSP_QSPI_Write((uint8_t *) &LogTrack, LOG_TRACK, sizeof(LogTrack));
	//HAL_Delay(100);
	return status;
}

#define USE_SMART_ERASE

#ifdef USE_SMART_ERASE



static int isLogPageErased(uint32_t ReadAddr)
{
	log_t logrec;

	BSP_QSPI_Read((uint8_t *)&logrec, ReadAddr, sizeof(log_t));

	return isErased((uint8_t *)&logrec, sizeof(logrec));
}
#endif



int8_t BSP_Log_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size)
{
	int8_t status;


#ifdef USE_SMART_ERASE

	// zg The Size of a log record is 32 bytes, therefore, a single
	// record can never span two pages of flash, either 4K or 64K.
	// Thus, it is sufficient to test for only the beginning of a page
	// and if it is the start of a page, does it need to be erased.
	if (log_IsStartOf4KPage(WriteAddr))
	{
		if (!isLogPageErased(WriteAddr))
		{
			status = BSP_QSPI_Erase_Sector(WriteAddr);

#ifdef	LOG_DOES_NOT_WRAP
#else
			// keep track of where first log record starts

			LogTrack_first_logrec_addr += SECTOR_SIZE;
			if (LogTrack_first_logrec_addr >= LOG_TOP)
			{
				LogTrack_first_logrec_addr = LOG_BOTTOM;  // rollover
			}
#endif
		}
	}

#else


	uint32_t tmp32;

	tmp32 = WriteAddr >> 12;
	if (tmp32 != LogTrack.last_sector_erased)
	{
		status = BSP_QSPI_Erase_Sector(WriteAddr);
		LogTrack.last_sector_erased = tmp32;
	}

#endif



	//HAL_Delay(100);

	status = BSP_QSPI_Write(pData, WriteAddr, Size);

	//HAL_Delay(100);

	return status;
}

/**
 * @brief  Writes an amount of data to the QSPI memory.
 * @param  pData: Pointer to data to be written
 * @param  WriteAddr: Write start address
 * @param  Size: Size of data to write
 * @retval QSPI memory status
 */
int8_t BSP_QSPI_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size)
{

	QSPI_CommandTypeDef sCommand;
	uint32_t end_addr, current_size, current_addr;

	/* Calculation of the size between the write address and the end of the page */
	current_size = N25Q256A_PAGE_SIZE - (WriteAddr % N25Q256A_PAGE_SIZE);

	/* Check if the size of the data is less than the remaining place in the page */
	if (current_size > Size)
	{
		current_size = Size;
	}

	/* Initialize the address variables */
	current_addr = WriteAddr;
	end_addr = WriteAddr + Size;

	/* Initialize the program command - zg */
	sCommand.Instruction       = FLASH_CMD_PP;
	sCommand.Address           = 0;
	sCommand.AlternateBytes    = 0;
	sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
	sCommand.AlternateBytesSize= 0;
	sCommand.DummyCycles       = 0;
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_1_LINE;
	sCommand.NbData            = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	WaitUntilDeviceIsIdle();  // zg - not strictly necessary since WriteEnable will wait...

	/* Perform the write page by page */
	do {
		sCommand.Address = current_addr;
		sCommand.NbData = current_size;


		/* Enable write operations */
		WriteEnable(&QSPIHandle);

		/* Send the command */
		if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
			return -2;
		}

		/* Transmit the data */
		if (HAL_QSPI_Transmit(&QSPIHandle, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
			return -3;
		}

		WaitUntilDeviceIsIdle();


		/* Update the address and size variables for next page programming */
		current_addr += current_size;
		pData += current_size;
		current_size = ((current_addr + N25Q256A_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : N25Q256A_PAGE_SIZE;
	} while (current_addr < end_addr);

	return QSPI_OK;
}



int8_t BSP_QSPI_Erase_Sector2(uint32_t BlockAddress, uint32_t Instruction)
{

	HAL_StatusTypeDef HAL_status;
	QSPI_CommandTypeDef sCommand;




	/* Initialize the erase command - zg */
	sCommand.Instruction       = Instruction;
	sCommand.Address           = BlockAddress;
	sCommand.AlternateBytes    = 0;
	sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
	sCommand.AlternateBytesSize= 0;
	sCommand.DummyCycles       = 0;
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_NONE;
	sCommand.NbData            = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;


	WriteEnable(&QSPIHandle);

	/* Send the command */
	HAL_status = HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
	switch (HAL_status)
	{
		case HAL_OK: break;
		default:
		case HAL_ERROR: return 1;
		case HAL_BUSY:  return 2;
		case HAL_TIMEOUT: return 3;
	}

	WaitUntilDeviceIsIdle();


	return HAL_OK;
}

int8_t BSP_QSPI_Erase_Sector(uint32_t BlockAddress)
{
	//return QSPI_OK;  // zg for timing test...

	return BSP_QSPI_Erase_Sector2(BlockAddress, FLASH_CMD_P4E);
}



/**
 * @brief  Erases the entire QSPI memory.
 * @retval QSPI memory status
 */
int8_t BSP_QSPI_Erase_Chip(void)
{

	HAL_StatusTypeDef HAL_status;

	QSPI_CommandTypeDef sCommand;

	/* Initialize the erase command - zg */
	sCommand.Instruction       = BULK_ERASE_CMD;
	sCommand.Address           = 0;
	sCommand.AlternateBytes    = 0;
	sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
	sCommand.AlternateBytesSize= 0;
	sCommand.DummyCycles       = 0;
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_NONE;
	sCommand.NbData            = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;



	/* Enable write operations */
	WriteEnable(&QSPIHandle);


	/* Send the command */
	HAL_status = HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
	switch (HAL_status)
	{
		case HAL_OK: break;
		default:
		case HAL_ERROR: return 1;
		case HAL_BUSY:  return 2;
		case HAL_TIMEOUT: return 3;
	}

	WaitUntilDeviceIsIdle();

	return HAL_OK;
}




#if 0
/* QUADSPI init function */
int8_t MX_QUADSPI_Init(void) {
	QSPIHandle.Instance = QUADSPI;
	HAL_QSPI_DeInit(&QSPIHandle);

	QSPIHandle.Init.ClockPrescaler = 1; //4;
	QSPIHandle.Init.FifoThreshold = 4; //1;
	QSPIHandle.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
	QSPIHandle.Init.FlashSize = 24; //1;
	QSPIHandle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_4_CYCLE;
	QSPIHandle.Init.ClockMode = QSPI_CLOCK_MODE_0;

	if (HAL_QSPI_Init(&QSPIHandle) != HAL_OK) {
		return -1; // _Error_Handler(__FILE__, __LINE__);
	}

	/* QSPI memory reset */
	if (ResetMemory(&QSPIHandle) != QSPI_OK) {
		return -2; //   _Error_Handler(__FILE__, __LINE__);
	}

	/*##-3- Configure the NVIC for QSPI #########################################*/
	/* NVIC configuration for QSPI interrupt */
	return 0;
}
#endif




void HAL_QSPI_MspInit(QSPI_HandleTypeDef* qspiHandle)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	if (qspiHandle->Instance == QUADSPI)
	{
		__HAL_RCC_QSPI_CLK_ENABLE();

		/* Reset the QuadSPI memory interface */
		__HAL_RCC_QSPI_FORCE_RESET();
		__HAL_RCC_QSPI_RELEASE_RESET();

		GPIO_InitStruct.Pin       = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
		GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull      = GPIO_NOPULL;
		GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
		HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	}
}

// zg - why is this being done ???
void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef* qspiHandle)
{
	if (qspiHandle->Instance == QUADSPI)
	{
		__HAL_RCC_QSPI_CLK_DISABLE();

		HAL_GPIO_DeInit(GPIOE, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
	}
}
