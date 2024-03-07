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

#include "tim.h"
#include "Sensor.h"


TIM_HandleTypeDef htim1; // TIM1 Pulses
TIM_HandleTypeDef htim2; // TIM2 Readings
TIM_HandleTypeDef htim5; // For microsecond delays used by one-wire

uint32_t tim2_CCR1 = 0;

int tim2_isr_total_pulse_count = 0;

uint32_t tim2_min_CNT = 8080705;  // max observed by measuring old code
uint32_t tim2_max_CNT = 0;

#define COMPLEMENTARY_OUTPUT     // This is for GEN5; See AN4776


/**
 * ISR for the capture of the pulses created by TIM1.
 *
 * A pulse sent doesn't guarantee a received pulse. This is why
 * we have gain control.
 *
 *	Save the sensor pulse readings (incoming)
 *  Get the RAW usec that passed by.
 *  Note that one transmit can result in multiple callbacks.
 * Receive Rules:
 *
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)   // TODO This is Gen5 specific (uses CCR1);  Gen4 used CCR4.
{

	tim2_CCR1 = TIM2->CCR1;  // grab the counter immediately
	//TIM2->CNT;

	tim2_isr_total_pulse_count++;

	if (RawPulseCnt < MAX_RAW_PULSES)
	{
		if (tim2_CCR1 >= MINIMUM_ECHO_RAW_PULSE_VALUE)   // Ringdown Blanking: ignore data less than 8.5 inches
		{
		  RawPulseResults[RawPulseCnt] = tim2_CCR1;
		  RawPulseCnt++;
	    }
	}

}


/**
 * PULSE TIMER, TIM1
 *
 * This timer sends the 75Khz pulse to the sensor driver pin to fire the ultrasonics.
 *
 * Timing:
 *
 * SystemCoreClock: 80,000,000 Hz
 * SENSOR_FREQ: 66,000 Hz
 * SENSOR_PERIOD: 1066 = SystemCoreClock / SENSOR_FREQ
 * SENSOR_DUTY_CYC: 533 = percent of SENSOR_PERIOD (50% is good, maybe higher for foam????)
 * SENSOR_PRESCALE: ((SystemCoreClock / 1,000,000) - 1)  = 79;
 */

