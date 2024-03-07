

//#define USE_SENSOR_SHIP_CODE   // define to use shipping algorithm
//#define BETA_CODEBASE          // define for beta codebase (kludge for now)

#include "tim.h"
#include "math.h"
#include "Sensor.h"
#include "Gen4_max5418.h"
#include "Gen5_AD5270.h"
#include "debug_helper.h"
#include "trace.h"
#include "factory.h"
#include "version.h"
#include "uart1.h"
#include "lpuart1.h"




uint8_t  PulseCnt  = 0;
uint8_t	 PulseMode = 0;
uint8_t  RawPulseCnt  = 0;
uint32_t RawPulseResults [MAX_RAW_PULSES];
static float PulseCal[MAX_PULSES];
static int last_non_zero_entry;
static int start_of_echo_data;

float    LastGoodDegF = 0;
float    LastGoodMeasure = 0;
uint32_t Actual_Blanking = 900;
uint16_t Actual_Gain = 127;  // TODO 127 is for Gen4




uint8_t PowerEN = 0;
int readings_taken_since_full_charge = 0;

int which_gen = 4;







static uint16_t Gen4_GainBlanking[16] =
{
		          // Actual_Gain
		1500,     //   0 (never used)

		1400,     //   1 (never used)
		1400,     //   2 (never used)
		1400,     //   3 (never used)  gains below 4 were not allowed in original code
		1400,     //   4

		1200,     //   5     if set blanking to 1300 for this gain, can get out to at least 55 inches...need more testing
		1200,     //   6
		1200,     //   7

		1400,     //   8
		1400,     //   9
		1400,     //  10...15  (note that 10, and 11...15 result in the same blanking)

		1200,     //  16...31  (note, could not get a reading at blanking 1400.  Gain 16 may be the limit for this instrument using original firmware.)
		1200,     //  32...63

		1000,     //  64...127
		1000,     // 128...191
		1000      // 192...255
};  // Note that the natural groupings of blanking (6 ranges) do not line up with the other gain decision points used in the original code.





static uint16_t Gen4_sensor_get_blanking(uint8_t gain)  // returns a number from 1000 to 1500 depending on what Range unit is operating in.
{
	uint8_t sel = 0;

	if (gain > 191)
		sel = 15;        //array index = 15   ==> 1000
	else if (gain > 127)
		sel = 14;        //array index = 14   ==> 1000
	else if (gain > 63)
		sel = 13;        //array index = 13   ==> 1000
	else if (gain > 31)
		sel = 12;        //array index = 12   ==> 1200
	else if (gain > 15)
		sel = 11;        //array index = 11   ==> 1200
	else if (gain > 10)
		sel = 10;        //array index = 10   ==> 1400
	else
		sel = gain;        // here array index is 0..10   varies

	return Gen4_GainBlanking[sel];  // There are 16 array elements.
}

static uint16_t Gen5_sensor_get_blanking(uint16_t gain)  // returns a number from 1000 to 1500 depending on what Range unit is operating in.
{
	return (uint16_t)(1000 + 0.5*gain);
}










#if 0
static int report_gain(uint8_t gain)
{
	return (255 - (int)gain);
}
#endif

static void fully_charge_capacitor(void)
{
#if 1
	// Add a 1 second delay here to:
	// a) fully charge the capacitor
	// b) wait for the water surface to change.
    HAL_led_delay(1000);
#else
    // This was considered, but abandoned.  No need to use battery current to measure temp so often.
	if (isGen4())
	{
		get_tempF();  // as before on Gen4
	    HAL_led_delay(1000);
	}
	if (isGen5())
	{
		// For Gen5, C26 is 1000 uF and is charged through R29 (100 ohm) from SENSOR_PWR which is 12 VDC.
		// 300 ms should be enough time. But might as well take a temperature reading and leave at a total time of 1000 ms
		// as that is proven to work well in the field.
		get_tempF();  // Takes a minimum of 378 ms and a max of (378+22)=400 ms
		HAL_led_delay(622);
	}
#endif

	readings_taken_since_full_charge = 0;
}

void sensor_power_en(uint8_t val)
{

	HAL_GPIO_WritePin(GEN45_VH_PWR_EN_Port, GEN45_VH_PWR_EN_Pin, val);
	HAL_GPIO_WritePin(GEN45_OPAMP_SD_Port, GEN45_OPAMP_SD_Pin, val);

	//TODO Monitor the Power Good pin here !!!!

	// Delay when turning sensor ON from the OFF state.
	// This allows the capacitor time to charge.
	// Need at least 800 ms as verified on scope 7/30/2020
	// Use 1 s
	if ((PowerEN == OFF) && (val == ON))
	{
		if (isGen5())
		{
	      /* Switch VDD_3V3 to PWM mode to reduce noise during sensor measurements */
		  HAL_GPIO_WritePin(GEN5_VDD_3V3_SYNC_Port, GEN5_VDD_3V3_SYNC_Pin, GPIO_PIN_SET);   // Low noise mode (PWM)
		  HAL_Delay(5);    // Wait for 3V3 to stabilize
		}

		fully_charge_capacitor();
	}
	else if ((PowerEN == ON) && (val == OFF))
	{
		// When turning power OFF, change the 3V3 supply back to low-power mode
		if (isGen5())
		{
		    /* Switch VDD_3V3 to low power mode */
			HAL_GPIO_WritePin(GEN5_VDD_3V3_SYNC_Port, GEN5_VDD_3V3_SYNC_Pin, GPIO_PIN_RESET);   // Low-power mode
		}
	}

	PowerEN = val;  // Remember current state
}




float get_speed_of_sound_inches_per_us(float tempC)
{
	float speed;
	speed = USEC_SPEED_Z * sqrt(1 + (tempC / 273));  // inches per us
	return speed;
}

static float calc_distance(float tempC, float AvePulseCal)
{
	float distance;
	float speed;

	speed = get_speed_of_sound_inches_per_us(tempC);  // inches per microsecond

	distance = (((AvePulseCal - 175) * speed) / 2) - 0.15; // doesn't work well at 10 inches

	//distance = (((AvePulseCal - 175) * speed) / 2);   // worked ok at 10 inches. A little high at 54 inches...

	//distance = ((AvePulseCal * speed) / 2);  // zg - didn't work at 10 inches

    return distance;
}

#if 0
static float calc_distance_2(float tempC, float AvePulseCal)
{
	float distance;
	float speed;
	speed = USEC_SPEED_Z * sqrt(1 + (tempC / 273));

	distance = (((AvePulseCal - 175) * speed) / 2) - 0.15; // doesn't work well at 10 inches

	//distance = (((AvePulseCal - 175) * speed) / 2);   // worked ok at 10 inches. A little high at 54 inches...

	//distance = ((AvePulseCal * speed) / 2);  // zg - didn't work at 10 inches

    return distance;
}
#endif

