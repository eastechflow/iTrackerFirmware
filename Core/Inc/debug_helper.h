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

#ifndef __debug_helper_H
#define __debug_helper_H

#include "gpio.h"


extern int RunSensorContinuous;

void debug_led_on(void);

#define STATUS_LED_OFF    (HAL_GPIO_WritePin(GEN45_STATUS_LED_Port, GEN45_STATUS_LED_Pin, 0))         	/*!< Turn off target LED1 */
#define STATUS_LED_ON     (HAL_GPIO_WritePin(GEN45_STATUS_LED_Port, GEN45_STATUS_LED_Pin, 1))          	/*!< Turn on target LED1 */
//#define STATUS_LED_ON     debug_led_on()          	/*!< Turn on target LED1 */
#define STATUS_LED_TOGGLE (GEN45_STATUS_LED_Port->ODR ^= GEN45_STATUS_LED_Pin)          				/*!< Toggle on target LED1 */

#define STATUS2_LED_OFF    (HAL_GPIO_WritePin(GEN5_STATUS2_LED_Port, GEN5_STATUS2_LED_Pin, 0))         	/*!< Turn off target LED2 */
#define STATUS2_LED_ON     (HAL_GPIO_WritePin(GEN5_STATUS2_LED_Port, GEN5_STATUS2_LED_Pin, 1))          /*!< Turn on target LED2 */
#define STATUS2_LED_TOGGLE (GEN5_STATUS2_LED_Port->ODR ^= GEN5_STATUS2_LED_Pin)          				/*!< Toggle on target LED2 */




// List of LED patterns
//
// LED Pattern                              Indication
//
// Red then yellow then both blink once.    Power On
// Red blinks fast.                         Firmware reload in progress.  Must power-cycle when complete.
// Yellow blinks fast.                      Factory extract data in progress. Requires SD card with FactoryExtract.txt on it.

// Red on.                                  Hard Stop 1.  Hardware problem.
// Red and yellow on.                       Hard Stop 2.  Battery or External voltage too low (< 4.4 v).  Must replace batteries or use different external supply.
// Red then yellow once every 3 seconds.    Hard Stop 3.  Battery capacity too low (<=2%).  Only get here when sleeping.  Must remove batteries to restart and use App to reset battery.

// One blink per second.                    Wireless on. Cell off.
//                                          If red, battery capacity greater than 5%.  Cell calls allowed from App.
//                                          If yellow, battery capacity less than 5%.  Cell calls not allowed.
//                                          If battery not reset after 10 min, unit goes to sleep and data logging continues until battery capacity drops below 2%.
//                                          When battery capacity drops below 2%, unit enters Hard Stop 3.
// Two quick blinks per second.             Wireless off. Cell on. If red, battery capacity greater than 5%.  If yellow, battery capacity less than 5% and no Cell calls allowed.
// One blink followed by two quick blinks.  Wireless on. Cell on.  If red, battery capacity greater than 5%.  If yellow, battery capacity less than 5% and no Cell calls allowed.
//
// Red blinks once every 3 seconds.         Light sleep.  Battery capacity greater than 5%.  Data logging, WiFi wakeups, and cell calls allowed.
// Yellow blinks once every 3 seconds.      Light sleep.  Battery capacity greater than 2% but less than 5%. Data logging,  but no cell calls or wifi wakeups.
//                                          When battery capacity drops below 2%, unit enters Hard Stop 3.
// Both leds blink once at wake up.         Awake.  Scheduled data point, wifi wakeup, cell call, magnet, etc.
//

// Red and Yellow blink fast in unison.     Reserved for future use
// Red and Yellow alternate rapidly.        Reserved for future use
// Yellow on.                               Reserved for future use.
// Red and yellow blink steady in unison.   Reserved for future use
// Red blinks pattern slowly                Reserved for future use

//
//
// Complicating factors:  cannot always read voltage or battery percentage due to LTC2943 issues.
// If cannot read LTC2943 then ? enter limited functionality mode ?
// if can read LTC2943 then use voltage to ? use capacity to ?

#define LED_PATTERN_OFF                 0
#define LED_PATTERN_SINGLE_RED          1
#define LED_PATTERN_DOUBLE_RED          2
#define LED_PATTERN_TRIPLE_RED          3
#define LED_PATTERN_SINGLE_DOUBLE_RED   4
#define LED_PATTERN_SINGLE_YEL          5
#define LED_PATTERN_DOUBLE_YEL          6
#define LED_PATTERN_TRIPLE_YEL          7
#define LED_PATTERN_SINGLE_DOUBLE_YEL   8
#define LED_PATTERN_FAST_RED            9
#define LED_PATTERN_FAST_YEL           10
#define LED_PATTERN_SLOW_RED           11
#define LED_PATTERN_SLOW_YEL           12



extern int led_enable_pattern;  // 0 = disables pattern; 1 = enables pattern

int  led_set_pattern(int pattern);  // returns previous pattern
void led_heartbeat(void);
void HAL_led_delay(uint32_t Delay_ms);

void led_power_on_banner(void);
void led_red_blink(void);
void led_yellow_blink(void);
void led_red_and_yellow_blink(void);

void led_hard_stop_1(void);  // red on.  Hardware problem.
void led_hard_stop_2(void);  // red and yellow on. Battery or External voltage too low (< 4.4 v).  Must replace batteries or use different external supply.
void led_hard_stop_3(void);  // Slow alternate red/yellow. Battery capacity too low (<=2%).

// Fast low energy blip.
//void led_heartbeat(void);

// Blink Status LED
//void led_action_heartbeat(int blinks);

// Blink Status LED
//void led_status_report(void);

// Blink Error LED
void led_error_report(void);

// Fast blink for loading demo stuff.
//void led_fast_status(void);

// SENSOR TESTING
//void runSensorContinuous(void);
//void runSensor1000(void);

//#define TEST_CURRENT_DRAW

//#define FORCE_1_MINUTE_DATA_POINTS

#define ENABLE_SPY_UARTS   // VEL1 spies on WiFi/BT;  VEL2 spies on CELL

//#define TEST_SENSOR_MAIN_LOOP    //TestSensorMainLoop


//#define TESTSENSOR_MX_UART_Init       MX_LPUART1_UART_Init
//#define TESTSENSOR_UART_tx_string     lpuart1_tx_string
//#define TESTSENSOR_UART_inbuf         lpuart1_inbuf
//#define TESTSENSOR_UART_inbuf_head    lpuart1_inbuf_head
//#define TESTSENSOR_UART_inbuf_tail    lpuart1_inbuf_tail
//#define TESTSENSOR_UART_flush_inbuffer lpuart1_flush_inbuffer

#define TESTSENSOR_MX_UART_Init       MX_UART4_UART_Init
#define TESTSENSOR_UART_tx_string     uart4_tx_string
#define TESTSENSOR_UART_inbuf         uart4_inbuf
#define TESTSENSOR_UART_inbuf_head    uart4_inbuf_head
#define TESTSENSOR_UART_inbuf_tail    uart4_inbuf_tail
#define TESTSENSOR_UART_flush_inbuffer uart4_flush_inbuffer


#define PACE_UART2_TX

// some sort of problem with LOG_SLEEP_WAKE, the QSPI has some sort of error when logging sleep/wake ??

//#define LOG_SLEEP_WAKE  // Note: this injects bogus data points into the log for logging sleep/wake timestamps and accumulator values

//#define ENABLE_AUTO_WAKEUP


#endif
