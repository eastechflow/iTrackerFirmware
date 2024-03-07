/*
 * history.c
 *
 *  Created on: Feb 6, 2020
 *      Author: Zachary
 */

#include "debug_helper.h"
//#include "stm32l4xx_hal.h"
#include "Common.h"
#include "time.h"
#include "rtc.h"
//#include "WiFi.h"
#include "WiFi_Socket.h"
#include "history.h"
#include "uart2.h"
#include "Hydro.h"
#include "flume.h"

// User clicked "history"

typedef struct {
  uint8_t  graphType;    // Type of graph requested
  uint8_t  printComma;   // Whether to print a comma prefix
  uint32_t interval;     // size of the interval in seconds
  uint32_t startTime;    // start time in seconds
  uint32_t endTime;      // end time in seconds
  uint32_t count;        // number of records processed in this interval
  uint32_t reported;     // number of intervals reported
  uint32_t total_rec;    // total number of records processed
  float    average;      // running average of all values in this interval.  For SmartFlume, running total of all values in this interval.
  float    total;        // running total of all values in all intervals (used for SmartFlume)
  float    peak_val;     // peak value found in this log interval
  log_t    peak_rec;     // copy of log entry where max value was found
  log_t    prev_rec;     // copy of previous log entry (used to determine interval timestamp)
  float    K;            // K Factor for flume
} HISTORY_INTERVAL;

#define GRAPH_TYPE_LEVEL         1  // iTracker
#define GRAPH_TYPE_VOLUME_RATIO  2  // iTracker
#define GRAPH_TYPE_FLOW          3  // iTracker
#define GRAPH_TYPE_FLUME         4  // SmartFlume



#define USE_ZG_CODE

#ifdef USE_ZG_CODE

static void send_history_header(uint8_t graphType, uint8_t DaysRequested)
{
	char txBuff[WIFI_BUF_SIZE];
	int dec3, dec4;

#if 0
	int dec1, dec2, dec3, dec4;
	uint32_t currentSec;
	struct tm tm_st;

	// Get settings for History header fields.

	currentSec = rtc_get_time();
	sec_to_dt(&currentSec, &tm_st);
    "{\"DateTime\":\"%02d/%02d/%d %02d:%02d\",\"GraphType\": %d,\"Units\": \"%s\",\"Values\":[",
    tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year,
    tm_st.tm_hour, tm_st.tm_min,
#endif

	// Detect the unit types.
	char units[12];
	switch (graphType)
	{
		case 1:
		default:
			// Level
			if (MainConfig.units == UNITS_ENGLISH)
				strcpy(units, "Inches");
			else
				strcpy(units, "Millimeters");
			break;
		case 2:
			// Volume
			strcpy(units, "DeltaV");
			break;
		case 3:
			// Flow
			if (MainConfig.units == UNITS_ENGLISH)
				strcpy(units, "GPM");
			else
				strcpy(units, "LPM");
			break;

	}

	// Pipe ID
	convfloatstr_2dec(MainConfig.pipe_id, &dec3, &dec4);

	// History header fields.
	sprintf(txBuff,
	        "{\"GraphType\":%d,\"GraphDay\":%d, \"Units\":\"%s\",\"Pipe_ID\":%d.%02d,\"Values\":[",
	        graphType, DaysRequested, units, dec3, dec4);
	uart2_tx_string(txBuff);
}



static void report_interval_results(HISTORY_INTERVAL * pInterval)
{
	int dec1, dec2;
	char prefix;
	struct tm tm_st;
	char txBuff[WIFI_BUF_SIZE];

	pInterval->reported++;

	// Determine if a comma is needed for JSON
	if (pInterval->printComma)
	{
		// Add a preceding comma
		prefix = ',';
	}
	else
	{
	  prefix = ' ';
	  pInterval->printComma = 1;  // From now on, print commas
	}


	// Perform unit conversions
	switch (pInterval->graphType)
	{
		default:
		case GRAPH_TYPE_LEVEL: // 1
			// Level
			if (MainConfig.units == UNITS_METRIC)
			{
				pInterval->average *= 25.4;
			}
			break;
		case GRAPH_TYPE_VOLUME_RATIO: // 2
            // Volume Ratio
			break;
		case GRAPH_TYPE_FLOW:  // 3
			// Flow
			if (MainConfig.units == UNITS_METRIC)
			{
				pInterval->average *= 3.79;  // GPM to LPM
			}
			break;
	}


	// Format results
	convfloatstr_2dec(pInterval->average, &dec1, &dec2);


	// Get the timestamp -- this can be determined a number of different ways...
	sec_to_dt(&pInterval->prev_rec.datetime, &tm_st);

	// Add the JSON value object (considering the leading commas)

	sprintf(txBuff, "%c{\"DateTime\":\"%02d/%02d/%d %02d:%02d\",\"Value\":%d.%02d}",
		prefix,
		tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year, tm_st.tm_hour, tm_st.tm_min,
		dec1, dec2);

	uart2_tx_string(txBuff);

}

