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

#include "i2c.h"
#include "gpio.h"

I2C_HandleTypeDef hi2c2;
unsigned char i2c_txBuff[10];


#if 0 // zg for ref
typedef struct __I2C_HandleTypeDef
{
  I2C_TypeDef                *Instance;      /*!< I2C registers base address                */
  I2C_InitTypeDef            Init;           /*!< I2C communication parameters              */
  uint8_t                    *pBuffPtr;      /*!< Pointer to I2C transfer buffer            */
  uint16_t                   XferSize;       /*!< I2C transfer size                         */
  __IO uint16_t              XferCount;      /*!< I2C transfer counter                      */
  __IO uint32_t              XferOptions;    /*!< I2C sequantial transfer options, this parameter can
                                                  be a value of @ref I2C_XFEROPTIONS */
  __IO uint32_t              PreviousState;  /*!< I2C communication Previous state          */
  HAL_StatusTypeDef(*XferISR)(struct __I2C_HandleTypeDef *hi2c, uint32_t ITFlags, uint32_t ITSources);  /*!< I2C transfer IRQ handler function pointer */
  DMA_HandleTypeDef          *hdmatx;        /*!< I2C Tx DMA handle parameters              */
  DMA_HandleTypeDef          *hdmarx;        /*!< I2C Rx DMA handle parameters              */
  HAL_LockTypeDef            Lock;           /*!< I2C locking object                        */
  __IO HAL_I2C_StateTypeDef  State;          /*!< I2C communication state                   */
  __IO HAL_I2C_ModeTypeDef   Mode;           /*!< I2C communication mode                    */
  __IO uint32_t              ErrorCode;      /*!< I2C Error code                            */
  __IO uint32_t              AddrEventCount; /*!< I2C Address Event counter                 */
#endif

/* I2C2 init function */
int8_t MX_I2C2_Init(void)
{

	//HAL_I2C_DeInit(&hi2c2);  // zg - can you safely de-init before init ?? What about .Instance ??

	hi2c2.Instance              = I2C2;
	hi2c2.State                 = HAL_I2C_STATE_RESET;

	hi2c2.Init.Timing           = 0x00707CBB;   // zg - what does this mean?
	//hi2c2.Init.Timing           = 0x00100D13;   // zg - Standard Mode timing from CubeMX (100 kHz)
	//hi2c2.Init.Timing           = 0x00000002;   // zg - Fast Mode timing from CubeMX (400 kHz)


	hi2c2.Init.OwnAddress1      = 0;
	hi2c2.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
	hi2c2.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
	hi2c2.Init.OwnAddress2      = 0;
	hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c2.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
	hi2c2.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

	if (HAL_I2C_Init(&hi2c2) != HAL_OK)
	{
		return -1;
	}

#if 0
    // zg - only one filter is active.  The analogue is active by default, which filters 50 ns spikes, per the I2C spec.
	// Furthermore, these must be set prior to enabling the peripheral, which is done in HAL_I2C_Init above.
	// So...the following functions disable the peripheral, change the value, then re-enable.  Again, not sure why needed at all?

	/**Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
	{
		return -2;
	}

	/**Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
	{
		return -3;
	}
#endif
	return 0;
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle) {
	GPIO_InitTypeDef GPIO_InitStruct;
	if (i2cHandle->Instance == I2C2) {

		GPIO_InitStruct.Pin = GEN45_I2C_SCL_Pin | GEN45_I2C_SDA_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
		HAL_GPIO_Init(GEN45_I2C_SCL_Port, &GPIO_InitStruct);

		/* I2C2 clock enable */
		__HAL_RCC_I2C2_CLK_ENABLE()
		;
	}
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle) {
	if (i2cHandle->Instance == I2C2) {
		/* Peripheral clock disable */
		__HAL_RCC_I2C2_CLK_DISABLE();

		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13 | GPIO_PIN_14);
	}
}

