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

#include "common.h"
#include "gpio.h"

#if 0
void gpio_unused_pin(GPIO_TypeDef  *Port, uint32_t Pin)  // This is pending...
{
	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin   = Pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_WritePin(Port, Pin, GPIO_PIN_RESET);
	HAL_GPIO_Init(Port, &GPIO_InitStruct);

}

#else

void gpio_unused_pin(GPIO_TypeDef  *Port, uint32_t Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	HAL_GPIO_DeInit(Port, Pin);   // Just to be safe, reset pin to default state

	GPIO_InitStruct.Pin  = Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;  // see app note AN4899
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;     // see app note AN4899
	HAL_GPIO_WritePin(Port, Pin , GPIO_PIN_RESET);  // Just to be safe, set pin logic state to ground
	HAL_GPIO_Init(Port, &GPIO_InitStruct);


}

#endif

uint32_t BoardId = BOARD_ID_GEN4;

static int get_digital_board_id(void)
{
	int pin0, pin1, pin3, pin4;
	int board_id;
	// Gen5 iTracker boards left pins PD10, PD11, PD13, and PD14 floating.
	// This firmware installs a pull-up resistor.  Therefore, a Gen5 iTracker board will report 0xF for a board id value.
	// SewerWatch pull PD10 low.

	pin0 = HAL_GPIO_ReadPin(GEN5_BOARD_ID_0_Port, GEN5_BOARD_ID_0_Pin);
	pin1 = HAL_GPIO_ReadPin(GEN5_BOARD_ID_1_Port, GEN5_BOARD_ID_1_Pin);
	pin3 = HAL_GPIO_ReadPin(GEN5_BOARD_ID_3_Port, GEN5_BOARD_ID_3_Pin);
	pin4 = HAL_GPIO_ReadPin(GEN5_BOARD_ID_4_Port, GEN5_BOARD_ID_4_Pin);

	board_id = (pin4 << 3) | (pin3 << 2) | (pin1 << 1) | pin0;

	return board_id;

}

void get_board_id(void)
{
	if (isGen4())
	{
		BoardId = BOARD_ID_GEN4;
	}
	else if (isGen5())
	{

		if (get_digital_board_id() == 0xF)  // iTracker5 with floating digital board id pins
		{

			float board_id_voltage = 99.9;

			BoardId = BOARD_ID_510;  // to be safe, assign a default

			// Brief history of the analog board id designs:
			// 1. Very first prototypes of 5.10 had 3V3 driving R18/R14 voltage divider and would produce a voltage.
			// 2. Very first production run of 5.10 marked all parts as DNI.  These boards produce a decreasing set of voltages (nothing steady).
			// 3. Schematic of 5.11 shows REV_DRV instead of 3V3 and R18/R14 are marked DNI. These boards produce a decreasing set of voltages (nothing steady).
			//
			// Consequently, it is very hard to distinguish a 5.11 board.

			Gen5_get_board_id_voltage(&board_id_voltage);
			if (board_id_voltage < 1.045)
			{
				BoardId = BOARD_ID_510;
			}
			else if ((board_id_voltage >= 1.045) && (board_id_voltage < 1.155))
			{
				BoardId = BOARD_ID_511;
			}
		}
		else
		{
		    // SewerWatch 1.0 or newer, for now assume sewerwatch 1.0
			BoardId = BOARD_ID_SEWERWATCH_1;
		}
	}
	// Turn off board id digital pins as they are no longer used
	gpio_unused_pin(PD10);  // unused
	gpio_unused_pin(PD11);  // unused
	gpio_unused_pin(PD13);  // unused
	gpio_unused_pin(PD14);  // unused

	// Initialize other gpio pins based on BoardId



	if (BoardId == BOARD_ID_SEWERWATCH_1)
	{
		GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_InitStruct.Pin   = GEN5_SEWERWATCH_TEMP_DRV_Pin;  //PE5
		GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull  = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_WritePin(GEN5_SEWERWATCH_TEMP_DRV_Port, GEN5_SEWERWATCH_TEMP_DRV_Pin, GPIO_PIN_RESET);
		HAL_GPIO_Init(GEN5_SEWERWATCH_TEMP_DRV_Port, &GPIO_InitStruct);

		//PE6
#if 0  // For later
		GPIO_InitStruct.Pin = GEN5_SEWERWATCH_REED_SW_INTR_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GEN5_SEWERWATCH_REED_SW_INTR_Port, &GPIO_InitStruct);

		/* EXTI interrupt init*/
		HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
#endif
	}

}