static void start_interval(log_t * pLogRec,  HISTORY_INTERVAL * pInterval)
{
	pInterval->startTime = pLogRec->datetime;
	pInterval->endTime   = pLogRec->datetime + pInterval->interval;
	//pInterval->total = 0.0;  // not done per interval

	switch (pInterval->graphType)
	{
		default:
		case GRAPH_TYPE_LEVEL:  // 1 Level
			pInterval->average  = log_get_level(pLogRec); // MainConfig.vertical_mount - pLogRec->distance;
			pInterval->peak_val = pInterval->average;
			break;
		case GRAPH_TYPE_VOLUME_RATIO:  //  2 Volume ratio
			pInterval->average = log_get_qratio(pLogRec); // pLogRec->qratio;
			pInterval->peak_val = pInterval->average;
			break;
		case GRAPH_TYPE_FLOW:  // 3 Flow
			pInterval->average = Hydro_CalcFlowFromDeltaQ(log_get_qratio(pLogRec));
			pInterval->peak_val = pInterval->average;
			break;
	}
	pInterval->count = 1;
	pInterval->peak_rec = *pLogRec;
	pInterval->prev_rec = *pLogRec;
}

static void process_iTracker_data(log_t * pLogRec, HISTORY_INTERVAL * pInterval)
{
	// Sum the selected value

	switch (pInterval->graphType)
	{
		default:
		case GRAPH_TYPE_LEVEL:	// 1 Level
			pInterval->average += log_get_level(pLogRec); //MainConfig.vertical_mount - pLogRec->distance;
			break;

		case GRAPH_TYPE_VOLUME_RATIO: //  2 Volume ratio
			pInterval->average += Hydro_CalcNormalizedDeltaQ(log_get_qratio(pLogRec));
			break;

		case GRAPH_TYPE_FLOW: // 3 Flow
			pInterval->average += Hydro_CalcFlowFromDeltaQ(log_get_qratio(pLogRec));
			break;
	}

	// Average the two summed values (running average)
	pInterval->average /= 2;

	if (pInterval->average < 0)
	{
		pInterval->average = 0.0;
	}
}

static void process_SmartFlume_data(log_t * pLogRec, HISTORY_INTERVAL * pInterval)
{
	// Check if very first record
	if (pInterval->total_rec == 1)  return;  // cannot calculate a volume based on a single point in time

	// calculate and sum the volumetric flow between the two logged data points using SmartFlume calculations
	// do not "average", just keep a running total

	pInterval->average += flume_calc_gallons(pInterval->K, &(pInterval->prev_rec), pLogRec, NULL, NULL);

}

static void process_log_record(log_t * pLogRec, HISTORY_INTERVAL * pInterval)
{
	// Check if very first record
	if (pInterval->total_rec == 1)
	{
		start_interval(pLogRec, pInterval);
		return;
	}

	// If the log entry is between the interval bounds, perform a running average.
	if ((pLogRec->datetime >= pInterval->startTime) && (pLogRec->datetime < pInterval->endTime))
	{
		if (pInterval->graphType == GRAPH_TYPE_FLUME)
		{
		  process_SmartFlume_data(pLogRec, pInterval);
		}
		else
		{
		  process_iTracker_data(pLogRec, pInterval);
		}

		if (pInterval->average > pInterval->peak_val)
		{
			pInterval->peak_val = pInterval->average;
			pInterval->peak_rec = *pLogRec;  // save a copy of the peak record for possible future use
		}

		pInterval->prev_rec = *pLogRec;  // save this record (in case it is the last one) so we have an interval timestamp

		pInterval->count++;
	}
	else
	{
		// The current log record is not within this interval.

		// Totalize all interval values for SmartFlume
		if (pInterval->graphType == GRAPH_TYPE_FLUME)
		{
			pInterval->total += pInterval->average;
		}

		// Report the results of the interval
		report_interval_results(pInterval);

		// Use this record to start the next interval
		start_interval(pLogRec, pInterval);
	}

}