static int all_zeros(int start_j)
{
    int j;
	for (j = start_j; j < MAX_PULSES; j++)
	{
	   if (RawPulseResults[j + start_of_echo_data]) return 0; // non-zero entry found, fail this function
	}
	return 1; // all remaining entries are zero
}

#if 0
static int all_zeros_raw(int start_j)
{
    int j;
	for (j = start_j; j < MAX_RAW_PULSES; j++)
	{
	   if (RawPulseResults[j]) return 0; // non-zero entry found, fail this function
	}
	return 1; // all remaining entries are zero
}
#endif

static void print_pulse_data_prefix(int RcvPulseCnt, int actualPulseCnt, int divCnt, float distance, float distance_2)
{
	char msg[256];
	char distance_str[20];

	static int just_once = 1;
	if (just_once)
	{
	    // Print header
		// FW Ver, MAC, Serial Number
		sprintf(msg, "\nFW Ver, %d.%d.%d, MAC, %s, Serial Number, %s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD, MainConfig.MAC, MainConfig.serial_number);
		TraceSensor_AppendMsg(msg);

		sprintf(msg, "\nActual_Gain, TotalPulseCnt, RcvPulseCnt, ActPulseCnt,  Criteria, DivCnt, Distance, Distance_2, RawPulseResults, ");
		TraceSensor_AppendMsg(msg);
		just_once = 0;
	}
    // Print prefix
	sprintf(msg, "\n%03d, %03d, %02d, %02d", Actual_Gain, tim2_isr_total_pulse_count, RcvPulseCnt, actualPulseCnt);
	TraceSensor_AppendMsg(msg);

	if (actualPulseCnt == 0)
	{
		TraceSensor_AppendMsg(", ZERO     , ");  // pulse is too narrow, reduce gain
	}
	else if (PulseCal[0] <= (Actual_Blanking + 40))
	{
		TraceSensor_AppendMsg(", TOO SMALL, ");
	}
	else if (actualPulseCnt < 6)
	{
		TraceSensor_AppendMsg(", TOO FEW  , ");  // pulse is too narrow, reduce gain
	}
	else if ((actualPulseCnt > 16) || ((actualPulseCnt == 16) && (RcvPulseCnt > 16)))
	{
		trace_AppendMsg(", TOO MANY , ");  // pulse is too wide, increase gain
	}
	else
	{
		TraceSensor_AppendMsg(", OK       , ");
	}

	// Now append divCnt and distance

	f_to_str_nn_nn(distance_str, distance);

	sprintf(msg, "%02d,  %s  ,", divCnt, distance_str);  // indicate which array value(s) were used to report distance

	TraceSensor_AppendMsg(msg);

	// Now append Distance_2 - an alternative to using the variable pulse width...

	f_to_str_nn_nn(distance_str, distance_2);

	sprintf(msg, "  %s  ,", distance_str);  // use the 5th array element

	TraceSensor_AppendMsg(msg);

}

// Print the pulse data

static void print_pulse_data(int rcvPulseCnt, int actualPulseCnt, int divCnt, float distance, float distance_2)
{
	char msg[256];

	if (!sd_TraceSensor) return;


	print_pulse_data_prefix(rcvPulseCnt, actualPulseCnt, divCnt, distance, distance_2);

#if 1 //  print the data behind the PulseCal array
	for (int j=0; j < PulseCnt; j++)
	{

		if (all_zeros(j)) break;  // there are no more data points after this, go to next row

		//if ((j == start_of_echo_data) && (start_of_echo_data)) trace_AppendMsg("  echo_start  , ");


		//char pulse_cal_str[20];
		//f_to_str_2dec(pulse_cal_str, PulseCal[j]);
		//f_to_str_nn_nn(pulse_cal_str, calc_distance(tempC, PulseCal[j]);
		//sprintf(msg, "%s,  ", pulse_cal_str);
		sprintf(msg, "%06lu, ", RawPulseResults[j + start_of_echo_data]);

		TraceSensor_AppendMsg(msg);

	}
#endif
#if 0
	TraceSensor_AppendMsg("  raw data  ,");

	// print all of the raw data
	for (int j=0; j < RawPulseCnt; j++)
	{

		if (all_zeros_raw(j)) break;  // there are no more data points after this, go to next row

		if ((j == start_of_echo_data) && (start_of_echo_data)) trace_AppendMsg("  echo_start  , ");

		sprintf(msg, "%06lu, ", RawPulseResults[j]);

		TraceSensor_AppendMsg(msg);
	}
#endif
}

static float get_distance_2(float tempC, uint8_t actualPulseCnt)
{
#if 0
#if 1
#define DISTANCE_2_MIN_PULSE_CNT    1
#define DISTANCE_2_CAL_PULSE_INDEX  0
#else
#define DISTANCE_2_MIN_PULSE_CNT    5
#define DISTANCE_2_CAL_PULSE_INDEX  4
#endif


	if (PulseCnt >= DISTANCE_2_MIN_PULSE_CNT)
		return calc_distance_2(tempC, PulseCal[DISTANCE_2_CAL_PULSE_INDEX]);
	else
		return 0.0;
#endif

#if 0
	// Report the average of all of the good echos.
	// This does not work that well.
	if (PulseCnt)
	{
		int i;
		float sum = 0.0;
		float ave;
		for (i=0; i < PulseCnt; i++)
		{
			sum += PulseCal[i];
		}
		ave = sum / PulseCnt;
		return calc_distance_2(tempC, ave);

	}
	else
		return 0.0;
#endif


#if 1
	uint8_t divCnt;
	float pulse_cal;


	// Select the mid point of the validated array to use as the reported data point.
	// Note:  all other points are ignored

	if (actualPulseCnt % 2 != 0)
	{
		divCnt = (actualPulseCnt / 2) + 1;  // plus one for this case

		pulse_cal = PulseCal[divCnt];

	}
	else
	{
		divCnt = actualPulseCnt / 2;

		pulse_cal = (PulseCal[divCnt] + PulseCal[divCnt+1]) / 2;
	}

	return calc_distance(tempC, pulse_cal);
#endif
}

static int find_last_non_zero_entry(void)
{
	int i;

	for (i = MAX_RAW_PULSES-1; i >= 0; i--)
	{
		if (RawPulseResults[i] != 0) return i;
	}
	return 0; // all entries are zero
}



static int find_start_of_echo_data(int last_non_zero_entry)
{
	int i;

#if 0 // This works well for data with only one echo.  For data with multiple echos it finds the last echo, not the first...

	// Search backwards by pairs.  Each pair must be within ALLOWED_PAIRWISE_DIFFERENCE
	for (i = last_non_zero_entry; i > 0; i--)
	{
	  uint32_t time_diff;
	  time_diff = RawPulseResults[i] - RawPulseResults[i-1];
	  if (time_diff > ALLOWED_PAIRWISE_DIFFERENCE_RAW) return i;
	}
#endif

	// Search forward until an entry exceeds MINIMUM_ECHO_RAW_PULSE_VALUE
	for (i = 0; i < last_non_zero_entry + 1; i++)
	{
		if (RawPulseResults[i] > MINIMUM_ECHO_RAW_PULSE_VALUE) return i;
	}

	return 0;  //  No data appears to be valid echo data
}

static void analyze_echo_data(void)
{


	last_non_zero_entry = find_last_non_zero_entry();
	start_of_echo_data = find_start_of_echo_data(last_non_zero_entry);

	// Calculate the number of valid echo pulses found
	//if ((last_non_zero_entry == 0) || (start_of_echo_data == 0))  // Gen4 code always had RawPulseResults[0] as ringdown data, Gen5 does not.  It has a blanking interval in the ISR
    if (last_non_zero_entry == 0)
		PulseCnt = 0;
	else
        PulseCnt = last_non_zero_entry - start_of_echo_data + 1;

	if (PulseCnt > (MAX_PULSES - 1)) PulseCnt = MAX_PULSES - 1;
}



static uint8_t sensor_single(float tempC)
{
	float prescale = SystemCoreClock / 1000000;  // zg - this is the value 80.00

	//clear counters
	tim2_isr_total_pulse_count = 0;
	RawPulseCnt = 0;
	PulseCnt = 0;
	PulseMode = 1;

	// Zero out contents of PulseCal to avoid spurious uses of PulseCal[0] when subsequent readings return PulseCnt of zero
	for (int j = 0; j < MAX_PULSES; j++)
	{
		PulseCal[j] = 0;
	}
	for (int j = 0; j < MAX_RAW_PULSES; j++)
	{
		RawPulseResults[j] = 0;
	}

StressTest:

    // From lab testing on 7/30/2020:
    // Need about 500 ms in-between transmit bursts
    // to allow the capacitors time to charge up to 9.5v.
    // Would prefer to get to 12V but don't want to
    // wait that long.
    // The loop overhead provides about:
    //   17.4 ms at max ticks of 1,400,000  (add 485 ms)
    //   34.8 ms at max ticks of 2,800,000  (add 465 ms)
    // 8-3-2020 verified 200 ms between readings keeps voltage level from dropping over repeated cycles during stress test.
    // However, due to pressing needs to release, it was deemed too risky.

    //HAL_led_delay(185);
    //HAL_led_delay(485);

	//Init timers TIM1-pulse out, TIM2-Pulse in
	MX_TIM2_Init();  //TODO - must add a run-time switch between Gen4 and Gen5
	MX_TIM1_Init();  //TODO - must add a run-time switch between Gen4 and Gen5


	TIM1->CNT = 0;  //
	TIM2->CNT = 0;  // zg - this is required to start counting from zero.

	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);

#if 1 //TODO Gen5 vs Gen4  zg - include this line if TIM2 is a slaves to TIM1
	TIM1->CR1 |= TIM_CR1_CEN;
#endif

	while (MEASUREMENT_IN_PROGRESS())
	{

	}

	HAL_TIM_Base_Stop(&htim1);  // TIM1 automatically stops itself and sets both outputs low, this should not be needed
	HAL_TIM_Base_Stop_IT(&htim2);

	if (sd_StressTest) goto StressTest;

#if 0
	if (readings_taken_since_full_charge == MAX_TRANSMIT_CYCLES_BEFORE_RECHARGE)
	{
	  HAL_led_delay(185);
	  recharge_countdown = 0;
	}
#endif

	analyze_echo_data();


	for (int j = 0; j < PulseCnt; j++)
	{
#ifdef ZG_USE_NEW_TIME_ANCHOR
		PulseCal[j] = (RawPulseResults[j + start_of_echo_data] / prescale);  // This is a number greater than or equal to 900 (power on) or 1000 (after 1st data point)
#else
		PulseCal[j] = (RawPulseResults[j] / prescale) + Actual_Blanking;  // This is a number greater than or equal to 900 (power on) or 1000 (after 1st data point)
#endif
	}

	if (sd_TraceSensor)
	{
		char msg[100];
		char prescale_str[25];
		static int just_once = 1;
		if (just_once)
		{
		  f_to_str_4dec(prescale_str, prescale);
		  sprintf(msg,"\nprescale = %s\n", prescale_str);
		  TraceSensor_AppendMsg(msg);
		  just_once = 0;
		}

	}


	return PulseCnt;
}





