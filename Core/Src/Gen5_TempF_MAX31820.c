/**
 * Keil project example for DS18B20 temperature sensor
 *
 * Before you start, select your target, on the right of the "Load" button
 *
 * @author    Tilen MAJERLE
 * @email     tilen@majerle.eu
 * @website   http://stm32f4-discovery.net
 * @ide       Keil uVision 5
 * @conf      PLL parameters are set in "Options for Target" -> "C/C++" -> "Defines"
 * @packs     STM32F4xx/STM32F7xx Keil packs are requred with HAL driver support
 * @stdperiph STM32F4xx/STM32F7xx HAL drivers required
 */
/* Include core modules */
//#include "stm32fxxx_hal.h"
#include "stm32l4xx_hal.h"
#include "usart.h"
#include "uart4.h"
#include "lpuart1.h"
#include "tim.h"
#include "Gen5_TempF.h"
#include "Common.h"
#include "debug_helper.h"
#include "factory.h"


/* Include my libraries here */

#include "tm_stm32_ds18b20.h"
#include "tm_stm32_onewire.h"

extern void TM_GPIO_SetPinAsOD(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
extern void TM_GPIO_SetPinAsPP(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);


/* Onewire structure */
TM_OneWire_t OW;

/* Array for DS18B20 ROM number */
uint8_t DS_ROM[8];





#if 0
static void print_DS_ROM(void)
{
  char msg[80];

  sprintf(msg,"DS_ROM: %X %X %X %X %X %X %X %X",
			  DS_ROM[0],
			  DS_ROM[1],
			  DS_ROM[2],
			  DS_ROM[3],
			  DS_ROM[4],
			  DS_ROM[5],
			  DS_ROM[6],
			  DS_ROM[7]);
  AppendStatusMessage(msg);

  //lpuart1_tx_string(msg);
  //lpuart1_tx_string("\n");

}
#endif


static void turn_on_temperature_sensor(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	// This is to communicate over the bus and to provide parasitic power
	GPIO_InitStruct.Pin   = GEN5_Temp_SNSR_Pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_WritePin(GEN5_Temp_SNSR_Port,    GEN5_Temp_SNSR_Pin,    GPIO_PIN_SET);
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


	//delay_us(10);  // see datasheet "Time to Strong Pullup On"
	HAL_led_delay(1);  // give the capacitor plenty of time to charge
}

static void turn_off_temperature_sensor(void)
{
	gpio_unused_pin(GEN5_Temp_SNSR_Port, GEN5_Temp_SNSR_Pin);  // This should de-activate the pullup as well
}



#if 0 // moved to init
void Gen5_configure_tempF_eeprom(void)
{
	// Turn on parasitic power by enabling the GPIO pins and pullup
	turn_on_temperature_sensor();

	/* Set resolution */
	TM_DS18B20_SetResolution(&OW, DS_ROM, TM_DS18B20_Resolution_11bits);

	// Turn off parasitic power by disabling the GPIO pins and pullup
	 turn_off_temperature_sensor();
}
#endif



void Gen5_MAX31820_init(void)
{
	// Turn on parasitic power by enabling the GPIO pins and pullup
	turn_on_temperature_sensor();

	/* Init ONEWIRE port  */
	TM_OneWire_Init(&OW, GEN5_Temp_SNSR_Port, GEN5_Temp_SNSR_Pin);

	/* Check if any device is connected */
	if (TM_OneWire_First(&OW))
	{
		/* Read ROM number */
		TM_OneWire_GetFullROM(&OW, DS_ROM);

		//print_DS_ROM();
	}
	else
	{
		//AppendStatusMessage("Temperature Sensor not detected");
	}

	/* Check and Set Resolution  */
	if (TM_DS18B20_Is(DS_ROM))
	{
		if (TM_DS18B20_GetResolution(&OW, DS_ROM) != TM_DS18B20_Resolution_11bits)  // reduce stress on device EEPROM by writing only when needed
		{
		  /* Set resolution */
		  TM_DS18B20_SetResolution(&OW, DS_ROM, TM_DS18B20_Resolution_11bits);

		  // Must wait here to allow eeprom to finish writing before turning off parasitic power.  Datasheet indicates from 2 to 10 ms
		  HAL_led_delay(11);

		  //AppendStatusMessage("Temperature Sensor resolution set to 11 bits");
		}
		else
		{
			//AppendStatusMessage("Temperature Sensor resolution already set");
		}
	}
	else
	{
		//AppendStatusMessage("Temperature Sensor not a DS18B20");
	}

	// Turn off parasitic power by disabling the GPIO pins and pullup
	 turn_off_temperature_sensor();
}

static int max31820_wait_for_conversion_to_complete(void)  // returns 1 if conversion completed, 0 if timed out.  This can take up to 400 ms to fail.
{

	//HAL_led_delay(378);  // Wait for conversion to complete, based on 11 bit resolution.  This failed for some units which apparently did not have the 11 bit resolution set.
	HAL_led_delay(750+1);  // Wait for conversion to complete, based on worst case --- power-on defaults of the chip itself in case setting resolution failed. This worked on problematic units.

	// Standby current is 3 uA
	// Active current is 1 to 1.5 mA
	// So, overall, it is better to set resolution to 11 bits and incur the extra wait (just in case) than to simply use the full 12 bit resolution.
	//

	generic_delay_timer_ms = 22;  // Wait a max of 22 ms before timing out

	while (generic_delay_timer_ms)
	{
		if (TM_DS18B20_AllDone(&OW)) return 1;  // conversion completed OK
		led_heartbeat();
	}

	//if (sd_CellCmdLogEnabled)
	//{
	//	trace_AppendMsg("\nTX Error: timeout\n");
	//}

	return 0;  // conversion timed out

}

static int Gen5_MAX31820_get_tempF(float *DegF)  // returns 1 if Ok, 0 if fail.  This can take up to 400 ms to fail.
{

	int result = 0;
	float deg_C;

	// Turn on parasitic power by enabling the GPIO pins and pullup
	turn_on_temperature_sensor();

	/* Start conversion on all sensors */
	TM_DS18B20_StartAll(&OW);  // Note: for parasitic power, the last WriteByte/WriteBit will leave GEN5_Temp_SNSR_Pin in the SET state which is correct.
	
	if (max31820_wait_for_conversion_to_complete())  // returns 1 if conversion completed, 0 if timed out
	{
		/* Read temperature from device */
		if (TM_DS18B20_Read(&OW, DS_ROM, &deg_C))
		{

			//deg_C = (deg_F - 32.0) * 5.0 / 9.0;

			*DegF = (deg_C * 9.0 / 5.0) + 32.0;

			result = 1;
		}
	}

	// Turn off parasitic power by disabling the GPIO pins and pullup
	turn_off_temperature_sensor();

	return result;
}



void Gen5_MAX31820_test(void)
{
	float temp;

	//if (!Gen5_MAX31820_init()) return;
	
	while (1)
	{
		if (Gen5_MAX31820_get_tempF(&temp))
		{
			/* Temp read OK, CRC is OK */
			char msg[80];
			sprintf(msg, "\nTemperature %d.", (int)temp);
			lpuart1_tx_string(msg);
			sprintf(msg, "%d", (int)((temp-(int)temp)*100));
			lpuart1_tx_string(msg);
		}

		HAL_Delay(500);  // slow things down a bit between readings
    }

}



void get_tempF(void) // SIDE-EFFECTS !! Sets LastGoodDegF
{
  switch (BoardId) {
    case BOARD_ID_GEN4:
      Gen45_get_tempF(&LastGoodDegF);
      break;

    case BOARD_ID_510:
    case BOARD_ID_511:

      // This takes about 380 ms
      if (!Gen5_MAX31820_get_tempF(& LastGoodDegF)) LastGoodDegF = 0;  // TODO What is a good sentinel value to use for "bad" temperature ?
      break;

    case BOARD_ID_SEWERWATCH_1:
      Gen45_get_tempF(&LastGoodDegF);
	  break;

    default:
      LastGoodDegF = 0.0;  // TODO What is a good sentinel value to use for "bad" temperature ?
      break;
  }
}
