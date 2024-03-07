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
//#include "Sensor.h"




int led_enable_pattern = 1;   // At power on, pattern is enabled.  After that, it is enabled only if WiFi is active.

static int led_pattern = LED_PATTERN_SINGLE_RED;  // At power on, single red pulse pattern is active

static int led_state = 1;

// Want to indicate: WiFi Active, Cell Active, and when Battery Capacity is less than 5%.
//
// o....o....o....o     WiFi only          Single
// o....oo...o....oo    WiFi and Cell      Single,Double alternating
// oo...oo...oo...oo    Cell only          Double
// ooo..ooo..ooo..ooo   Data log only      Triple
//
// Red    = battery capacity > 5%
// Yellow = battery capacity <5%

// Double Pulse
//  75 ms on 100 ms off
//  75 ms on 100 ms off
// 400 ms off




int led_set_pattern(int pattern)
{
	int prev_pattern;

	prev_pattern = led_pattern;

	if (prev_pattern == pattern) return prev_pattern;  // pattern has not changed, let it continue to operate, do not restart

    // Be sure both LEDs are off when starting any pattern
    STATUS_LED_OFF;
    STATUS2_LED_OFF;

	if (led_enable_pattern)
	{
		// switch to new pattern requested
		led_pattern = pattern;
	}
	else
	{
		// force LED off
		led_pattern = LED_PATTERN_OFF;
	}

	led_state = 1;  // by definition, all patterns start in state 1

	// Force the pattern to start
	led_countdown_timer_ms = 0;
	led_heartbeat();
	return prev_pattern;
}

void led_power_on_banner(void)
{

  STATUS_LED_ON;
  HAL_Delay(100);
  STATUS2_LED_ON;
  HAL_Delay(200);

  STATUS_LED_OFF;
  STATUS2_LED_OFF;
  HAL_Delay(250);

  STATUS_LED_ON;
  STATUS2_LED_ON;
  HAL_Delay(200);

  STATUS_LED_OFF;
  STATUS2_LED_OFF;
  HAL_Delay(250);

}

void led_red_blink(void)
{
	  STATUS_LED_ON;
	  STATUS2_LED_OFF;
	  HAL_Delay(100);
	  STATUS_LED_OFF;
}

void led_yellow_blink(void)
{
	  STATUS_LED_OFF;
	  STATUS2_LED_ON;
	  HAL_Delay(100);
	  STATUS2_LED_OFF;
}

void led_red_and_yellow_blink(void)
{
	  STATUS_LED_ON;
	  STATUS2_LED_ON;
	  HAL_Delay(30);
	  STATUS_LED_OFF;
	  STATUS2_LED_OFF;
}

void led_hard_stop_1(void)  // red on.  Hardware problem.
{
	STATUS_LED_ON;
	STATUS2_LED_OFF;
	while (1);
}

void led_hard_stop_2(void)  // red and yellow on. Battery or External voltage too low (< 4.4 v).  Must replace batteries or use different external supply.
{
	STATUS_LED_ON;
	STATUS2_LED_ON;
	while (1);
}

void led_hard_stop_3(void)  // Red and yellow on in unison once every 4 seconds. Battery capacity too low (<=2%).
{
	  STATUS_LED_ON;
	  STATUS2_LED_ON;
	  HAL_Delay(30);
	  STATUS_LED_OFF;
	  STATUS2_LED_OFF;
}

static void pattern_off(void) // LED_PATTERN_OFF
{
  STATUS_LED_OFF;
  STATUS2_LED_OFF;
}


static void pattern_single_red(void)
{
	STATUS2_LED_OFF; // make sure yellow is off
	switch (led_state)
	{
		case 1: STATUS_LED_ON;  led_countdown_timer_ms =  68; led_state = 2; break;  //  was 10 ms ON
		case 2: STATUS_LED_OFF; led_countdown_timer_ms = 950; led_state = 1; break;  // was 490 ms OFF
	}
}

static void pattern_single_yellow(void)
{
	STATUS_LED_OFF; // make sure red is off
	switch (led_state)
	{
		case 1: STATUS2_LED_ON;  led_countdown_timer_ms =  68; led_state = 2; break;  //  was 10 ms ON
		case 2: STATUS2_LED_OFF; led_countdown_timer_ms = 950; led_state = 1; break;  // was 190 ms OFF
	}
}

static void pattern_double_red(void)
{
	STATUS2_LED_OFF; // make sure yellow is off
	switch (led_state)
	{
	    default:
		case 1: STATUS_LED_ON;  led_countdown_timer_ms =    68; led_state = 2; break;
		case 2: STATUS_LED_OFF; led_countdown_timer_ms =   280; led_state = 3; break;
		case 3: STATUS_LED_ON;  led_countdown_timer_ms =    68; led_state = 4; break;
		case 4: STATUS_LED_OFF; led_countdown_timer_ms =   950; led_state = 1; break;
	}
}


