/*
 * testalert.c
 *
 *  Created on: May 22, 2020
 *      Author: Zachary
 */
#include "stdint.h"
#include "stdlib.h"  // atoi
#include "time.h"
#include "rtc.h"
#include "fatfs.h" // SD Card
#include "sdmmc.h" // SD Card
#include "common.h"
#include "dual_boot.h"
#include "bootloader.h"
#include "version.h"
#include "debug_helper.h"
#include "WiFi_Socket.h"
#include "Sample.h"
#include "trace.h"
#include "factory.h"
#include "testalert.h"



typedef struct {
	float LastGoodMeasure;
	//int   ExpectedEvent;
} SIM_DATA_POINT;

#define MAX_SIM_DATA 100
static SIM_DATA_POINT sim_data[MAX_SIM_DATA];
static int current_sim_point = 0;
static int max_sim_points = 0;

static void add_point(float LastGoodMeasure)
{
	char msg[80];
	int dec1, dec2;

	sim_data[current_sim_point].LastGoodMeasure = LastGoodMeasure;
	//sim_data[current_sim_point].ExpectedEvent   =  ExpectedEvent;
	current_sim_point++;

	convfloatstr_2dec(LastGoodMeasure, &dec1, &dec2);
	sprintf(msg,"Alert Sim Data %d.%02d", dec1, dec2);

	AppendStatusMessage(msg);
}
// It is assumed that vertical mount = 30
// alert_1 = 23 at distance of 7
// alert_2 = 27 at distance of 3
// An excel spreadsheet was used to generate this table
static void init_sim_data(void)
{
	char msg[80];
	add_point( 9 );  // logs 9
	add_point( 8 );  // logs 8
	add_point( 7 );
	add_point( 8 );  // logs 8

	add_point( 7 );
	add_point( 7 );  // retry
	add_point( 8 );  // logs 8

	add_point( 7 );
	add_point( 7 ); // retry
	add_point( 7 ); // retry
	add_point( 8 ); // logs 8

	add_point( 7 );
	add_point( 7 ); // retry
	add_point( 7 ); // retry
	add_point( 7 ); // logs 7 - alert is "confirmed"

	add_point( 7 );
	add_point( 7 ); // retry
	add_point( 7 ); // retry
	add_point( 7 ); // logs 7 - alert is "confirmed"

	add_point( 7 );
	add_point( 7 ); // retry
	add_point( 7 ); // retry
	add_point( 7 ); // logs 7 - alert is "confirmed"

	add_point( 6 );
	add_point( 6 );
	add_point( 6 );
	add_point( 7 ); // logs 7 - alert is "confirmed"

	add_point( 5 );
	add_point( 5 );
	add_point( 5 );
	add_point( 5 ); // logs 5 - alert is "confirmed"

	add_point( 4 );
	add_point( 4 );
	add_point( 4 );
	add_point( 4 ); // logs 4 - alert is "confirmed"

	add_point( 3 );
	add_point( 3 );
	add_point( 3 );
	add_point( 3 ); // logs 3 - alert is "confirmed" (at level 2 now)

	add_point( 7 );
	add_point( 7 );
	add_point( 7 );
	add_point( 7 ); // logs 7 - alert is "confirmed" (2 to 1 transition, no cell call)

	add_point( 2 );
	add_point( 2 );
	add_point( 2 );
	add_point( 2 ); // logs 2 - alert is "confirmed" (1 to 2 transition, no cell call)


	add_point( 8 );
	add_point( 8 );
	add_point( 8 );
	add_point( 8 );  // logs 8 -  should clear alerts (2 to 0 transition, cell call)


	max_sim_points = current_sim_point;
	current_sim_point = 0;

	sprintf(msg,"Alert Max Sim Data Points %d", max_sim_points);
	AppendStatusMessage(msg);


#if 0

	add_point( 7, EVENT_TRIGGER_ALERT_1);
	add_point( 7, EVENT_TRIGGER_ALERT_1);
	add_point( 7, EVENT_TRIGGER_ALERT_1);
	add_point( 7, EVENT_TRIGGER_ALERT_1);
	add_point( 7, EVENT_TRIGGER_ALERT_1);


	add_point( 6, 0);
	add_point( 5, 0);
	add_point( 4, 0);
	add_point( 5, 0);
	add_point( 6, 0);
	add_point( 7, 0);
	add_point( 8, EVENT_CLEAR_ALERT);
	add_point( 7, EVENT_TRIGGER_ALERT_1);
	add_point( 6, 0);
	add_point( 5, 0);
	add_point( 4, 0);
	add_point( 3, EVENT_TRIGGER_ALERT_2);
	add_point( 2, 0);
	add_point( 1, 0);
	add_point( 2, 0);
	add_point( 3, EVENT_NO_ACTION);
	add_point( 4, 0);
	add_point( 5, 0);
	add_point( 4, 0);
	add_point( 3, EVENT_NO_ACTION);
	add_point( 2, 0);
	add_point( 3, EVENT_NO_ACTION);
	add_point( 4, 0);
	add_point( 5, 0);
	add_point( 6, 0);
	add_point( 7, EVENT_NO_ACTION);
	add_point( 8, EVENT_CLEAR_ALERT);
	add_point( 9, 0);
#endif
}