void MX_TIM1_Init(void)  //TODO - must add a run-time switch between Gen4 and Gen5
{

	uint32_t uwPrescalerValue;

	__HAL_RCC_TIM1_CLK_ENABLE();

	htim1.Instance = TIM1;

	HAL_TIM_MspPostInit(&htim1);

	// Select the up counter mode
	TIM1->CR1 &= ~(TIM_CR1_DIR | TIM_CR1_CMS);
	TIM1->CR1 |= TIM_COUNTERMODE_UP;

	// Set the clock division to 1
	TIM1->CR1 &= ~TIM_CR1_CKD;
	TIM1->CR1 |= TIM_CLOCKDIVISION_DIV1;

	// PULSE WIDTH BURST CONFIGURATION
#define PRESCALER_DIV 12000000

//#define SENSOR_FREQ		75000 // too high
//#define SENSOR_FREQ	  67285  // too low
#define SENSOR_FREQ	  67500      // measured 74,766 Hz so this looks optimal on the iTracker board with the new transformer center tap
//#define SENSOR_FREQ	  68000  // too high at	75.187

#define PERIOD_VAL    PRESCALER_DIV / SENSOR_FREQ
#define PULSE_VAL     PERIOD_VAL * 0.5		//50 duty cycle
#define SENSOR_PULSES	10
	uwPrescalerValue = (uint32_t)((SystemCoreClock) / PRESCALER_DIV) - 1;
	TIM1->ARR  = PERIOD_VAL;
	TIM1->CCR1 = PULSE_VAL;
	TIM1->PSC  = uwPrescalerValue;
	TIM1->RCR  = SENSOR_PULSES - 1;

#if 0  // zg - this worked, moved lower to see if still worked
	// Generate an update event to reload the Prescaler and the repetition counter value immediately
	TIM1->EGR |= TIM_EGR_UG;   // This can send a TRGO to whoever is listening
#endif


	// Configure the One Pulse Mode
	// Select the OPM Mode and optionally Fast Enable (see page 984 of RM0351)
	TIM1->CR1   |= TIM_CR1_OPM;
	//TIM1->CCMR1 |= TIM_CCMR1_OC1FE;

	// Select the Channel 1 Output Compare and the Mode
	//TIM1->CCMR1 &= ~TIM_CCMR1_OC1PE;   /* Disable the Output Compare 1 Preload so changes to CCMR1 take effect immediately */
	TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
	TIM1->CCMR1 &= ~TIM_CCMR1_CC1S;
	TIM1->CCMR1 |=  TIM_OCMODE_PWM2;

	// Set the Output Compare Polarity to High
	TIM1->CCER &= ~TIM_CCER_CC1P;
	TIM1->CCER |= TIM_OCPOLARITY_HIGH;
	TIM1->CCER |= TIM_CCER_CC1E;


#ifdef COMPLEMENTARY_OUTPUT     // TODO This is for Gen5, Gen4 is single level output
	/* Select active High as output Complementary polarity level */

	TIM1->CCER &= ~TIM_CCER_CC1NP;	     /* Reset the Output N State */
	TIM1->CCER |= TIM_OCNPOLARITY_HIGH;  /* Set the Output N Polarity to high level */
	TIM1->CCER |= TIM_CCER_CC1NE;        /* Enable CC1 output */
	//TIM1->CR2  |= TIM_CR2_OIS1N;       /* Set Complementary Idle state */

	TIM1->DIER |= TIM_DIER_UIE;   /* Enable the Update Interrupt so outputs can be forced low after the pulse train is complete */
#endif

#if 0

	/******************* COM Control update configuration ******************/

	TIM1->CR2 |= TIM_CR2_CCPC;  /* Set the Capture Compare Preload */
	TIM1->CR2 |= TIM_CR2_CCUS;  /* Set CCUS bit to select the COM control update to trigger input TRGI*/
	TIM1->DIER |= TIM_DIER_COMIE;   /* Enable the Commutation Interrupt sources */
	//TIM1->CCMR1 |= TIM_CCMR1_OC1PE;   /* Enable the Output Compare 1 Preload - subsequent changes to CMMR1 only affect the preload register */

    // These now only affect the preload register, not the shadow register
    //TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;              /* Reset the OC1M bits in the CCMR1 register */
    //TIM1->CCMR1 |= TIM_OCMODE_FORCED_INACTIVE;   /* configure the OC1M bits in the CCMRx register to inactive mode*/

    //TIM1->BDTR &= ~TIM_BDTR_OSSR;   // no effect
    //TIM1->BDTR |=  TIM_BDTR_OSSR;  //

    //TIM1->BDTR |= TIM_BDTR_OSSI;   //no effect

#endif

	// Master mode configuration: Trigger Reset mode
	TIM1->CR2 &= ~TIM_CR2_MMS;
	TIM1->CR2 |=  TIM_TRGO_RESET;   // The UG bit from the EGR register is used as a trigger output (TRGO)

#if 1  // zg - this worked, unclear where exactly to leave it
	// Generate an update event to reload the Prescaler and the repetition counter value immediately
	TIM1->EGR |= TIM_EGR_UG;   // This can send a TRGO to whoever is listening
#endif

	// Enable the TIM main Output
	TIM1->BDTR |= TIM_BDTR_MOE;

	TIM1->SR &= ~TIM_SR_UIF;              // clear any pending UPDATE interrupt
    NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);   // enable the UPDATE interrupt

}

void TIM1_UP_TIM16_IRQHandler(void)
{
  uint32_t sr;
  // read and clear the ISR reason
  sr = TIM1->SR;
  TIM1->SR = ~sr;

  // react only on the UPDATE-triggered interrupts (others should be disabled in DIER, as well as TIM17)
  if (sr & TIM_SR_UIF)
  {
  	// Disable the TIM main Output
  	TIM1->BDTR &= ~TIM_BDTR_MOE;  // This works for allowing both outputs to go low
    NVIC_DisableIRQ(TIM1_UP_TIM16_IRQn);  // prevent further UPDATE interrupts until the next cycle
  }
}