static void send_blank_log_entry()
{
	char txBuff[WIFI_BUF_SIZE];

	// If no records found, send an empty JSON value object.
	sprintf(txBuff, "{\"DateTime\":\"00/00/00 00:00\",\"Value\":0.0}");
	uart2_tx_string(txBuff);
}

static void send_history_body(uint32_t StartAddr, HISTORY_INTERVAL * pInterval)
{
  log_t log_rec;


  // Check if totally empty log file
  if (StartAddr == 0)
  {
	  send_blank_log_entry();
	  return;
  }



  log_EnumStart(StartAddr, LOG_FORWARD);

  while (log_EnumGetNextRecord(&log_rec, LogTrack.next_logrec_addr))
  {
	  pInterval->total_rec++;

	  process_log_record(&log_rec,  pInterval);

	  led_heartbeat();
  }

  // Report last interval, if one in progress
  if (pInterval->count)
  {
	  report_interval_results(pInterval);
  }

#if 0
  if (pInterval->reported)
  {
	  int i = 10;
	  while (i--); // for debugging breakpoint
  }
#endif
}

static void send_history_footer(HISTORY_INTERVAL * pInterval)
{
	int dec1, dec2;
	char txBuff[WIFI_BUF_SIZE];
#if 0
	// Detect the unit types.
	char units[12];
	switch (graphType)
	{
		case 2:
			// Volume
			strcpy(units, "DeltaV");
			break;
		case 3:
			// Flow
			if (MainConfig.units == UNITS_ENGLISH)
				strcpy(units, "GPM");
			else
				strcpy(units, "LPM");
			break;
		default:
			// Level
			if (MainConfig.units == UNITS_ENGLISH)
				strcpy(units, "Inches");
			else
				strcpy(units, "Millimeters");
			break;
	}

	// Calculate Delta V values.
	char valsD2[10];  // zg are these big enough?
	char valsP2[10];
	if (DQ.DQ_Days)
	{
		if (totalavgcnt)
		{
			totalfloat /= totalavgcnt;
		}
		convfloatstr_2dec(totalfloat, &dec1, &dec2);
		sprintf(valsD2, "%d.%02d", dec1, dec2);

		convfloatstr_2dec(peakfloat, &dec1, &dec2);
		sprintf(valsP2, "%d.%02d", dec1, dec2);
	}
	else
	{
		strcpy(valsD2, "INVALID");
		strcpy(valsP2, "INVALID");
	}
#endif

	convfloatstr_2dec(pInterval->total, &dec1, &dec2);

	// Send the closing summary data.
	sprintf(txBuff, "],\"ReportedIntervals\":%lu,\"TotalRecords\":%lu,\"TotalValue\":%d.%02d}", pInterval->reported, pInterval->total_rec, dec1, dec2);

	uart2_tx_string(txBuff);
}



void wifi_send_history(uint8_t graphType, uint8_t days)
{
	uint32_t addr;
	HISTORY_INTERVAL interval;

#if 0
	if (days == 30)
	{
		extern uint8_t zg_debug;
		zg_debug = 1;

		int i;
		for (i=0;i < 10; i++); // for debugging put a breakpoint here..
	}
#endif

	// Kludge for testing:
	//graphType = GRAPH_TYPE_FLUME;

	// Prepare the interval object
	interval.graphType  = graphType;
	interval.printComma = 0;
	interval.interval   = 3600;  // 1 hour intervals
	interval.count      = 0;
	interval.reported   = 0;
	interval.total_rec  = 0;     // no data records processed

	// For SmartFlume
	switch ((int)MainConfig.pipe_id)
	{
	case 4:  interval.K = flume_K_Factor[0]; break;
	case 6:  interval.K = flume_K_Factor[1]; break;
	default:
	case 8:  interval.K = flume_K_Factor[2]; break;
	case 10: interval.K = flume_K_Factor[3]; break;
	case 12: interval.K = flume_K_Factor[4]; break;
	}

	interval.total      = 0.0;   // SmartFlume totalizer always starts at zero

	addr = log_FindPreviousEntry((uint32_t) days);  // Note: can return zero if no entries

	send_history_header(graphType, days);

	send_history_body(addr, &interval);

	send_history_footer(&interval);

	// Delay to make sure the last output buffer for the UART has cleared.
	//HAL_Delay(2000);


}


#else
// zg Originial RC16 code...



#endif


