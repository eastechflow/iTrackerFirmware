/******************************************************************************
 *
 * Revision: A
 * Author:   MWatson
 * Created:  4/22/2022
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  FILE: AD5270.c
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * DESCRIPTION: Source file for the AD5270 Gain control Potentiometer
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

#include <string.h>
#include "math.h"
#include "Gen5_AD5270.h"
#include "spi.h"


unsigned char spi_txBuff[2];

/******************************************************************************
 *
 * Revision: A
 * Author:   MWatson
 * Created:  April 2022
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  TITLE: AD5270_Set
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * PURPOSE:	Writes a register pattern via SPI to Digital Pot AD5270
 *
 * INPUTS:	register number and desired data.
 *
 * OUTPUTS:	None.
 *
 * CALLS:	none.
 *
 * CALLED BY:	.
 *
 * DETAILS:
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *	
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
bool Gen5_AD5270_Set(unsigned char reg_address, uint16_t reg_value)
{
	HAL_StatusTypeDef HAL_status;

	reg_value &= 0x03FF;  // Just in case, lop off the command section of the value.
	spi_txBuff[1] = (reg_value >> 8) | (reg_address << 2); // The upper nibble becomes the command (or address)
	spi_txBuff[0] = (uint8_t)reg_value;
	HAL_GPIO_WritePin(GEN5_SPI2_NSS_Port, GEN5_SPI2_NSS_Pin, GPIO_PIN_RESET);
	HAL_status = HAL_SPI_Transmit(&hspi2, spi_txBuff, 1, 10000);
	HAL_GPIO_WritePin(GEN5_SPI2_NSS_Port, GEN5_SPI2_NSS_Pin, GPIO_PIN_SET);
	if (HAL_status == HAL_OK)
		return 0;
	else
		return 1;
}

uint16_t Gen5_AD5270_Get(unsigned char reg_address)
{
	HAL_StatusTypeDef HAL_status;

	spi_txBuff[1] = reg_address << 2; // The middle 4 bits become the command
	spi_txBuff[0] = 0x00;
	HAL_GPIO_WritePin(GEN5_SPI2_NSS_Port, GEN5_SPI2_NSS_Pin, GPIO_PIN_RESET);
	HAL_status = HAL_SPI_Transmit(&hspi2, spi_txBuff, 1, 10000);
	HAL_GPIO_WritePin(GEN5_SPI2_NSS_Port, GEN5_SPI2_NSS_Pin, GPIO_PIN_SET);
	if (HAL_status != HAL_OK) return 1;
	HAL_GPIO_WritePin(GEN5_SPI2_NSS_Port, GEN5_SPI2_NSS_Pin, GPIO_PIN_RESET);
	HAL_status = HAL_SPI_Receive(&hspi2, spi_rxBuff, 1, 10000);
	HAL_GPIO_WritePin(GEN5_SPI2_NSS_Port, GEN5_SPI2_NSS_Pin, GPIO_PIN_SET);
	if (HAL_status != HAL_OK) return 1;
	return((spi_rxBuff[0] << 8) + spi_rxBuff[1]);
}

void Gen5_AD5270_Init(void)
{
//	__HAL_RCC_SPI2_CLK_ENABLE();

    Gen5_AD5270_Set(AD5270_SW_RESET, 0x0000);
    HAL_Delay(2);
    /* The device wiper is locked by default. It is unlocked by writing a 1 to C1 in the Control Register. */
    Gen5_AD5270_Set(AD5270_CONTROL_WR, 0x0002);
    /* Now, set to a gain of 10 */
    Gen5_AD5270_Set(AD5270_WRITE_WIPER, 240);
}

void Gen5_AD5270_SetWiper(uint16_t WiperPos)
{
    /* Now, set to a gain of 10 */
    Gen5_AD5270_Set(AD5270_WRITE_WIPER, WiperPos);
}

/******************************************************************************
 * Done.
 */