void us_Timer_Init(void)
{
	uint32_t gu32_ticks = (HAL_RCC_GetHCLKFreq() / 1000000);

	__HAL_RCC_TIM5_CLK_ENABLE();

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    htim5.Instance = TIM5;
    htim5.Init.Prescaler = gu32_ticks-1;
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 65535;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
    {
      Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
    {
      Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
    {
      Error_Handler();
    }
    HAL_TIM_Base_Start(&htim5);
}

void delay_us(uint16_t microseconds)
{
	htim5.Instance->CNT = 0;
    while (htim5.Instance->CNT < microseconds);
}


/**
 * RECEIVE/MEASURE TIMER - TIM2
 *
 * Count every 1us.
 *
 */
void MX_TIM2_Init(void)  //TODO - must add a run-time switch between Gen4 and Gen5
{

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xffffffff;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  TIM_IC_InitTypeDef sConfigIC;
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }



#if 1 // zg this is for Gen5
	/******* Slave mode configuration: Trigger mode *********/
	/* Select the TIM_TS_ITR0 signal as Input trigger for TIM2 */
	TIM2->SMCR &= ~TIM_SMCR_TS;
	TIM2->SMCR |= TIM_TS_ITR0;   // zg - this is TIM1_TRGO see reference manual RM0351 page 1066
	/* Select the Slave Mode */
	TIM2->SMCR &= ~TIM_SMCR_SMS;
	TIM2->SMCR |= TIM_SLAVEMODE_COMBINED_RESETTRIGGER;
	/******************************************************************/
#endif

  /* Set the TIMx priority */
  TIM2->SR &= ~TIM_SR_CC1OF;
  HAL_NVIC_SetPriority(TIM2_IRQn, 2, 0);

  /* Enable the TIMx global Interrupt */
  HAL_NVIC_EnableIRQ(TIM2_IRQn);

}


void HAL_TIM_OC_MspInit(TIM_HandleTypeDef* tim_ocHandle)
{
  if (tim_ocHandle->Instance==TIM1)
  {
    __HAL_RCC_TIM1_CLK_ENABLE();
  }
}


void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(timHandle->Instance==TIM1)
  {

#ifdef COMPLEMENTARY_OUTPUT  // This requires Gen5 AND a new Transformer
	    GPIO_InitStruct.Pin = GEN5_SENSOR_DRVn_Pin | GEN45_SENSOR_DRVp_Pin;  // GPIO_PIN_8 | GPIO_PIN_9
	    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	    GPIO_InitStruct.Pull = GPIO_PULLDOWN;        // Any time the timer is NOT controlling the pin, pull it low
	    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
#else
    GPIO_InitStruct.Pin = GEN45_SENSOR_DRVp_Pin;    //   GPIO_PIN_9
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
#endif
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);   // GEN5_SENSOR_DRVn_Port, GEN45_SENSOR_DRVp_Port
  }
}

void HAL_TIM_OC_MspDeInit(TIM_HandleTypeDef* tim_ocHandle)
{
  if (tim_ocHandle->Instance == TIM1)
  {
	/* Peripheral clock disable */
    __HAL_RCC_TIM1_CLK_DISABLE();
  }
}

// This is called for TIM2
void HAL_TIM_IC_MspInit(TIM_HandleTypeDef* tim_icHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  if (tim_icHandle->Instance == TIM2)
  {
    __HAL_RCC_TIM2_CLK_ENABLE();

    GPIO_InitStruct.Pin = GEN5_HIGH_SIG_OUT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;    // ???
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;  // 0x1 which means TIM2_CH1
    HAL_GPIO_Init(GEN5_HIGH_SIG_OUT_Port, &GPIO_InitStruct);

  }
}

void HAL_TIM_IC_MspDeInit(TIM_HandleTypeDef* tim_icHandle)
{
  if (tim_icHandle->Instance == TIM2)
  {
    /* Peripheral clock disable */
    __HAL_RCC_TIM2_CLK_DISABLE();
  
//    HAL_GPIO_DeInit(Sensor_IN_Port, Sensor_IN_Pin);
    gpio_unused_pin(GEN5_HIGH_SIG_OUT_Port,  GEN5_HIGH_SIG_OUT_Pin);
  }
}
