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

#ifndef __adc_H
#define __adc_H
#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;

#define VDD_APPLI                      ((uint32_t)3300)    /* Value of analog voltage supply Vdda (unit: mV) */
#define RANGE_12BITS                   ((uint32_t)4095)    /* Max value with a full range of 12 bits */
#define TIMER_FREQUENCY_HZ             ((uint32_t)1000)    /* Timer frequency (unit: Hz). With SysClk set to 2MHz, timer frequency TIMER_FREQUENCY_HZ range is min=1Hz, max=33.3kHz. */

#define COMPUTATION_DIGITAL_12BITS_TO_VOLTAGE(ADC_DATA)                        \
  ( (ADC_DATA) * VDD_APPLI / RANGE_12BITS)

#define VH_Pwr_CHAN 	ADC_CHANNEL_2  //ADC_CHSELR_CHSEL8
#define Temp_Snr_CHAN	ADC_CHANNEL_3  // on pin PC2
#define Temp_NTC_CHAN	ADC_CHANNEL_4  // on pin PC3
#define ITRACKER5_Board_ID_CHAN   ADC_CHANNEL_4  // on pin PC3
#define SEWERWATCH_TEMP_AD_CHAN   ADC_CHANNEL_4  // on pin PC3

extern void _Error_Handler(char *, int);

int8_t Gen4_MX_ADC1_Init(void);

int8_t read_rtc_batt(float * temp);
//int8_t read_VH_Pwr(float * temp);

#define USE_NEW_ADC_CODE

#ifdef USE_NEW_ADC_CODE
int8_t Gen45_config_and_calibrate(uint32_t Channel);
int8_t Gen45_start_adc_and_read_voltage(float * temp);
#endif


int8_t Gen45_read_voltage(uint32_t Channel, float * temp);
int8_t Gen5_Read_Board_ID(float * temp);

#ifdef __cplusplus
}
#endif
#endif
