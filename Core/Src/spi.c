/*************************************************************************
* COPYRIGHT - All Rights Reserved.
* 2022 Eastech Flow Controls, Inc.
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

#include "spi.h"
#include "gpio.h"

SPI_HandleTypeDef hspi2;
unsigned char spi_txBuff[2];
unsigned char spi_rxBuff[2];


/* I2C2 init function */
int8_t MX_SPI2_Init(void)
{
	hspi2.Instance = SPI2;
	hspi2.State = HAL_SPI_STATE_RESET;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_16BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_HARD_OUTPUT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.pTxBuffPtr = spi_txBuff;
	hspi2.TxXferSize = 2;
	hspi2.pRxBuffPtr = spi_rxBuff;
	hspi2.RxXferSize = 2;
	hspi2.TxXferCount = 0;
	hspi2.RxXferCount = 0;

	if (HAL_SPI_Init(&hspi2) != HAL_OK)
	{
		return -1;
	}
	return 0;
}

void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	/* SPI2 clock enable */
	__HAL_RCC_SPI2_CLK_ENABLE();

	if (spiHandle->Instance == SPI2)
	{
		GPIO_InitStruct.Pin = GEN5_SPI2_MOSI_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
		HAL_GPIO_Init(GEN5_SPI2_MOSI_Port, &GPIO_InitStruct);

		/* Chip select will be handled manually */
		GPIO_InitStruct.Pin = GEN5_SPI2_NSS_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_WritePin(GEN5_SPI2_NSS_Port, GEN5_SPI2_NSS_Pin, GPIO_PIN_SET);
		HAL_GPIO_Init(GEN5_SPI2_NSS_Port, &GPIO_InitStruct);


#ifdef PROTOTYPE_BOARD

#else
		GPIO_InitStruct.Pin = GEN5_SPI2_MISO_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
		HAL_GPIO_Init(GEN5_SPI2_MISO_Port, &GPIO_InitStruct);
#endif

		GPIO_InitStruct.Pin =  GEN5_SPI2_SCK_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
		HAL_GPIO_Init(GEN5_SPI2_SCK_Port, &GPIO_InitStruct);
	}
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{
	if (spiHandle->Instance == SPI2)
	{
		/* Peripheral clock disable */
		__HAL_RCC_SPI2_CLK_DISABLE();

		HAL_GPIO_DeInit(GEN5_SPI2_MOSI_Port, GEN5_SPI2_MOSI_Pin);
		HAL_GPIO_DeInit(GEN5_SPI2_NSS_Port,  GEN5_SPI2_NSS_Pin);

#ifdef PROTOTYPE_BOARD

#else
		HAL_GPIO_DeInit(GEN5_SPI2_MISO_Port, GEN5_SPI2_MISO_Pin);
#endif

		HAL_GPIO_DeInit(GEN5_SPI2_SCK_Port,  GEN5_SPI2_SCK_Pin);
	}
}

