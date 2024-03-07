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

#ifndef SOURCES_PERIPHERALS_LTC2943_H_
#define SOURCES_PERIPHERALS_LTC2943_H_

#include "stdint.h"


// zg - in the original code base, 4.0.16 it was 15000.0
// zg - in the beta code base, it was changed to 17000 with no reason noted.
#define Qlsb 0.425   // qLSB = 0.340mAh * (50mOhm/Rsense) * (M/4096)  in this case M is set to 1024 and Rsense is 10 mOhms

#define MAX_BATTERY 15000.0 	// milliAmp Hours, give 4000mA reserve
//#define MAX_BATTERY	17000   // milliAmp Hours, give 4000mA reserve

#define MAX_BATTERY_RAW 35294   // 15000 / Qlsb = 15000 / 0.425 = 35,294.11 to hex integer 0x89DE

// 7-18-2022 zg
// The design of the original iTracker, and the iTracker4, and the iTracker5
// was incorrect in the choice of using a 10 milli-ohm resistor across Sense+ and Sense-.
// That small of a resistor does not provide enough of a voltage drop for the LTC2943
// to work during sleep.  See the datasheet, page 15, which describes the
//    For input signals with an absolute value smaller than the
//    offset of the internal op amp, the LTC2943 stops integrating
//    and does not integrate its own offset.
// That is, there is a minimum voltage offset that must be provided by the resistor
// during sleep.
// Consequently, the LTC2943 DOES NOT MEASURE SLEEP CURRENT with a 10 milli-ohm resistor
// and a workaround must be provided in firmware.   This will be done by using the RTC to determine
// how many seconds the device was asleep and then estimate the enrgy used
// during sleep based on bench measurements for a typical device.
// email from Mark Watson dated 7-18-2022
//   Zack,
//
//   Assuming the higher sleep current of 450 uA, the factor will be
//   0.45mAH/3600sec/hour = 125e-6 mAh/Second  ( which comes to one mAh every 2.22 hours of sleep )
//
//   Roughly, I get 3.8 years of sleep time at the higher current based on a 15,000 mAh battery.
//
//   Mark

#define LTC2943_mAH_per_second_of_sleep   125E-06

void ltc2943_timeslice(void);

void  ltc2943_init_hw(void);  // do this to as close to power on as possible to set pre-scaler (M) correctly for sense resistor on the board
void  ltc2943_init_battery_accumulator(int From_MCU_Power_On);


int  ltc2943_get_battery_percentage(void);
void ltc2943_set_battery_percentage(int percentage);

float ltc2943_get_accumulator(void);
int   ltc2943_update_log_track(void);
int   ltc2943_set_battery_accumulator(float val);
void  ltc2943_update_percentage(void);


uint16_t ltc2943_get_raw_accumulator(void);
int      ltc2943_set_raw_accumulator(uint16_t data);

float    ltc2943_get_mAh(uint16_t raw_accumulator);


void  ltc2943_correct_for_sleep_current(uint16_t raw_accumulator_before_sleep, uint32_t seconds_asleep);

int16_t ltc2943_trigger_adc(void); // Datasheet says all three conversions are started: voltage (33 ms), current (4.5 ms), temperature (4.5 ms) and then ADC_Mode is set to 0


float ltc2943_get_voltage(int BlinkLed);



extern float ltc2943_last_voltage_reported;
extern float ltc2943_last_current_reported;
extern float ltc2943_last_temperature_reported;
extern uint16_t ltc2943_last_raw_acc_reported;
extern int ltc2943_last_percentage_reported;

void ltc2943_report_measurements(const char *label);
void ltc2943_trace_measurements(const char *label);  // this can take 64 ms to complete

int   ltc2943_are_batteries_present(void);

// RC Time Constants
// 1 Tau = 220ms for a 100 Ohm 2200 uF circuit (the main capacitor)
// 1 Tau = 63% = 220 ms
// 2 Tau = 86% = 440 ms
// 3 Tau = 95% = 660 ms
// 4 Tau = 98% = 880 ms

#define ONE_TAU   220   // 63%
#define TWO_TAU   440   // 86%
#define THREE_TAU 660   // 95%
#define FOUR_TAU  880   // 98%
void ltc2943_charge_main_capacitor(uint32_t delay_ms);  // takes N ms or up to N + 128 ms if voltages are reported

#define LTC2943_TIMEOUT_MS   2   // Note:  if an i2c error occurs, this timeout threshold is never reached

void ltc2943_Enforce_HARDSTOP_Voltage_Limit(int ReadVoltage);

#endif