static void set_sensor_gain(uint16_t new_gain)
{

	if (which_gen == 4)
	{
		/*
		 * Gen4 Actual_Gain was an index into a table of decreasing voltage values.
		 * This number was saved in the log file as-is and later reported to the user as 255 - gain.
		 */
	  Actual_Gain =  new_gain;
	  Actual_Blanking = Gen4_sensor_get_blanking(Actual_Gain); // returns a number from 1000 to 1500
	  Gen4_max5418_setR(Actual_Gain);
	}
	else
	{
		/* TODO - reconcile gain usage and reporting between Gen4 and Gen5.
		 * Gen4 Actual_Gain was an index into a table of decreasing voltage values.
		 * This number was saved in the log file as-is and later reported to the user as 255 - gain.
		 * Since Gen5 gain is from 0 to 1023, in increasing voltage values, this causes an issue with
		 * the binary search algorithm.  Consequently, the binary search is performed on a table
		 * of decreasing voltage values...
		 */
      uint16_t wiper_pos;
	  wiper_pos = new_gain;  // For Gen5, the wiper position is the "gain"
	  Actual_Gain = new_gain;
	  Actual_Blanking = Gen5_sensor_get_blanking(Actual_Gain); // returns a number from 1000 to 1500
	  Gen5_AD5270_Set(AD5270_WRITE_WIPER, wiper_pos);
	}

	//HAL_led_delay(5);  // Original was 5 + 20
	//HAL_led_delay(25);  // Test
	//HAL_led_delay(50);  // Test
	//HAL_led_delay(100);  // Test
	//HAL_led_delay(200);  // Test
}



















// This is the new, binary search method



// sensor_measure_2:

#define BIN_SEARCH_IN_RANGE    1
#define BIN_SEARCH_TOO_LOW     2
#define BIN_SEARCH_TOO_HIGH    3



//#define TEST_BINARY_SEARCH