#define TIME_BETWEEN_ALERT_PHONE_CALLS    15000

float testalert_get_next_sim_value(void)
{
	float result;

	if (sd_TestCellAlerts)
	{

		if (current_sim_point == max_sim_points)
		{

			current_sim_point = 0;
			AppendStatusMessage("Alert restart sim values");

		}

		result = sim_data[current_sim_point].LastGoodMeasure;

		//if (sd_TestCellNoTx == 0)  // If actually sending data, enforce a time gap between alert values
		//{
		//	if (sim_data[current_sim_point].ExpectedEvent) HAL_led_delay(15000);  // Force a delay here to give space between alert phone calls, note this affects log spacing
		//}

		if (current_sim_point < max_sim_points)  current_sim_point++;


	}
	else
	{
		result = ((float) current_sim_point) + 1.0;  // repetitive cycle 1..96
		current_sim_point++;
		if (current_sim_point >= 96) current_sim_point = 0;
	}

	return result;
}

#if 0
void testalert_verify_action(int ActualEvent, float ActualDistance)
{
	int prev_sim_point;

	prev_sim_point = current_sim_point - 1;
	if (prev_sim_point < 0) prev_sim_point = 0;

	if (ActualEvent != sim_data[prev_sim_point].ExpectedEvent)
	{
		char msg[225];

		char expected_distance_str[40];
		char actual_distance_str[40];
		int dec1, dec2;
		convfloatstr_2dec(ActualDistance, &dec1, &dec2);
		sprintf(actual_distance_str,"%d.%02d", dec1, dec2);

		convfloatstr_2dec(sim_data[prev_sim_point].LastGoodMeasure, &dec1, &dec2);
		sprintf(expected_distance_str,"%d.%02d", dec1, dec2);

		sprintf(msg,"\n=====\n\nUpcoming Alert mismatch (%d), expected %d at %s   received %d at %s\n",
				prev_sim_point, sim_data[prev_sim_point].ExpectedEvent, expected_distance_str, ActualEvent, actual_distance_str);
		trace_AppendMsg(msg);
	}
}
#endif

void testalert_init(int TestAlerts)
{

	//MainConfig.log_interval = 60;  // sleep for 60 seconds  --- this really screws up the log file...
	MainConfig.log_interval = 120; // 2 minute intervals


	MainConfig.vertical_mount = 30;

	if (TestAlerts)
	{
		MainConfig.AlertLevel1 = 23;
		MainConfig.AlertLevel2 = 27;
	}
	else
	{
		MainConfig.AlertLevel1 = 0;
		MainConfig.AlertLevel2 = 0;
	}



	// Enable the Cell Phone
	SetCellEnabled();


	// Clear the logs as opposed to skipping already logged (but not sent) data
	log_start(0);
	//LogTrack.last_cellsend_addr = LogTrack.next_logrec_addr;  // only send simulated data, not previously logged data

	init_sim_data();

	current_sim_point = 0;

}

int testalert_get_current_sim_point(void)
{
	return current_sim_point;
}


