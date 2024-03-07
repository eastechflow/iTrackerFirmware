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

#ifndef __MAIN_H__
#define __MAIN_H__

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_pwr_ex.h"
#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "stdbool.h"
#include <ctype.h>
#include "Gen4_TempF_STLM20.h"
#include "Gen5_TempF.h"
#include "gpio.h"
#include "LTC2943.h"
#include "Log.h"
#include "WiFi.h"
#include "WiFi_Socket.h"
#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"

extern __IO uint32_t led_countdown_timer_ms;
extern __IO uint32_t hal_led_delay_timer_ms;
extern __IO uint32_t generic_delay_timer_ms;
extern __IO uint32_t cell_delay_timer_ms;
extern __IO uint32_t wifi_delay_timer_ms;
extern __IO uint32_t UTC_countdown_timer_ms;
extern __IO uint32_t UTC_time_s;

extern int UTC_timezone_offset_hh;

#define UTC_DO_NOT_START_TIMER     0
#define UTC_START_TIMER            1


void _Error_Handler(char *, int);
#define Error_Handler() _Error_Handler(__FILE__, __LINE__)

#endif