static void init_binary_search_2(BINARY_SEARCH * p, int start, binsearch_compare_func_t compare, binsearch_getnext_func_t getnext, float tempC, int max_averages)
{

	p->low  = p->lower_bound;
	p->high = p->upper_bound;
	p->flag = 0;  // not found

	p->compare = compare;
	p->getnext = getnext;

	// Cannot start at a boundary
	if (start == p->low)  start = p->low + 1;
	if (start == p->high) start = p->high - 1;

	if ((p->low < start) && (start < p->high))
		p->index = start;
	else
	    p->index = p->getnext(p); //  p->low + (p->high - p->low)/2;  // this expression handles the integer truncation correctly



	p->tempC = tempC;
	if (max_averages < 1) max_averages = 1;
	if (max_averages > MAX_READINGS_TO_AVE) max_averages = MAX_READINGS_TO_AVE;
	p->max_averages = max_averages;
	p->index_of_next_point = 0;

	// these are for self-testing
	p->self_test = 0;
	p->test_goal = 0;

	// Initialize output array to known values
    for (int i = 0; i < MAX_READINGS_TO_AVE; i++)
    {
	  p->distance[i] = 0.0;  // initialize to a known value
	  p->gain[i] = 0;  // initialize to a known value
    }
    p->sum = 0.0;
    p->ave_distance = 0.0;

    p->sum_gain = 0;
    p->ave_gain = 0;
}

static int binary_search_2(BINARY_SEARCH *p)
{
	while (p->low <= p->high)
	{
		int compare_result;

		if (p->self_test)
		{
			if (p->index == p->test_goal)
				compare_result = BIN_SEARCH_IN_RANGE;
			else if (p->index < p->test_goal)
				compare_result = BIN_SEARCH_TOO_LOW;
			else
				compare_result = BIN_SEARCH_TOO_HIGH;
		}
		else
		{
		  compare_result = p->compare(p);
		}

		switch (compare_result)
		{

		case BIN_SEARCH_IN_RANGE:
			p->flag = 1;
			return BIN_SEARCH_IN_RANGE;

		case BIN_SEARCH_TOO_LOW:
			p->low = p->index + 1;
			break;

		default:
			p->high = p->index - 1;
			break;
		}

		// compute the next index to try
#if 0
		p->index = p->low + (p->high - p->low)/2; // this expression handles the integer truncation correctly
#else
		p->index = p->getnext(p);
#endif

	}

    if (p->index == p->lower_bound) return BIN_SEARCH_TOO_LOW;

    return BIN_SEARCH_TOO_HIGH;
}

static uint8_t get_distance(uint8_t rcvPulseCnt, float tempC, float * distance);


extern void get_status_str(uint8_t status, char *status_str);

#ifdef TEST_SENSOR_MAIN_LOOP   //TestSensorMainLoop
#include "lpuart1.h"
#include "uart4.h"
void test_sensor_at_gain(uint16_t test_gain)
{
	char msg[80];
	char status_str[25];

	uint8_t rcvPulseCnt;
	uint8_t status;

	float curTemp;
	float tempC;
	float distance;
	int  dec9, dec10 = 0;

	get_tempF(&curTemp);
	tempC = (curTemp - 32.0) * 5.0 / 9.0;
//tempC = 25;
	set_sensor_gain(test_gain);

	rcvPulseCnt = sensor_single(tempC);

	// get the distance and its status
	distance = 0.0;
	status = get_distance(rcvPulseCnt, tempC, &distance);

	get_status_str(status, status_str);

	// Distance
	convfloatstr_2dec(distance, &dec9, &dec10);

	sprintf(msg,"\n%d.%02d,  %4d,  %s", dec9, dec10, test_gain, status_str);

	TESTSENSOR_UART_tx_string(msg);

}
#endif

// Returns IN_RANGE, BIN_SEARCH_TOO_LOW, or BIN_SEARCH_TOO_HIGH

static int perform_measurement_at_gain(void * pv)
{
	uint8_t rcvPulseCnt;
	BINARY_SEARCH *p;
	int i;
	uint8_t status;
	int retry_allowed = 1;


	p = (BINARY_SEARCH *)pv;

	set_sensor_gain(p->index);

	for (i = 0; i < p->max_averages; i++)
	{
		retry:

		// Apply a consistent delay for cap to charge.  This is to help with farther distances.
		// 20 or 25 ms was used by the old firmware, depending on how you count up the delays.
		// This does NOT fully charge the cap.
		// To maintain an 8.6 V level, 200 ms was required.
		// However, adding large amounts of time creates it's own problems with gain, changing water surface, etc.
		HAL_led_delay(25);

#if 0
		// Kludge - force long delay (500 ms) and hold gain fixed - with a fully charged cap, what happens? Not much. Hmmm.  Would have expected more echos.
		HAL_led_delay(475);
		// Kludge
#endif

		// Perform a reading
		rcvPulseCnt = sensor_single(p->tempC);

		// Keep track of how many readings since the capacitor was fully charged up
		// Not presently using this...
		readings_taken_since_full_charge++;

		// get the distance and its status, save into the array of good points
		status = get_distance(rcvPulseCnt, p->tempC, &(p->distance[p->index_of_next_point]));

		// Check for invalid data.  Allow for retires if a failure occurs.
		// This is intended to allow for turbulent conditions where the angle may temporarily prevent any echos.
		// In lab testing, results showed the angle can switch in-between measurements taken at full speed:
		// One measurement works, the next fails, the next works (at the same gain).
		// That was without the 500 ms delay required for charging the capacitors, so presumably it can
		// occur at the 500 ms timescale as well.  HOWEVER, the pulses were weakening each time, so this
		// retry mechanism may not be needed anymore.  In fact, it may have an adverse effect of
		// slowing down the gain tracking.  Need to verify with more traces in the lab.

		if (status != BIN_SEARCH_IN_RANGE)
		{
			if (retry_allowed)
			{
				retry_allowed = 0;  // don't allow any more retries
				goto retry;
			}

			// Check for zero echo pulses?  It is hard to determine what to do next when there is no information.

			break;   //  two failures in a row will exit the gain
		}

		// Save the gain
		p->gain[p->index_of_next_point] = p->index;
		p->sum_gain += p->gain[p->index_of_next_point];

		// sum all of the individual distances together, possibly at different gains
		p->sum += p->distance[p->index_of_next_point];

		// advance to the next array position
		p->index_of_next_point++;

		if (p->index_of_next_point == p->max_averages) break;  // we have gathered enough valid readings, possibly at different gains, to average

		// Allow cap to charge and perform spatial averaging on the flow of water, delay before taking the next point to average.
		//HAL_led_delay(20);
		retry_allowed = 1;   // reset the retry count since the water surface is new.

		//if (sd_TraceSensor)
		//{
		//	TraceSensor_AppendMsg("\n");  // indicate where the spatial averages are
		//}
	}

	// Compute the average distance and gain of the good readings taken so far, any additional points will result in a new average
	if (p->index_of_next_point)
	{
		p->ave_distance = p->sum / p->index_of_next_point;
		p->ave_gain = p->sum_gain / p->index_of_next_point;  // this truncates, which is probably ok
	}

	return status;
}