static void pattern_double_yellow(void)
{
	STATUS_LED_OFF; // make sure red is off
	switch (led_state)
	{
	    default:
		case 1: STATUS2_LED_ON;  led_countdown_timer_ms =    68; led_state = 2; break;
		case 2: STATUS2_LED_OFF; led_countdown_timer_ms =   280; led_state = 3; break;
		case 3: STATUS2_LED_ON;  led_countdown_timer_ms =    68; led_state = 4; break;
		case 4: STATUS2_LED_OFF; led_countdown_timer_ms =   950; led_state = 1; break;
	}
}

static void pattern_triple_red(void)
{
	STATUS2_LED_OFF; // make sure yellow is off
	switch (led_state)
	{
		case 1: STATUS_LED_ON;  led_countdown_timer_ms =  68; led_state = 2; break;
		case 2: STATUS_LED_OFF; led_countdown_timer_ms = 280; led_state = 3; break;
		case 3: STATUS_LED_ON;  led_countdown_timer_ms =  68; led_state = 4; break;
		case 4: STATUS_LED_OFF; led_countdown_timer_ms = 280; led_state = 5; break;
		case 5: STATUS_LED_ON;  led_countdown_timer_ms =  68; led_state = 6; break;
		case 6: STATUS_LED_OFF; led_countdown_timer_ms = 950; led_state = 1; break;
	}
}



static void pattern_triple_yellow(void)
{
	STATUS_LED_OFF; // make sure red is off
	switch (led_state)
	{
		case 1: STATUS2_LED_ON;  led_countdown_timer_ms =  68; led_state = 2; break;
		case 2: STATUS2_LED_OFF; led_countdown_timer_ms = 280; led_state = 3; break;
		case 3: STATUS2_LED_ON;  led_countdown_timer_ms =  68; led_state = 4; break;
		case 4: STATUS2_LED_OFF; led_countdown_timer_ms = 280; led_state = 5; break;
		case 5: STATUS2_LED_ON;  led_countdown_timer_ms =  68; led_state = 6; break;
		case 6: STATUS2_LED_OFF; led_countdown_timer_ms = 950; led_state = 1; break;
	}
}

static void pattern_single_double_red(void)
{
	STATUS2_LED_OFF; // make sure yellow is off
	switch (led_state)
	{
	    default:
		case 1: STATUS_LED_ON;  led_countdown_timer_ms =    68; led_state = 2; break;
		case 2: STATUS_LED_OFF; led_countdown_timer_ms =   950; led_state = 3; break;

		case 3: STATUS_LED_ON;  led_countdown_timer_ms =    68; led_state = 4; break;
		case 4: STATUS_LED_OFF; led_countdown_timer_ms =   280; led_state = 5; break;
		case 5: STATUS_LED_ON;  led_countdown_timer_ms =    68; led_state = 6; break;
		case 6: STATUS_LED_OFF; led_countdown_timer_ms =   950; led_state = 1; break;
	}
}

static void pattern_single_double_yellow(void)
{
	STATUS_LED_OFF; // make sure red is off
	switch (led_state)
	{
	    default:
		case 1: STATUS2_LED_ON;  led_countdown_timer_ms =    68; led_state = 2; break;
		case 2: STATUS2_LED_OFF; led_countdown_timer_ms =   950; led_state = 3; break;

		case 3: STATUS2_LED_ON;  led_countdown_timer_ms =    68; led_state = 4; break;
		case 4: STATUS2_LED_OFF; led_countdown_timer_ms =   280; led_state = 5; break;
		case 5: STATUS2_LED_ON;  led_countdown_timer_ms =    68; led_state = 6; break;
		case 6: STATUS2_LED_OFF; led_countdown_timer_ms =   950; led_state = 1; break;
	}
}


static void pattern_fast_red(void)  // this is never really used, the bootloader itself toggles the led
{
	STATUS2_LED_OFF; // make sure yellow is off
	switch (led_state)
	{
		case 1: STATUS_LED_ON;  led_countdown_timer_ms =   20; led_state = 2; break;  //  was 2 ms ON
		case 2: STATUS_LED_OFF; led_countdown_timer_ms =  150; led_state = 1; break;  //  was 2 ms OFF
	}
}
static void pattern_fast_yellow(void)
{
	STATUS_LED_OFF; // make sure red is off
	switch (led_state)
	{
		case 1: STATUS2_LED_ON;  led_countdown_timer_ms =   20; led_state = 2; break;  //  was 2 ms ON
		case 2: STATUS2_LED_OFF; led_countdown_timer_ms =  150; led_state = 1; break;  //  was 2 ms OFF
	}
}
static void pattern_slow_red(void) // LED_PATTERN_SLOW_RED
{
	STATUS2_LED_OFF; // make sure yellow is off
	switch (led_state)
	{
		case 1: STATUS_LED_ON;  led_countdown_timer_ms =  1000; led_state = 2; break;  //  1000 ms ON
		case 2: STATUS_LED_OFF; led_countdown_timer_ms =  1000; led_state = 1; break;  //  1000 ms OFF
	}
}
static void pattern_slow_yellow(void) // LED_PATTERN_SLOW_RED
{
	STATUS_LED_OFF; // make sure red is off
	switch (led_state)
	{
		case 1: STATUS2_LED_ON;  led_countdown_timer_ms =  1000; led_state = 2; break;  //  1000 ms ON
		case 2: STATUS2_LED_OFF; led_countdown_timer_ms =  1000; led_state = 1; break;  //  1000 ms OFF
	}
}





