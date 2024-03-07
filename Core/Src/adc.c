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

#include "adc.h"
#include "gpio.h"
#include "Common.h"

#define PS_VDD	3.3
#define ADC_VCONV  PS_VDD / RANGE_12BITS
#define BATT_CONV  11 // 1/0.091 //  100 / 1100
#define VH_CONV		(470+100)/100



ADC_HandleTypeDef hadc1;



int8_t Gen4_MX_ADC1_Init(void)
{
	// Gen4 code

	ADC_MultiModeTypeDef multimode;
	ADC_ChannelConfTypeDef sConfig;

	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.NbrOfDiscConversion = 1;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
	hadc1.Init.OversamplingMode = DISABLE;

	if (HAL_ADC_Init(&hadc1) != HAL_OK) {
		return -1;
	}

	multimode.Mode = ADC_MODE_INDEPENDENT;
	if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK) {
		return -2;
	}

	sConfig.Channel = ADC_CHANNEL_2;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;

	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		return -3;
	}
	return 0;
}



void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

	GPIO_InitTypeDef GPIO_InitStruct;
	if (adcHandle->Instance == ADC1)
	{
		__HAL_RCC_ADC_CLK_ENABLE();

		if (isGen4())
		{
			GPIO_InitStruct.Pin = GEN4_VH_PWR_ADC_Pin | GEN4_Temp_SNR_ADC_Pin; //|GEN4_Temp_NTC_ADC_Pin;
			GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
			GPIO_InitStruct.Pull = GPIO_NOPULL;
			HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
		}
		if (isGen5())
		{
			GPIO_InitStruct.Pin  = GEN5_BOARD_ID_ADC_Pin;     // PC3 is used for either BoardId (iTracker5) or Temperature (SewerWatch)
			GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
			GPIO_InitStruct.Pull = GPIO_NOPULL;
			//GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			//GPIO_InitStruct.Pull = GPIO_PULLUP;
			HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
		}

	}
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{
	if (adcHandle->Instance == ADC1)
	{
		__HAL_RCC_ADC_CLK_DISABLE();
		if (isGen4())
		{
		  //HAL_GPIO_DeInit(GPIOC, GEN4_VH_PWR_ADC_Pin | GEN4_Temp_SNR_ADC_Pin );  // PC1 and PC2
		  gpio_unused_pin(PC1);
		  gpio_unused_pin(PC2);
		}
		if (isGen5())
		{
		  //HAL_GPIO_DeInit(GPIOC, GEN5_BOARD_ID_ADC_Pin );  // PC3
		  gpio_unused_pin(PC3);
		}
	}

}

#ifdef USE_NEW_ADC_CODE
int8_t Gen45_config_and_calibrate(uint32_t Channel)
{
	ADC_ChannelConfTypeDef sConfig;
	int8_t stat = 0;

	sConfig.Channel = Channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;

	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		stat = -1;
	}

	/* Run the ADC calibration in single-ended mode */
	else if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
		stat = -2;
	}

	return stat;
}

int8_t Gen45_start_adc_and_read_voltage(float * temp)
{
	int8_t stat;

	/*##-3- Start the conversion process #######################################*/
	if (HAL_ADC_Start(&hadc1) != HAL_OK) {
		stat = -3;
	}

	/*##-4- Wait for the end of conversion #####################################*/
	/*  For simplicity reasons, this example is just waiting till the end of the
		 conversion, but application may perform other tasks while conversion
		 operation is ongoing. */
	else if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
		stat = -4;
	} else {
		uint32_t adc_res;
		/* ADC conversion completed */
		/*##-5- Get the converted value of regular channel  ########################*/
		adc_res = HAL_ADC_GetValue(&hadc1);

		/* Compute the voltage */
		*temp = (float) adc_res * ADC_VCONV;
	}

	return stat;
}

int8_t Gen45_read_voltage(uint32_t Channel, float * temp)
{
  int8_t stat;

  stat = Gen45_config_and_calibrate(Channel);

  if (stat != HAL_OK)
  {
	  stat = -1;
  }
  else
  {
	  stat = Gen45_start_adc_and_read_voltage(temp);
  }

  return stat;
}

#else // original

int8_t Gen45_read_voltage(uint32_t Channel, float * temp)
{
	ADC_ChannelConfTypeDef sConfig;
	int8_t stat = 0;

	sConfig.Channel = Channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;

	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		stat = -1;
	}

	/* Run the ADC calibration in single-ended mode */
	else if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
		stat = -2;
	}

	/*##-3- Start the conversion process #######################################*/
	else if (HAL_ADC_Start(&hadc1) != HAL_OK) {
		stat = -3;
	}

	/*##-4- Wait for the end of conversion #####################################*/
	/*  For simplicity reasons, this example is just waiting till the end of the
	 conversion, but application may perform other tasks while conversion
	 operation is ongoing. */
	else if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
		stat = -4;
	} else {
		uint32_t adc_res;
		/* ADC conversion completed */
		/*##-5- Get the converted value of regular channel  ########################*/
		adc_res = HAL_ADC_GetValue(&hadc1);

		/* Compute the voltage */
		*temp = (float) adc_res * ADC_VCONV;
	}

	return stat;
}
#endif

#if 1  // never used in Gen 4

int8_t read_rtc_batt(float * temp)
{
	ADC_ChannelConfTypeDef sConfig;
	float result;
	uint32_t adc_res;
	int8_t stat = 0;

	sConfig.Channel = ADC_CHANNEL_VBAT;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;

	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		stat = -1;
	}

	/* Run the ADC calibration in single-ended mode */
	else if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
		stat = -2;
	}

	/*##-3- Start the conversion process #######################################*/
	else if (HAL_ADC_Start(&hadc1) != HAL_OK) {
		stat = -3;
	}

	/*##-4- Wait for the end of conversion #####################################*/
	/*  For simplicity reasons, this example is just waiting till the end of the
	 conversion, but application may perform other tasks while conversion
	 operation is ongoing. */
	else if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
		stat = -4;
	} else {
		/* ADC conversion completed */
		/*##-5- Get the converted value of regular channel  ########################*/
		adc_res = HAL_ADC_GetValue(&hadc1);

		/* Compute the voltage */
		result = (float) adc_res * ADC_VCONV;
		*temp = result * 3.926;
		//*temp = result * 3.0;
	}
	return stat;
}
#endif

int8_t Gen5_Read_Board_ID(float * temp)
{
	ADC_ChannelConfTypeDef sConfig;
	uint32_t adc_res;
	int8_t stat = 0;

	sConfig.Channel = ITRACKER5_Board_ID_CHAN;  // PC3
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;

	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		stat = -1;
	}

	/* Run the ADC calibration in single-ended mode */
	else if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
		stat = -2;
	}

	/*##-3- Start the conversion process #######################################*/
	else if (HAL_ADC_Start(&hadc1) != HAL_OK) {
		stat = -3;
	}

	/*##-4- Wait for the end of conversion #####################################*/
	/*  For simplicity reasons, this example is just waiting till the end of the
conversion, but application may perform other tasks while conversion
operation is ongoing. */
	else if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
		stat = -4;
	} else {
		/* ADC conversion completed */
		/*##-5- Get the converted value of regular channel  ########################*/
		adc_res = HAL_ADC_GetValue(&hadc1);

		/* Compute the voltage */
		*temp = (float) adc_res * ADC_VCONV;
	}

	return stat;
}