static int find_closest_gain(void * pv, float value)
{
	int i;
	BINARY_SEARCH *p;
	p = (BINARY_SEARCH *)pv;

	// Do a pairwise search
	for (i = p->lower_bound; i < p->upper_bound; i++)
	{
		float volt_i;    // i
		float volt_i1;   // i+1

		volt_i  = p->voltage(i);
		volt_i1 = p->voltage(i+1);

		if (value == volt_i)   return i;
		if (value == volt_i1)  return i+1;

		if (p->sort_order == SORT_ORDER_LO_TO_HI)  // Gen5
		{
			// Gen4
			if ((volt_i < value) && (value < volt_i1))   // this assumes v[0] < v[1]
			{
				// always snap to the higher voltage (higher index)
				return i+1;
			}
		}
		else
		{
			// Gen4
			if ((volt_i1 < value) && (value < volt_i))   // this assumes v[0] > v[1]
			{
				// always snap to the higher voltage (lower index)

				return i;
			}
		}
	}
	// we should never get here, but...
	// if value is out of bounds, report which boundary
	if (p->sort_order == SORT_ORDER_LO_TO_HI)  // Gen5
	{
		if (value > p->voltage(p->upper_bound))  return p->upper_bound;
		if (value < p->voltage(p->lower_bound))  return p->lower_bound;
	}
	else
	{
		//Gen4
		if (value < p->voltage(p->upper_bound))  return p->upper_bound;
		if (value > p->voltage(p->lower_bound))  return p->lower_bound;
	}
	// we should never get here, but if so, return the midpoint.
	return p->midpoint;
}

int getnext_gain(void * pv)
{
	BINARY_SEARCH *p;
	p = (BINARY_SEARCH *)pv;

	// Use a binary search on the voltages

	float next_value;
	// Calculate the midpoint between the hi and lo boundaries

	if (p->sort_order == SORT_ORDER_LO_TO_HI)  // Gen5
		next_value = p->voltage(p->low) + (p->voltage(p->high) - p->voltage(p->low))/2;   // note v[0] < v[1]
	else
		next_value = p->voltage(p->high) + (p->voltage(p->low) - p->voltage(p->high))/2;  // note v[0] > v[1]


	return find_closest_gain(pv, next_value);  // this function also must snap to the lowest index for binary search to work correctly.

}

#if 0  // used to find the midpoint by voltage
int find_midpoint(void)
{
	float next_value;


	next_value = p->voltage(GAIN_MAX) + (p->voltage(GAIN_MIN) - p->voltage(GAIN_MAX))/2;  // note v[0] > v[1]

	return find_closest_gain(next_value);
}
#endif


int number_of_points_averaged(BINARY_SEARCH * p)
{
	return p->index_of_next_point;
}

void get_status_str(uint8_t status, char *status_str)
{
	strcpy(status_str, "unknown status");
	if (status == BIN_SEARCH_IN_RANGE) strcpy(status_str,"IN_RANGE");
	if (status == BIN_SEARCH_TOO_LOW)  strcpy(status_str,"TOO_LOW");
	if (status == BIN_SEARCH_TOO_HIGH) strcpy(status_str,"TOO_HIGH");
}


static uint8_t get_distance(uint8_t rcvPulseCnt, float tempC, float * distance)
{
	uint8_t divCnt;
	float pulse_cal;

	uint8_t actualPulseCnt;


#ifdef TEST_BINARY_SEARCH




	if (last_range_index == 2)
	{

		if (Actual_Gain > 82) return BIN_SEARCH_TOO_HIGH;
		if (Actual_Gain < 81) return BIN_SEARCH_TOO_LOW;

	}

	//return GAIN_TOO_HIGH;
	return BIN_SEARCH_TOO_LOW;


#endif

	actualPulseCnt = rcvPulseCnt;  // assume all array points are good

	if (rcvPulseCnt)
	{
		// Perform pair-wise comparison of raw pulses
		for (int j = 0; j < rcvPulseCnt; j++)
		{
			float pulse_cal_diff;

			// Note:  for real data, PulseCal[j+1] is always greater than PulseCal[j], so this is always a positive number.
			// Note also that "running off the end of data" will compare a zero to non-zero and hence be negative.
			// A negative number will not trigger an early exit, thus actualPulseCnt will be correct.
			// The original code did not zero out PulseCal[] before each measurement, so this might behave differently.
			pulse_cal_diff = PulseCal[j+1] - PulseCal[j];

			// stop at the first pair that are too far apart.  A time difference of 16 usec is equal to 0.10 inches.
			if (pulse_cal_diff > ALLOWED_PAIRWISE_DIFFERENCE)   // This is a one sided comparison because the data is ordered, e.g. data at (j+1) is always greater than data at (j)
			{
				actualPulseCnt = j+1;
				break;
			}
		}
	}



	if (actualPulseCnt == 0)
	{
	    print_pulse_data(rcvPulseCnt, actualPulseCnt, 0, 0.0, 0.0);
		//zg return  BIN_SEARCH_TOO_HIGH;  // When there is no information, it is hard to tell which way to change gain.
		return  BIN_SEARCH_TOO_LOW;  // When there is no information, it is hard to tell which way to change gain.
	}



	// Check if measured data contains a valid array.
	// The old firmware used this criteria to reset gain to 239, and may have caused oscillation.
	// It is unclear if this criteria is actually useful, or what physical meaning it has.
	// It appears that this "fixes" a low-level timing issue with the ISR's wherein a pulse
	// can be "counted" sooner than expected.
	if (PulseCal[0] <= (Actual_Blanking + 40))
	{
		print_pulse_data(rcvPulseCnt, actualPulseCnt, 0, 0.0, get_distance_2(tempC, actualPulseCnt));
		//zg return BIN_SEARCH_TOO_LOW;  // increase wiper position
		return BIN_SEARCH_TOO_HIGH;  //
	}

	//if ((actualPulseCnt < 4) || (rcvPulseCnt < 5))  // allows rcv >= 5 and act >= 4 as good

	if (actualPulseCnt < 6)
	{
		print_pulse_data(rcvPulseCnt, actualPulseCnt, 0, 0.0, get_distance_2(tempC, actualPulseCnt));
		//zg return  BIN_SEARCH_TOO_HIGH;  // pulse is too narrow, reduce wiper position
		return  BIN_SEARCH_TOO_LOW;  //
	}

	// Special case:  if both actual and rcv are 16, it is good data
	if ((actualPulseCnt > 16) || ((actualPulseCnt == 16) && (rcvPulseCnt > 16)))
	{
		print_pulse_data(rcvPulseCnt, actualPulseCnt, 0, 0.0, get_distance_2(tempC, actualPulseCnt));
		//zg return BIN_SEARCH_TOO_LOW;   // pulse is too wide, increase wiper position
		return BIN_SEARCH_TOO_HIGH; //
	}

	// ASSERT:  pulse has from 6 to 16 peaks


	// Select the mid point of the validated array to use as the reported data point.
	// Note:  all other points are ignored

	if (actualPulseCnt % 2 != 0)
	{
		divCnt = (actualPulseCnt / 2) + 1;  // plus one for this case

		pulse_cal = PulseCal[divCnt];

	}
	else
	{
		divCnt = actualPulseCnt / 2;

		pulse_cal = (PulseCal[divCnt] + PulseCal[divCnt+1]) / 2;
	}

	*distance = calc_distance(tempC, pulse_cal);

    print_pulse_data(rcvPulseCnt, actualPulseCnt, divCnt, *distance, get_distance_2(tempC, actualPulseCnt));

	return BIN_SEARCH_IN_RANGE;  // valid distance
}