#if 0
static void pattern_red_on(void) // LED_PATTERN_SOLID
{
  STATUS_LED_ON;
}
#endif

#if 0
static void pattern_cell_call(void) // LED_PATTERN_CELL_CALL_IN_PROGRESS
{
	switch (led_state)
	{
		case 1: STATUS_LED_ON;  led_countdown_timer_ms =     5; led_state = 2; break;  // was 10
		case 2: STATUS_LED_OFF; led_countdown_timer_ms =   305; led_state = 3; break;  // was 300
		case 3: STATUS_LED_ON;  led_countdown_timer_ms =     5; led_state = 4; break;  // was 10
		case 4: STATUS_LED_OFF; led_countdown_timer_ms =   305; led_state = 5; break;  // was 300
		case 5: STATUS_LED_ON;  led_countdown_timer_ms =     5; led_state = 6; break;  // was 10
		case 6: STATUS_LED_OFF; led_countdown_timer_ms =  1205; led_state = 1; break;  // was 1200
	}
}
#endif

#if 0
void debug_led_on(void)
{
	HAL_GPIO_WritePin(GEN45_STATUS_LED_Port, GEN45_STATUS_LED_Pin, 1);
}
#endif


void led_heartbeat(void)
{
	if (led_countdown_timer_ms != 0) return;  // nothing to do yet

	switch (led_pattern)
	{
	    case LED_PATTERN_OFF:               return pattern_off(); break;
		default:
		case LED_PATTERN_SINGLE_RED:         return pattern_single_red(); break;
		case LED_PATTERN_DOUBLE_RED:         return pattern_double_red(); break;
		case LED_PATTERN_TRIPLE_RED:         return pattern_triple_red(); break;

		case LED_PATTERN_SINGLE_DOUBLE_RED:  return pattern_single_double_red(); break;

		case LED_PATTERN_SINGLE_YEL:         return pattern_single_yellow(); break;
		case LED_PATTERN_DOUBLE_YEL:         return pattern_double_yellow(); break;
		case LED_PATTERN_TRIPLE_YEL:         return pattern_triple_yellow(); break;

		case LED_PATTERN_SINGLE_DOUBLE_YEL:  return pattern_single_double_yellow(); break;


		case LED_PATTERN_FAST_RED:          return pattern_fast_red(); break;
		case LED_PATTERN_FAST_YEL:          return pattern_fast_yellow(); break;
		case LED_PATTERN_SLOW_RED:          return pattern_slow_red(); break;
		case LED_PATTERN_SLOW_YEL:          return pattern_slow_yellow(); break;
	}
}

void HAL_led_delay(uint32_t Delay_ms)
{
	hal_led_delay_timer_ms = Delay_ms + 1;  // to ensure at least the number of requested ms...
	while (hal_led_delay_timer_ms)
	{
		led_heartbeat();
	}
}


#if 0
// Blink with lower energy
void led_heartbeat()    // zg - unused
{
	// Intended to equal 1s.
	STATUS_LED_ON;
	HAL_Delay(5);
	STATUS_LED_OFF;
	HAL_Delay(995);
}
#endif

#if 0
// Blink Status LED
void led_action_heartbeat(int blinks)
{
	for (int k = 0; k < blinks; k++)
	{
		STATUS_LED_ON;
		HAL_Delay(10);
		STATUS_LED_OFF;
		HAL_Delay(190);
	}
}
#endif

// Blink Status LED
void led_status_report(void)
{
	for (int k = 0; k < 3; k++)
	{
		STATUS_LED_ON;
		HAL_Delay(100);
		STATUS_LED_OFF;
		HAL_Delay(100);
	}
}

// Blink Error LED
void led_error_report(void)
{
	for (int k = 0; k < 1; k++)
	{
		STATUS_LED_ON;
		HAL_Delay(50);
		STATUS_LED_OFF;
		HAL_Delay(50);
	}
}

#if 0

void led_fast_status(void)
{
	STATUS_LED_ON;
	HAL_Delay(10);
	STATUS_LED_OFF;
	HAL_Delay(10);
}

int RunSensorContinuous = 1;

void runSensorContinuous()
{
	while (RunSensorContinuous == 1)
	{
		led_status_report();
		get_sensor_measurement_0();
	}
}

void runSensor1000()
{
	led_status_report();
    get_sensor_measurement_0();
}
#endif
