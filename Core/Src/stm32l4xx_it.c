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

#include "debug_helper.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx.h"
#include "stm32l4xx_it.h"
#include "quadspi.h"

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
extern RTC_HandleTypeDef hrtc;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;


/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void) {
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void) {
	while (1) {
		led_error_report();
	}
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void) {
	while (1) {
		led_error_report();
	}
}

/**
 * @brief This function handles Prefetch fault, memory access fault.
 */
void BusFault_Handler(void) {
	while (1) {
		led_error_report();
	}
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void) {
	while (1) {
		led_error_report();
	}
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void) {
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void) {
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void) {
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void) {
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
}

/******************************************************************************/
/* STM32L4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32l4xx.s).                    */
/******************************************************************************/

/**
 * @brief This function handles RTC alarm interrupt through EXTI line 18.
 */
//void RTC_Alarm_IRQHandler(void) {
//    HAL_RTC_AlarmIRQHandler(&hrtc);
//	if (__HAL_RTC_ALARM_GET_IT_SOURCE(&hrtc, RTC_IT_ALRA) != RESET) {
//		__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
//		__HAL_RTC_ALARM_EXTI_CLEAR_FLAG();
//	}
//}

void RTC_Alarm_IRQHandler(void) {
    HAL_RTC_AlarmIRQHandler(&hrtc);
}

void RTC_WKUP_IRQHandler(void) {
    HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
}

/**
 * @brief This function handles USB OTG FS global interrupt.
 */
void OTG_FS_IRQHandler(void) {
	HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}

void TIM1_IRQHandler(void) {
	HAL_TIM_IRQHandler(&htim1);
}

void TIM2_IRQHandler(void) {
	HAL_TIM_IRQHandler(&htim2);
}
#if 0
void TIM3_IRQHandler(void) {
	HAL_TIM_IRQHandler(&htim3);
}

void TIM5_IRQHandler(void) {
	HAL_TIM_IRQHandler(&htim5);
}
#endif

#if 1
/**
  * @brief This function handles EXTI line[9:5] interrupts.
  */
void EXTI9_5_IRQHandler(void)
{
	extern __IO uint32_t WakeUpFrom_EXTI;  // PA8 is USD_DETECT, active low.   Tied to SD Card.
	WakeUpFrom_EXTI = 1;
	//STATUS2_LED_ON;
  // Don't need to do anything special for PA8, just call the default handler to clear the interrupt.
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
}
#endif

#if 0


/**
  * @brief  EXTI line detection callback.
  * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI line.
  * @retval None
  */
int a;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

  a = 1;
  a = a + 1;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(GPIO_Pin);
}
#endif