#if 0
static float get_sensor_measurement_1(void)
{
	static float fLastMeasure = 0; // this definitely needs to be static
	float this_try;
	int8_t Snr_Stat;
	float tmpDelta;
	float criteria_lo;
	float criteria_hi;
	uint8_t minNum;
	uint8_t maxNum;


#define MAX_DELTA_1MIN  0.3

	tmpDelta = MAX_DELTA_1MIN * MainConfig.log_interval / 60;



	this_try = 0;
	minNum = 5 + MainConfig.damping;
	maxNum = 6 + MainConfig.damping;
	if (sd_TraceSensor)
	{
		char msg[150];
		sprintf(msg,"\n\n========== TRY 1 minNum = %d, maxNum = %d==========\n", minNum, maxNum);
		TraceSensor_AppendMsg(msg);
	}
	Snr_Stat = sensor_measure(&this_try, minNum, maxNum);


	if (fLastMeasure == 0)
		fLastMeasure = this_try;

	criteria_lo = fLastMeasure - (tmpDelta*2);
	criteria_hi = fLastMeasure + (tmpDelta*2);

#if 1
	if (sd_TraceSensor)
	{
		char msg[100];
		char this_try_str[30];
		char last_measure_str[30];
		char criteria_lo_str[34];
		char criteria_hi_str[34];

		f_to_str_2dec(this_try_str, this_try);
		f_to_str_2dec(last_measure_str, fLastMeasure);

		criteria_lo_str[0]='\0';
		criteria_hi_str[0]='\0';
		//f_to_str_2dec(criteria_lo_str, criteria_lo);
		//f_to_str_2dec(criteria_hi_str, criteria_hi);
		if (this_try < criteria_lo)  strcat(criteria_lo_str, " TOO LOW");
		if (this_try > criteria_hi)  strcat(criteria_hi_str, " TOO HI");


		sprintf(msg,"stat= %d, last=%s,  this=%s, %s, %s\n", Snr_Stat, last_measure_str, this_try_str, criteria_lo_str, criteria_hi_str);
		TraceSensor_AppendMsg(msg);
	}
#endif

	if (Snr_Stat || !this_try || (this_try < criteria_lo) || (this_try > criteria_hi))
	{
		float previous_try;

		if (Snr_Stat)
		{
			set_sensor_gain(LastGoodGain);
		}
		previous_try = this_try;
		minNum = 6 + MainConfig.damping;
		maxNum = 8 + MainConfig.damping;

		if (sd_TraceSensor)
		{
			char msg[150];
			sprintf(msg,"\n\n========== TRY 2 minNum = %d, maxNum = %d==========\n", minNum, maxNum);
			TraceSensor_AppendMsg(msg);
		}

		HAL_led_delay(100);

		this_try = 0;
		Snr_Stat = sensor_measure(&this_try, minNum, maxNum);

		criteria_lo = previous_try - MAX_DELTA_1MIN;   // <===== WHY LOOK AT PREVIOUS TRY ? SHOULD LOOK AT LAST, IF ANYTHING.
		criteria_hi = previous_try + MAX_DELTA_1MIN;

#if 1
		if (sd_TraceSensor)
		{
			char msg[100];
			char this_try_str[30];
			char last_measure_str[30];
			char criteria_lo_str[34];
			char criteria_hi_str[34];

			f_to_str_2dec(this_try_str, this_try);
			f_to_str_2dec(last_measure_str, previous_try);

			f_to_str_2dec(criteria_lo_str, criteria_lo);
			f_to_str_2dec(criteria_hi_str, criteria_hi);
			if (this_try < criteria_lo)  strcat(criteria_lo_str, " TOO LOW");
			if (this_try > criteria_hi)  strcat(criteria_hi_str, " TOO HI");

			sprintf(msg,"stat= %d, this=%s,  last=%s,  lo=%s,  hi=%s\n", Snr_Stat, this_try_str, last_measure_str, criteria_lo_str, criteria_hi_str);
			TraceSensor_AppendMsg(msg);
		}
#endif

		if (Snr_Stat || !this_try || (this_try < criteria_lo) || (this_try > criteria_hi))
		{



			if (Snr_Stat)
			{
				set_sensor_gain(126);
			}

			previous_try = this_try;
			minNum =  7 + MainConfig.damping;
			maxNum = 10 + MainConfig.damping;

			if (sd_TraceSensor)
			{
				char msg[150];
				sprintf(msg,"\n\n========== TRY 3 minNum = %d, maxNum = %d==========\n", minNum, maxNum);
				TraceSensor_AppendMsg(msg);
			}

			HAL_led_delay(100);

			this_try = 0;
			Snr_Stat = sensor_measure(&this_try, minNum, maxNum);

#if 1
			if (sd_TraceSensor)
			{
				char msg[100];
				char this_try_str[30];
				char last_measure_str[30];


				f_to_str_2dec(this_try_str, this_try);
				f_to_str_2dec(last_measure_str, previous_try);


				sprintf(msg,"stat= %d, this=%s,  last=%s\n", Snr_Stat, this_try_str, last_measure_str);
				TraceSensor_AppendMsg(msg);
			}
#endif

			// At this point, regardless of Snr_Stat the value of this_try is used, which can be zero
		}
		else
		{
			// ASSERT:  Snr_Stat is zero and this_try is non-zero
		}

	}
	else
	{
		// ASSERT:  Snr_Stat is zero and this_try is non-zero
	}



	fLastMeasure = this_try;  // note that zero can be returned when the 3rd try fails

	if (sd_TraceSensor) trace_SaveBuffer();

	return this_try;
}
#endif