void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;


	/* NOTE: According to ST application note: AN4899
	 * it is best to configure unused GPIO pins as ANALOG :
	 *    "GPIO always have an input channel, which can be either digital or analog.
     *    If it is not necessary to read the GPIO data, prefer the configuration as analog input. This saves the consumption of the input Schmitt trigger."
	 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();


    //==== GPIOA =====================================================

	gpio_unused_pin(PA0);  // used later by UART4 - Velocity 1
	gpio_unused_pin(PA1);  // used later by UART4 - Velocity 1
	gpio_unused_pin(PA2);  // used later by UART2 - WiFi
	gpio_unused_pin(PA3);  // used later by UART2 - WiFi

	// Get Board ID


	//PA4
	gpio_unused_pin(PA4);

    if (isGen4())
    {
      //PA5
	  GPIO_InitStruct.Pin  = GEN4_VH_SW_ADC_EN_Pin;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_WritePin(GEN4_VH_SW_ADC_EN_Port, GEN4_VH_SW_ADC_EN_Pin , GPIO_PIN_RESET);  // PA5
	  HAL_GPIO_Init(GEN4_VH_SW_ADC_EN_Port, &GPIO_InitStruct);  // PA5


	  //PA6
	  gpio_unused_pin(PA6);   // defined as NTC_EN but never used
    }


    if (isGen5())
    {
    	//PA5
    	GPIO_InitStruct.Pin  = GEN5_HIGH_SIG_OUT_Pin ;
    	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    	GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    	HAL_GPIO_Init(GEN5_HIGH_SIG_OUT_Port , &GPIO_InitStruct);
    	GPIO_InitStruct.Alternate = 0;

    	//PA6
    	GPIO_InitStruct.Pin = GEN5_Temp_POWER_Pin;    // Just leave it off all of the time ? So that either type (powered, unpowered) can be used.  Can free this pin for another use ?
    	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    	GPIO_InitStruct.Pull = GPIO_NOPULL;
    	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    	HAL_GPIO_WritePin(GEN5_Temp_POWER_Port, GEN5_Temp_POWER_Pin, GPIO_PIN_RESET);  // TODO may need to turn power on/off to lower sleep current when not using OneWire
    	HAL_GPIO_Init(GEN5_Temp_POWER_Port , &GPIO_InitStruct);
    }

	//PA7
	GPIO_InitStruct.Pin = GEN45_VH_PWR_EN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_WritePin(GEN45_VH_PWR_EN_Port, GEN45_VH_PWR_EN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_Init(GEN45_VH_PWR_EN_Port , &GPIO_InitStruct);



	//PA8
	// Note there is a physical pull-up resistor, R49 (47K) in the circuit, so why is there an internal pull-up here?
	// Also, this pin could potentially be configured as an interrupt.
#if 0 // original
	GPIO_InitStruct.Pin = GEN45_uSD_DETECT_Pin;     // See BSP_SD_IsDetected() for usage of this pin
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GEN45_uSD_DETECT_Port , &GPIO_InitStruct);
#else

	  GPIO_InitStruct.Pin = GPIO_PIN_8;
	  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  /* EXTI interrupt init*/
	  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
#endif

	//PA 9, 10, 11, 12 connected to USB
#ifdef ENABLE_USB

    // PA9 - not used for now
	GPIO_InitStruct.Pin  = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;  // see app note AN4899
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;     // see app note AN4899
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// PA10 is hooked to the USB id line
	GPIO_InitStruct.Pin   = GPIO_PIN_10;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	/*Configure PA10 pin Output Level to GND for USB Type A device */
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


	// PA11 PA12
	GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#else
	gpio_unused_pin(PA9);
	gpio_unused_pin(PA10);
	gpio_unused_pin(PA11);
	gpio_unused_pin(PA12);
