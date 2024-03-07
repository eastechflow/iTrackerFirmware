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

#include "Common.h"
#include "Gen4_TempF_STLM20.h"
#include "adc.h"
#include "stm32l4xx_hal_adc.h"
#include "debug_helper.h"
#include "trace.h"
#include "tim.h"

#define TEMP_AVG_SAMPLES 10

void Gen45_get_tempF(float * result)
{
	char msg[80];
	float tmpfloat = 0;
	float tmpfloat2 = 0;
	int8_t stat = 0;
	uint8_t kcnt = 0;
	uint32_t channel;

	if (isGen5())
	{
      /* Switch VDD_3V3 to PWM mode to reduce noise during sensor measurements */
	  HAL_GPIO_WritePin(GEN5_VDD_3V3_SYNC_Port, GEN5_VDD_3V3_SYNC_Pin, GPIO_PIN_SET);   // Low noise mode (PWM)
	  HAL_Delay(5);    // Wait for 3V3 to stabilize
	}


	if (BoardId == BOARD_ID_SEWERWATCH_1)
	{
		HAL_GPIO_WritePin(GEN5_SEWERWATCH_TEMP_DRV_Port, GEN5_SEWERWATCH_TEMP_DRV_Pin, GPIO_PIN_SET);
		delay_us(850);  // see datasheet
		channel = SEWERWATCH_TEMP_AD_CHAN; // SEWERWATCH_TEMP_AD_CHAN is PC3
	}
	else
	{
		channel = Temp_Snr_CHAN; // Temp_Snr_CHAN is PC2
	}

	Gen4_MX_ADC1_Init();
	HAL_Delay(1);

#ifdef USE_NEW_ADC_CODE
	stat = Gen45_config_and_calibrate(channel);
	if (stat != HAL_OK)
	{
		sprintf(msg, "stat error %d\n", stat);
		trace_AppendMsg(msg);
	}
#endif

//#define TRACE_THERM

#ifdef TRACE_THERM
	trace_AppendMsg("Therm");
#endif

	for (int k = 0; k < TEMP_AVG_SAMPLES; k++)
	{
		//uint32_t tickstart;
		//uint32_t duration;
		//tickstart = HAL_GetTick();


#ifdef USE_NEW_ADC_CODE
		stat = Gen45_start_adc_and_read_voltage(&tmpfloat);
#else
		stat = Gen45_read_voltage(channel, &tmpfloat);
#endif

		//duration = HAL_GetTick() - tickstart;

		if (stat == HAL_OK)
		{

			tmpfloat2 += tmpfloat;
			kcnt++;
#ifdef TRACE_THERM
			// For testing
			{

				int dec1, dec2;
				int dec3, dec4;
				float deg_C;
				if (BoardId == BOARD_ID_GEN4)
				{
					deg_C = (tmpfloat - 1.8641) / -0.01171;  // convert voltage to deg C
				}
				else if (BoardId == BOARD_ID_SEWERWATCH_1)
				{
				  deg_C = ((tmpfloat - 0.5) / 0.010) + 0.0;  // convert voltage to deg C  (SewerWatch)
				}
				else
				{
					deg_C = 25.0;
				}
				convfloatstr_4dec(tmpfloat, &dec1, &dec2);
				convfloatstr_2dec(deg_C, &dec3, &dec4);
				//sprintf(msg, ", %d.%04d, %d.%02d\n", dec1, dec2, dec3, dec4);
				sprintf(msg, ", %d.%02d", dec3, dec4);
				trace_AppendMsg(msg);
			}
#endif

		}
		else
		{
#ifdef TRACE_THERM
			sprintf(msg, " stat error %d", stat);
			trace_AppendMsg(msg);
#endif

			// unclear why the adc is reset here...
			HAL_ADC_DeInit(&hadc1);
			HAL_Delay(1);
			Gen4_MX_ADC1_Init();
			HAL_Delay(1);
			stat = Gen45_config_and_calibrate(channel);

		}
		HAL_Delay(10); // Needed for read differential.
	}

#ifdef TRACE_THERM
	trace_AppendMsg("\n");
#endif

	if (kcnt)
	{
		tmpfloat = tmpfloat2 / kcnt;

		if (BoardId == BOARD_ID_GEN4)
		{
			tmpfloat2 = (tmpfloat - 1.8641) / -0.01171;  // convert voltage to deg C
		}
		else if (BoardId == BOARD_ID_SEWERWATCH_1)
		{
			// TMP234AQDBz voltage output device
			// Vout = (Ta - Tinfl) * Tc + VOFFS
			// Tinfl = 0
			// Tc = 10 (mV/DegC)
			// Voffs = 500 mV
			// Ta = ((Vout - Voffs) / Tc) + Tinfl
			tmpfloat2 = ((tmpfloat - 0.5) / 0.010) + 0.0;  // convert voltage to deg C
		}
		else
		{
			tmpfloat2 = 21.111; // same as 70.0 F
		}

		*result = (tmpfloat2 * 9.0 / 5.0) + 32.0;  // convert deg C to deg F

	} else {
		*result = 70.0; // in F
	}

	// Don't leave the ADC running
	HAL_ADC_DeInit(&hadc1);

	if (BoardId == BOARD_ID_SEWERWATCH_1)
	{
		HAL_GPIO_WritePin(GEN5_SEWERWATCH_TEMP_DRV_Port, GEN5_SEWERWATCH_TEMP_DRV_Pin, GPIO_PIN_RESET);
	}

	// When turning power OFF, change the 3V3 supply back to low-power mode
	if (isGen5())
	{
	    /* Switch VDD_3V3 to low power mode */
		HAL_GPIO_WritePin(GEN5_VDD_3V3_SYNC_Port, GEN5_VDD_3V3_SYNC_Pin, GPIO_PIN_RESET);   // Low-power mode
	}
}