static BINARY_SEARCH gain_search;


static void try_binary_search(int start_index)
{
	int8_t status;

	float tempC;
	int max_averages;


	tempC = (LastGoodDegF - 32.0) * 5.0 / 9.0;

	max_averages = 5 + MainConfig.damping;  // this was the original.  Too risky to change at this time.
	//max_averages = 3 + MainConfig.damping;  //

	if (sd_TraceSensor)
    {
		char msg[100];
		sprintf(msg, "\n\n====== Binary Search: damping %d max_averages %d =======\n\n", (int)MainConfig.damping, max_averages);
		TraceSensor_AppendMsg(msg);
	}



	init_binary_search_2(&gain_search, start_index, perform_measurement_at_gain, getnext_gain, tempC, max_averages);


#if 1

	// FOR PRODUCTION CODE
	status = binary_search_2(&gain_search);

#else
	// FOR TESTING
	// Force known data and failures here to test "fail safe" code at outer level
	{
		static int counter = 1;

		// Force known data:  level = 1, 2, 3, etc
		status = BIN_SEARCH_IN_RANGE;
		gain_search.ave_distance = MainConfig.vertical_mount - counter;
		gain_search.index_of_next_point = 8;

		if ((counter >= 3) && (counter <= 11))
		{
			status = BIN_SEARCH_TOO_LOW;
			gain_search.index_of_next_point = 0;
			gain_search.ave_distance = 0;
		}

		if ((counter >= 15) && (counter <= 24))
		{
			status = BIN_SEARCH_TOO_HIGH;
			gain_search.index_of_next_point = 0;
			gain_search.ave_distance = 0;
		}

		if ((counter >= 30) && (counter <= 34))
		{
			status = BIN_SEARCH_TOO_HIGH;
			gain_search.index_of_next_point = 0;
			gain_search.ave_distance = 0;
		}
		counter++;
	}
#endif

	if (sd_TraceSensor)
	{
		char msg[140];
		char dist_str[25];
		char status_str[25];
		f_to_str_nn_nn(dist_str, gain_search.ave_distance);
		get_status_str(status, status_str);
		sprintf(msg,"\n\nBinary Search exit: status = %s, avgCnt = %u, ave_dist = %s, ave_gain = %d\n", status_str, gain_search.index_of_next_point, dist_str, gain_search.ave_gain);
		TraceSensor_AppendMsg(msg);
		trace_SaveBuffer();
	}
}


static int last_valid_gain_index = 14; // GAIN_MIDPOINT;  use 14 as it would work for either Gen4 or Gen5; note the sensor_init() function will override this
static float last_valid_distance = 0.0;


extern void Gen4_gain_init(BINARY_SEARCH *p);
extern void Gen5_gain_init(BINARY_SEARCH *p);

void sensor_init(int WhichGen)  // 4 = Gen4, 5 = Gen5
{
	// At run-time, set sensor variables for either Gen4 or Gen5 gain tables
	which_gen = WhichGen;

    if (WhichGen == 4)
	  Gen4_gain_init(&gain_search);   // sets min, max, mid, and gain table
    else
	  Gen5_gain_init(&gain_search);   // sets min, max, mid, and gain table

	last_valid_gain_index = gain_search.midpoint;

}

static float get_sensor_measurement_2(void)
{
	static int   holdover_data_points_reported = 0;

	try_binary_search(last_valid_gain_index);  // zg - why assume that the last data point taken 15 minutes ago gives a good starting point?  Why not always start at the midpoint?

	// check if any valid data was obtained
	if (number_of_points_averaged(&gain_search) == 0)
	{
		// Add a 1 second delay here to:
		// a) fully charge the capacitor
		// b) wait for the water surface to change.
		fully_charge_capacitor();

		if (sd_TraceSensor)
		{
			TraceSensor_AppendMsg("\n\n===== RETRY BINARY SEARCH =====\n\n");
		}

		// Try again at different starting point - force the midpoint to be used
		try_binary_search(gain_search.midpoint);

		// if still no valid data ---
		if (number_of_points_averaged(&gain_search) == 0)
		{
			// use last recorded data point up to 3 times,
			// then trigger an alert by reporting a level equal to the pipe_id
			if (holdover_data_points_reported == 3)
			{
				// Trigger an alert with a sentinel value: distance is zero.

				gain_search.ave_distance = 0;
				last_valid_distance   = 0;  // actually return the sentinel here
				last_valid_gain_index = gain_search.midpoint;

				if (sd_TraceSensor)
				{
					char msg[100];
					char dist_str[25];
					f_to_str_nn_nn(dist_str, gain_search.ave_distance);
					sprintf(msg, "\n\nHOLDOVER ALARM SET to %s\n\n", dist_str);
					TraceSensor_AppendMsg(msg);
				}
			}
			else
			{
			  gain_search.ave_distance = last_valid_distance;
			  holdover_data_points_reported++;

			  if (sd_TraceSensor)
			  {
				  char msg[100];
				  sprintf(msg,"\n\nHOLDOVER DATA REPORTED %d\n\n", holdover_data_points_reported);
				  TraceSensor_AppendMsg(msg);
			  }
			}

			// reset the starting point for the next binary search
			last_valid_gain_index = gain_search.midpoint;
		}
		else
		{
			// reset the holdover watchdog, save the last valid index and last valid distance
			holdover_data_points_reported = 0;
			last_valid_gain_index = gain_search.ave_gain;
			last_valid_distance   = gain_search.ave_distance;
		}
	}
	else
	{
		// reset the holdover watchdog, save the last valid index and last valid distance
		holdover_data_points_reported = 0;
		last_valid_gain_index = gain_search.ave_gain;
		last_valid_distance   = gain_search.ave_distance;
	}

	// Kludge - SIDE EFFECT : set the gain global variable for the log record
	set_sensor_gain(last_valid_gain_index);

	return last_valid_distance;
}

#if 0
static uint8_t take_reading(int repeats, float tempC)
{
	uint8_t status;
	uint8_t rcvPulseCnt;
	float distance;
	int i;

	for (i=0; i < repeats; i++)
	{

		// Perform a reading
		rcvPulseCnt = sensor_single(tempC);

		// get the reading and its status
		status = get_distance(rcvPulseCnt, tempC, &distance);

		//HAL_delay(1000);
	}

	return status;
}

static void sweep_gain_up(float tempC)
{
	int gain;

	// Note: at gains 252-255 there were no outgoing pulses detected by the ISR, so no need to include those in the final search parameters.

	if (sd_TraceSensor) TraceSensor_AppendMsg("\n\n========== Sweep Gain UP: 3 to 255       ==========\n");

	for (gain = 3; gain < 256; gain++)
	//for (gain = 4; gain < 128; gain++)
	//for (gain = 4; gain < 15; gain++)
	{
	    set_sensor_gain((gain & 0xFF));
	    take_reading(3, tempC);
	    if (sd_TraceSensor) TraceSensor_AppendMsg("\n\n=====\n\n");
	}
}