#endif

	//PA13 SWD_DIO  - NOTE: this pin is in AF mode after reset, which is JTMS/SWDAT in pull-up
	//PA14 SWD_CLK  - NOTE: this pin is in AF mode after reset, which is JTCK/SWCLK in pull-down

	//PA15 - NOTE: this pin is in AF mode after reset, which is JTDI with a pull-up  datasheet page 297
	if (isGen4())
	{
	  // PA15 Unused
	  gpio_unused_pin(PA15);
	}

	if (isGen5())
	{
		// PA15 WiFi power enable
		GPIO_InitStruct.Pin = GEN5_WIFI_PWR_EN_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_WritePin(GEN5_WIFI_PWR_EN_Port, GEN5_WIFI_PWR_EN_Pin, GPIO_PIN_RESET);
		HAL_GPIO_Init(GEN5_WIFI_PWR_EN_Port , &GPIO_InitStruct);
	}







	//==== GPIOB =====================================================
	//PB0
	GPIO_InitStruct.Pin  = GEN45_ON_EXT_PWR_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GEN45_ON_EXT_PWR_Port , &GPIO_InitStruct);

	//PB1 - This pin appears to be initialized in uart code
	GPIO_InitStruct.Pin  = GEN45_RS485_DE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_WritePin(GEN45_RS485_DE_Port, GEN45_RS485_DE_Pin, GPIO_PIN_RESET);   // disable TX here for low-power mode
	HAL_GPIO_Init(GEN45_RS485_DE_Port, &GPIO_InitStruct);


	// PB2
	gpio_unused_pin(PB2);

	gpio_unused_pin(PB3);  // used later by UART1 - Cell  - NOTE: this pin is in AF mode after reset, which is JTDO in floating state no pull-up/pull-down

	gpio_unused_pin(PB4);  // used later by UART1 - Cell   - NOTE: this pin is in AF mode after reset, which is NJTRRST in pull-up

    //PB5
    if (isGen5())
    {
	  //PB5 - Level sensor power control
	  GPIO_InitStruct.Pin  = GEN5_RS485_PWR_EN_Pin;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_WritePin(GEN5_RS485_PWR_EN_Port, GEN5_RS485_PWR_EN_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_Init(GEN5_RS485_PWR_EN_Port, &GPIO_InitStruct);

    }

    if (isGen4())
    {
    	gpio_unused_pin(PB5);
    }

    //PB6, PB7
	gpio_unused_pin(PB6);  // used later by UART1 - Cell
	gpio_unused_pin(PB7);  // used later by UART1 - Cell

	//PB8
	gpio_unused_pin(PB8);

	//PB9
	gpio_unused_pin(PB9);  // used later by SPI2

	//PB10
	gpio_unused_pin(PB10);  // used later by LPUART1 - Velocity 2

	//PB11
	if (isGen4())
	{
		//TODO GEN4_Sensor_IN_Pin
		gpio_unused_pin(PB11);
	}
	if (isGen5())
	{
	  gpio_unused_pin(PB11);  // used later by LPUART1 - Velocity 2
	}

	//PB12
	gpio_unused_pin(PB12);

	//PB13

	gpio_unused_pin(PB13);  // used later by I2C
	gpio_unused_pin(PB14);  // used later by I2C
	gpio_unused_pin(PB15);  // used later by SPI2

	//==== GPIOC =====================================================

	//PC0
	GPIO_InitStruct.Pin   = GEN45_OPAMP_SD_Pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_WritePin(GEN45_OPAMP_SD_Port,    GEN45_OPAMP_SD_Pin,    GPIO_PIN_RESET);
	HAL_GPIO_Init(GEN45_OPAMP_SD_Port, &GPIO_InitStruct);


	//PC1
	if (isGen5())
	{
		GPIO_InitStruct.Pin   = GEN5_Temp_SNSR_Pin;
		GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull  = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
		HAL_GPIO_WritePin(GEN5_Temp_SNSR_Port,    GEN5_Temp_SNSR_Pin,    GPIO_PIN_SET);
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	}
	else
	{
		//GEN4_VH_PWR_ADC_Pin initialized upon use
		gpio_unused_pin(PC1);  // used later by SPI2
	}

	//PC2
	if (isGen5())
	{
		//TODO GEN5_SPI2_MISO_Pin
		gpio_unused_pin(PC2);  // used later by SPI2
	}

	//PC3
	if (isGen4())
	{
		gpio_unused_pin(PC3);  // reserved for TEMP_NTC but never used
	}
	if (isGen5())
	{
		gpio_unused_pin(PC3);  // reserved for ADC_CH4, connected to TP1
		// In SewerWatch TEMP_AD
	}

	//PC4 - GEN45_RS485_TXD_Pin
	gpio_unused_pin(PC4);  // used later by UART3-TX - RS-485

	//PC5 - GEN45_RS485_RXD_Pin
	gpio_unused_pin(PC5);  // used later by UART3-RX - RS-485

	//PC6
	GPIO_InitStruct.Pin   = GEN45_STATUS_LED_Pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_WritePin(GEN45_STATUS_LED_Port,    GEN45_STATUS_LED_Pin,    GPIO_PIN_RESET);
	HAL_GPIO_Init(GEN45_STATUS_LED_Port, &GPIO_InitStruct);


	//PC7
	if (isGen5())
	{
	  GPIO_InitStruct.Pin   = GEN5_STATUS2_LED_Pin;
	  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull  = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_WritePin(GEN5_STATUS2_LED_Port,    GEN5_STATUS2_LED_Pin,    GPIO_PIN_RESET);
	  HAL_GPIO_Init(GEN5_STATUS2_LED_Port, &GPIO_InitStruct);

	}
	if (isGen4())
	{
		gpio_unused_pin(PC7);  //
	}

	//PC8, PC9, PC10, PC11, PC12 used by uSD card, initialized upon use
	gpio_unused_pin(PC8);
	gpio_unused_pin(PC9);
	gpio_unused_pin(PC10);
	gpio_unused_pin(PC11);
	gpio_unused_pin(PC12);

	//PC13 unused
	gpio_unused_pin(PC13);

	//PC14, PC15 used by oscillator



	//==== GPIOD =====================================================
    //PD0 - GEN5_SENSOR_PG_Pin
	//PD1 - GEN5_SPI2_SCK_Pin
	//PD2 - GEN45_uSD_CMD_Pin
	//PD3 - GEN5_WIFI_CTS_Pin
	//PD4 - GEN5_WIFI_RTS_Pin
	//PD5 - GEN5_VELOCITY1_EN_Pin
	//PD6 - GEN5_VELOCITY2_EN_Pin
	//PD7 - GEN5_WIFI_AUX1_Pin
	gpio_unused_pin(PD0);
	gpio_unused_pin(PD1);
	gpio_unused_pin(PD2);
	gpio_unused_pin(PD3);
	gpio_unused_pin(PD4);
	gpio_unused_pin(PD5);
	gpio_unused_pin(PD6);
	gpio_unused_pin(PD7);
	gpio_unused_pin(PD8);

	//PD8
	if (isGen5())
	{
		GPIO_InitStruct.Pin   = GEN5_VELOCITY1_EN_Pin;  //PD5
		GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull  = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_WritePin(GEN5_VELOCITY1_EN_Port, GEN5_VELOCITY1_EN_Pin, GPIO_PIN_RESET);
		HAL_GPIO_Init(GEN5_VELOCITY1_EN_Port, &GPIO_InitStruct);


		GPIO_InitStruct.Pin   = GEN5_VELOCITY2_EN_Pin; //PD6
		GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull  = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_WritePin(GEN5_VELOCITY2_EN_Port, GEN5_VELOCITY2_EN_Pin, GPIO_PIN_RESET);
		HAL_GPIO_Init(GEN5_VELOCITY2_EN_Port, &GPIO_InitStruct);


		GPIO_InitStruct.Pin   = GEN5_VDD_3V3_SYNC_Pin; //PD8
		GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull  = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_WritePin(GEN5_VDD_3V3_SYNC_Port, GEN5_VDD_3V3_SYNC_Pin, GPIO_PIN_RESET);  // Start in low-power mode.  When performing a measurement, this will change to PWM mode.
		HAL_GPIO_Init(GEN5_VDD_3V3_SYNC_Port, &GPIO_InitStruct);

	}


	gpio_unused_pin(PD9);   // GEN5_CELL_AUX1_Pin

#if 0
	// PD9 this pin is reserved for bootloading Laird firmware on the nRF52840
	GPIO_InitStruct.Pin   = GEN5_CELL_AUX1_Pin; //PD9
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_WritePin(GEN5_CELL_AUX1_Port, GEN5_CELL_AUX1_Pin, GPIO_PIN_SET);  // Hold Hi during Laird power on
	HAL_GPIO_Init(GEN5_CELL_AUX1_Port, &GPIO_InitStruct);
#endif


	if (isGen4())
	{
		gpio_unused_pin(PD10);  // unused
		gpio_unused_pin(PD11);  // unused
	}

	if (isGen5())
	{
		// Gen5 iTracker was floating; SewerWatch will pull low for board id
		//gpio_unused_pin(PD10);  // unused
		//gpio_unused_pin(PD11);  // unused

	    // PD10
		GPIO_InitStruct.Pin  = GEN5_BOARD_ID_0_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		HAL_GPIO_Init(GEN5_BOARD_ID_0_Port , &GPIO_InitStruct);

	    // PD11
		GPIO_InitStruct.Pin  = GEN5_BOARD_ID_1_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		HAL_GPIO_Init(GEN5_BOARD_ID_1_Port , &GPIO_InitStruct);
	}

    // PD12 GEN45_Bat_Alarm_Pin - never used in Gen4
	GPIO_InitStruct.Pin  = GEN45_Bat_Alarm_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GEN45_Bat_Alarm_Port , &GPIO_InitStruct);

	if (isGen4())
	{
		gpio_unused_pin(PD13);  // unused
		gpio_unused_pin(PD14);  // unused
	}

	if (isGen5())
	{
		// Gen5 iTracker was floating; SewerWatch will pull low for board id
		//gpio_unused_pin(PD13);  // unused
		//gpio_unused_pin(PD14);  // unused

	    // PD13
		GPIO_InitStruct.Pin  = GEN5_BOARD_ID_3_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		HAL_GPIO_Init(GEN5_BOARD_ID_3_Port , &GPIO_InitStruct);

	    // PD14
		GPIO_InitStruct.Pin  = GEN5_BOARD_ID_4_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		HAL_GPIO_Init(GEN5_BOARD_ID_4_Port , &GPIO_InitStruct);
	}


	//PD15 Cell Power Enable
	GPIO_InitStruct.Pin   = GEN45_CELL_PWR_EN_Pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_WritePin(GEN45_CELL_PWR_EN_Port, GEN45_CELL_PWR_EN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_Init(GEN45_CELL_PWR_EN_Port, &GPIO_InitStruct);



	//==== GPIOE =====================================================

	//PE0 - GEN5_AUX1_Pin
	//PE1 - GEN5_AUX2_Pin
	//PE2 - GEN5_AUX3_Pin
	//PE3 - GEN5_AUX4_Pin
	//PE4 - GEN5_AUX_EN_Pin
	//PE5 unused on Gen4 and iTracker5
	//PE6 unused

	gpio_unused_pin(PE0);
	gpio_unused_pin(PE1);
	gpio_unused_pin(PE2);
	gpio_unused_pin(PE3);
	gpio_unused_pin(PE4);
	gpio_unused_pin(PE5);  // In SewerWatch TEMP_DRV
	gpio_unused_pin(PE6);  // In SewerWatch REED_SW_INTR


	//PE7 - GEN45_RS485_nRE_Pin - UART3


	if (isGen4())
	{
		GPIO_InitStruct.Pin =  GEN4_Zentri_WU_Pin | GEN4_Zentri_CMD_STR_Pin | GEN4_Zentri_Reset_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GEN4_Zentri_SC_Pin | GEN4_Zentri_OOB_Pin | GEN4_Sensor_PG_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;
		HAL_GPIO_WritePin(GPIOE, GEN4_Zentri_WU_Pin | GEN4_Zentri_CMD_STR_Pin | GEN4_Zentri_Reset_Pin, GPIO_PIN_RESET);
		HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	}


	// PE7 for RS485 (not RE)
	GPIO_InitStruct.Pin =  GEN45_RS485_nRE_Pin;  //PE7
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // From Mark Watson 7-15-2021
	GPIO_InitStruct.Alternate = 0;
	HAL_GPIO_WritePin(GPIOE, GEN45_RS485_nRE_Pin, GPIO_PIN_SET);  // disable RX here for low-power mode
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);


	//PE8 -
	//PE9 -
	gpio_unused_pin(PE8);  // initialized later for GEN5_SENSOR_DRVn_Pin
	gpio_unused_pin(PE9);  // initialized later for GEN45_SENSOR_DRVp_Pin

	// PINS PE10 - PE15 are used by QSPI for SD CARD - initialized later
	gpio_unused_pin(PE10);
	gpio_unused_pin(PE11);
	gpio_unused_pin(PE12);
	gpio_unused_pin(PE13);
	gpio_unused_pin(PE14);
	gpio_unused_pin(PE15);


}

#if 0
void TM_GPIO_SetPinAsOD(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	uint8_t i;

	/* Go through all pins */
	for (i = 0x00; i < 0x10; i++) {
		/* Pin is set */
		if (GPIO_Pin & (1 << i))
		{
			/* Set bit combination for open drain */
			GPIOx->OTYPER |= (0x01 << i);
		}
	}
}

void TM_GPIO_SetPinAsPP(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	uint8_t i;
	/* Go through all pins */
	for (i = 0x00; i < 0x10; i++) {
		/* Pin is set */
		if (GPIO_Pin & (1 << i))
		{
			/* Set 01 bits combination for output */
			GPIOx->OTYPER &= ~(0x01 << i);
		}
	}
}
#endif
