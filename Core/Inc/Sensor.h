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

#ifndef INC_SENSOR_H_
#define INC_SENSOR_H_

#define ZG_USE_NEW_TIME_ANCHOR

// The largest uint32_t value is:  4,294,967,296
// So, TIM2->CNT can span 4,294 seconds at 1 usec resolution.
// It can easily exceed the operational range of the underlying hardware.
// The original code allowed a max measurement time of about 8,080,824 ticks,
// which was thought to cut off good echos*.  Note that the old code was subject
// to jitter based on using interrupts from TIM5.  It appears the old code
// was intending a value of 100,000 TIM5 ticks = 8,000,000 TIM2 ticks = 0.1 sec.
//
// Using the distance formula, it was determined that at zero C, a CNT
// of 1,400,000 produces a distance of 9.41 feet.  As this exceeds the
// design target of 9 feet, it represents a good choice for the maximum
// transit time in ticks.
//
// *However, that still leaves the question of why a lot of ZERO pulse counts were
// occurring.  It may be that the holdup capacitors did not have sufficient
// time to recharge in-between back-to-back sensor readings.  The RC time
// constants were computed and a delay of ?? ms was chosen to give the
// electronics time to rest.

//
// New code will use 10,000,000 ticks = 125,000 usec = 0.125 sec

//#define MAX_TRANSIT_TIME_IN_TICKS   10000000UL  // This is 0.125 sec

//#define MAX_TRANSIT_TIME_IN_TICKS    1400000UL  // This is 9.41 feet at 0 C
#define MAX_TRANSIT_TIME_IN_TICKS    2800000UL  // This allows time to charge the capacitor a little bit (not fully)

#define MEASUREMENT_IN_PROGRESS()   (TIM2->CNT < MAX_TRANSIT_TIME_IN_TICKS)

//#define MAX_TRANSMIT_CYCLES_BEFORE_RECHARGE  2
extern int readings_taken_since_full_charge;

// Time of Flight variables
#define SPEED_SOUND			13354.111986 	// Speed of sound - inches/sec at 55degF (12.7778 C)
#define USEC_SPEED_SOUND	.013354 		// inches per usec
#define USEC_SPEED_Z		.0130499		// inches per microsec @ 0 celsius

// From PGA460 reference document titled:
// Application Report PGA460 Ultrasonic Module Hardware and Software Optimization
// page six
// v = speed of sound in air
// T = temperature in degrees C
// v = 331 m/s + 0.6 m/s/°C × T

float get_speed_of_sound_inches_per_us(float tempC);

#define MAX_PULSES 20
#define MAX_RAW_PULSES 100
extern uint32_t RawPulseResults[MAX_RAW_PULSES];
extern uint8_t  PulseCnt;
extern uint8_t  RawPulseCnt;
extern uint8_t  PulseMode;
extern int tim2_isr_total_pulse_count;
extern uint32_t tim2_min_CNT;
extern uint32_t tim2_max_CNT;



#define PRESCALE  80
#define ALLOWED_PAIRWISE_DIFFERENCE   16
#define ALLOWED_PAIRWISE_DIFFERENCE_RAW 1280    //  16 * 80 = 1280

// 120,000 corresponds to a distance 8.5 inches at zero C (9.08 inches at 38C)
// However, during testing at 99 inches, there were some "outbound" pulses
// that passed this threshold and hence messed up the true echo start point.
// For a true echo, it seems that there needs to be a relatively large "gap"
// between the "outbound" and the first "inbound".  When you view all the
// pulses at once, it is pretty obvious as a human observer. But, how to
// program a detection function?  Maybe there is a tradeoff between how
// small a distance you can measure vs how long a distance you can measure.

#define MINIMUM_ECHO_RAW_PULSE_VALUE   120000





#define MAX_READINGS_TO_AVE   14    // 14 = 5+9; based on old code: damping ranges from 3 to 9, max readings

#define SORT_ORDER_HI_TO_LO  0      // voltage[0] > voltage[1]
#define SORT_ORDER_LO_TO_HI  1      // voltage[0] < voltage[1]

typedef float (*binsearch_voltage_at_gain_func_t)(uint16_t);
typedef int (*binsearch_compare_func_t)(void *);
typedef int (*binsearch_getnext_func_t)(void *);
typedef struct {
	int sort_order;
	int lower_bound;
	int midpoint;
	int upper_bound;
	int low;
	int high;
	int flag;
	int index;


	binsearch_voltage_at_gain_func_t voltage;

	binsearch_compare_func_t compare;
	binsearch_getnext_func_t getnext;
	float tempC;
	int max_averages;
	int index_of_next_point;

	// these are for self-testing
	int self_test;
	int test_goal;

	// these are output parameters
	float distance[MAX_READINGS_TO_AVE];
	float sum;
	float ave_distance;
	int   gain[MAX_READINGS_TO_AVE];
	int   sum_gain;
	int   ave_gain;
} BINARY_SEARCH;






void sensor_init(int WhichGen);  // 4 = Gen4, 5 = Gen5

void sensor_power_en(uint8_t val);

void get_sensor_measurement_0(void);

#if 0
// zg - Hack to export raw data
#define RAW_PULSE_ARRAYS      15
#define RAW_PULSE_ARRAY_SIZE  20
extern uint32_t raw_SystemCoreClock[RAW_PULSE_ARRAYS];
extern uint32_t raw_PulseResults[RAW_PULSE_ARRAYS][RAW_PULSE_ARRAY_SIZE];
extern uint8_t  raw_array_index;
extern float    raw_temperature;
extern uint8_t  raw_entry_minNum;
extern uint8_t  raw_entry_maxNum;
extern uint16_t  raw_entry_Actual_Gain;
extern uint32_t raw_entry_Actual_Blanking;
extern uint16_t  raw_exit_Actual_Gain;
extern uint32_t raw_exit_Actual_Blanking;
extern float    raw_exit_distance;
#endif


#endif


void test_sensor_at_gain(uint16_t test_gain);