#endif

#if 0
static void sweep_gain_down(float tempC)
{
	int gain;

	if (sd_TraceSensor) TraceSensor_AppendMsg("\n\n========== Sweep Gain DOWN: 255 to 3       ==========\n");

	for (gain = 255; gain > 2; gain--)
	{
	    set_sensor_gain((gain & 0xFF));
	    take_reading(5, tempC);
	    if (sd_TraceSensor) TraceSensor_AppendMsg("\n\n=====\n\n");
	}
}
#endif



#if 0
void sweep_old_code(void)
{
	static uint8_t old_code_Actual_Gain;
	static float   old_code_LastGoodMeasure;

#if 0
	void get_sensor_measurement_0(void)
	{
		sensor_power_en(ON);
		LastGoodMeasure = get_sensor_measurement_1();
		sensor_power_en(OFF);
	}
#endif

	int gain;

	old_code_Actual_Gain = Actual_Gain;
	old_code_LastGoodMeasure = LastGoodMeasure;

	sensor_power_en(ON);

	//for (gain = 4; gain < 256; gain++)
	for (gain = 4; gain < 16; gain++)
	{
		set_sensor_gain((gain & 0xFF));

	    LastGoodMeasure = get_sensor_measurement_1();
	}

	sensor_power_en(OFF);

	// Restore the state to the way it was on entry

	set_sensor_gain((old_code_Actual_Gain & 0xFF));

	LastGoodMeasure = old_code_LastGoodMeasure;

}
#endif


#if 0
void side_by_side_test(void)
{
	uint8_t prev_Actual_Gain;

	prev_Actual_Gain = Actual_Gain;  // save state of old code

	get_sensor_measurement_2();

	set_sensor_gain(prev_Actual_Gain);
}
#endif


void report_last_gain_and_distance(void)
{
	// Intended for testing auto gain loop
}


static void itracker_get_sensor_measurement_0(void)
{
	int prev_pattern;

	prev_pattern = led_set_pattern(LED_PATTERN_OFF);  // need to conserve battery as much as possible, especially during cap charging

	sensor_power_en(ON);  // Note: this calls fully_charge_capacitor()  which takes 1 s

	get_tempF(); // SIDE-EFFECTS !! Sets LastGoodDegF

    LastGoodMeasure = get_sensor_measurement_2();

	sensor_power_en(OFF);

	led_set_pattern(prev_pattern);
}

extern float pga460_take_measurement(float deg_C);

static void pga460_get_sensor_measurement_0(void)
{
	int prev_pattern;
	float deg_C;

	prev_pattern = led_set_pattern(LED_PATTERN_OFF); // need to conserve battery as much as possible, especially during cap charging

	get_tempF(); // SIDE-EFFECTS !! Sets LastGoodDegF

	deg_C = (LastGoodDegF - 32.0) * 5.0 / 9.0;  // Note: USES GLOBAL VARIABLE

	// Kludge for bringing up new SewerWatch board made by Aamon
	//LastGoodDegF = 77.0;
	//LastGoodMeasure = 25.0;
	LastGoodMeasure = pga460_take_measurement(deg_C);

	led_set_pattern(prev_pattern);


}

void get_sensor_measurement_0(void)
{
  uint32_t tickstart;
  uint32_t duration;
  int dec1, dec2;
  char msg[75];

  tickstart = HAL_GetTick();

  if (BoardId == BOARD_ID_SEWERWATCH_1)
  {
	  pga460_get_sensor_measurement_0();
  }
  else
  {
	  itracker_get_sensor_measurement_0();
  }

  duration = HAL_GetTick() - tickstart;

  convfloatstr_2dec(LastGoodMeasure, &dec1, &dec2);

  sprintf(msg, "Data point in %lu ms  %3d.%02d", duration, dec1, dec2);

  AppendStatusMessage(msg);

}



#if 0

void test_new_binary_search(void)
{
	float curTemp;
	float tempC;
	//float distance;
	int status;


	get_tempF(&curTemp);
	tempC = (curTemp - 32) * 5 / 9;



	sensor_power_en(ON);


	for (int i = 0; i < 10; i++)
	{

		init_binary_search_2(&gain_search, gain_search.midpoint, perform_measurement_at_gain, getnext_gain, tempC, 5);

		gain_search.self_test = 1;
        gain_search.test_goal = i;

		status = binary_search_2(&gain_search);

		//if (status == IN_RANGE)
		//{
		//	distance = gain_search.distance;
		//}

		if (sd_TraceSensor)
		{
			char msg[100];
			sprintf(msg,"Exit binary_search_2 status = %d, index = %d\n", status, gain_search.index);
			TraceSensor_AppendMsg(msg);
		}
	}

	if (sd_TraceSensor) trace_SaveBuffer();

	sensor_power_en(OFF);

}
#endif


#if 0
void test_sensor(void)
{

	float curTemp;
	float tempC;
	float tape_distance;

#if 0

	return;
#endif


#if 0
// kludge to see how old code worked

	 sweep_old_code();
 	if (sd_TraceSensor) trace_SaveBuffer();
	 return;
#endif


	get_tempF(&curTemp);
	tempC = (curTemp - 32) * 5 / 9;


	sensor_power_en(ON);

	// to allow moving a target in-between sweeps
	// 0.125 inch per move
	// 200 tests is 25 inches
	//
	// Start target 33 inches away, stop target approximately 8 inches away.
	tape_distance = 31.25;

#ifdef TEST_BINARY_SEARCH

#else
	//for (int i = 0; i < 200; i++)
	//for (int i = 0; i < 2; i++)
#endif
	{
	    //repeated_readings(20, tempC);  // single gain, repeated readings

#if 0
	    if (sd_TraceSensor)
	    {
	    	char msg[100];
	    	char tape_distance_str[20];
			f_to_str_nn_nn(tape_distance_str, tape_distance);
			sprintf(msg, "\n\nTape: %s   0 ms delay after gain change, ~2 ms after each reading \n\n", tape_distance_str);
			TraceSensor_AppendMsg(msg);
	    }
#endif

	    // see if power-on produces better numbers now that the ISR was fixed...

	    //LastGoodMeasure = get_sensor_measurement_1();

	    //report_blanking_array();

	    sweep_gain_up(tempC);

	    //sweep_gain_down(tempC);


    	if (sd_TraceSensor) trace_SaveBuffer();

        tape_distance -= 0.125;

	    HAL_led_delay(2000);
	}

	sensor_power_en(OFF);


}
#endif