void Gen5_get_board_id_voltage(float * result)
{
	// Gen5 code

	float tmpfloat = 0;
	float tmpfloat2 = 0;
	int8_t stat = 0;
	uint8_t kcnt = 0;

	Gen4_MX_ADC1_Init();
	HAL_led_delay(1);

	//for (int k = 0; k < 3; k++)
	for (int k = 0; k < 10; k++)
	{
		stat = Gen5_Read_Board_ID(&tmpfloat);  // this takes on the order of 10 ms
		if (!stat)
		{
			tmpfloat2 += tmpfloat;
			kcnt++;
#if 1
			// For testing if resistor divider network is really there
			{
				char msg[80];
				int dec1, dec2;
				convfloatstr_4dec(tmpfloat, &dec1, &dec2);
				sprintf(msg, "%d.%04d\n", dec1, dec2);
				trace_AppendMsg(msg);
			}
#endif
		}
		else
		{
			HAL_ADC_DeInit(&hadc1);
			HAL_led_delay(1);
			Gen4_MX_ADC1_Init();
			HAL_led_delay(1);
			trace_AppendMsg("stat error\n");
		}
		HAL_led_delay(10); // Needed for read differential.
	}

	if (kcnt)
	{
		tmpfloat = tmpfloat2 / kcnt;
		tmpfloat2 = tmpfloat;
		*result = tmpfloat2;
	}
	else
	{
		*result = 0.0;
	}

#if 1
			// For testing if resistor divider network is really there on iTracker5 boards
			{
				char msg[80];
				int dec1, dec2;
				convfloatstr_4dec(*result, &dec1, &dec2);
				sprintf(msg, "BoardId voltage: %d.%04d\n", dec1, dec2);
				trace_AppendMsg(msg);
			}
#endif

	// Don't leave the ADC running
	HAL_ADC_DeInit(&hadc1);

	// When turning power OFF, change the 3V3 supply back to low-power mode
	if (isGen5())
	{
	    /* Switch VDD_3V3 to low power mode */
		HAL_GPIO_WritePin(GEN5_VDD_3V3_SYNC_Port, GEN5_VDD_3V3_SYNC_Pin, GPIO_PIN_RESET);   // Low-power mode
	}
}
