/*
 * HydroDecomp.c
 *
 *  Created on: Mar 12, 2020
 *      Author: Zachary
 */

#include "main.h"
#include "rtc.h"
#include "DeltaQ.h"
#include "time.h"
#include "Log.h"
#include "Hydro.h"


/* Original description from  Mark:
 *
 *

Hydrograph decomposition is a way of estimating the different flow components in the sewer system.
The decomposition process determines the three essential sources of flow and RDII characteristics.
The three components making up the flow in a sewer system are listed below:

•Base Wastewater Flow (BWF): The wastewater flow from residential, commercial, and industrial areas and releases to a sanitary sewer system.
•Ground Water infiltration (GWI): GWI is the groundwater that enters the collection system through defects in sewer pipes, pipe joints, and manhole.
  It occurs when the water table is higher than the hydraulic grade line of the sewer pipes and components.
•Rain Derived Inflow and Infiltration (RDII): RDII is the rainfall-driven flow that enters a sewer network in direct response to the intensity and duration of rainfall events.
  It occurs during and following rainfall events, and ends when the flow pattern returns to the pre-rainfall levels.

Method:
To identify these three separate components and estimate their values in terms of flow, using iTrackers, we must start with what is known and what can logically be assumed.
We can ascertain the ID of the pipe, the iTrackers provide sewer levels, and the area serviced by the infrastructure leading to the monitored sites can be ascertained from sewer maps.
Follow these steps to accomplish the decomposition and conversion to flows:
1.	Applying the DeltaQ formula input a small reference value like .01” for the dry number, and each level for the wet values, the formula gives us the ratiometric change for each recorded time.
    Store these values as DeltaQs or DeltaVs.
2.	Grouping the ratiometric values by day, calculate the average ratiometric value for each day.  To simplify, I call this average daily ratiometric value the Average Daily DeltaQ.
3.	Find the Average Dry Day DeltaQ:
    a.	Find the Dry Weekday DeltaQ by averaging lowest 20% of the weekday Average Daily DeltaQs
    b.	Find the Dry Weekend DeltaQ by averaging the lowest 20% of the weekend Average Daily DeltaQs.
    c.	Multiply the Dry Weekday DeltaQ by (5/7) add this to the Dry Weekend DeltaQ multiplied by (2/7) and divide the result by 2.
        Dry Day DeltaQ = ((Dry Weekday DeltaQ * (5/7)) + (Dry Weekend DeltaQ * (2/7))) / 7.
4.	Find the average of the lowest DeltaQ for each of the driest days to get the Groundwater DeltaQs.
    Subtract the Groundwater DeltaQs from the Dry Day DeltaQs to get the Base DeltaQs.

    a.	Calculate the GPM per DeltaQ multiplying the number of residences serviced by each site by the average household water usage in GPD for the Base Flow.
    b.	Divide the Base Flow by 1440 to get Base Flow in GPM.
    c.	Divide the Base Flow in GPM by the Base DeltaQ to get a GPM per DeltaQ.
5.	Convert levels to flows by multiplying each DeltaQ by the GPM per DeltaQ for each level reading.


Zack's idea for a computer implementation:
Group the range of data specified into an array of Day objects (maximum of 365 Day objects).
Each Day object contains data averaged from the underlying readings, the type of day (weekday or weekend_day)
and also other data that will be computed from "MetaData".

MetaData is gathered by making several passes through the Day array:

Step1:
   max_daily_val, min_daily_val, total_number_of_days
   max_weekday_val, min_weekday_val, total_number_of_weekdays
   max_weekend_val, min_weekend_val, total_number_of_weekend_days
Step3:
   scan day array for max and min daily deltaQ (need to find the max out of all of them to do ranking)
   Then walk day array again:
     mark each day with % rank
     count of days below 20% Threshold
     count of weekdays below 20% Threshold
     count of weekend_days below 20% Threshold
   calculate Average Dry Day DeltaQ
Step4:
  Calculate Groundwater_DeltaQs as the average of ??  is there just one Groundwater_DeltaQ for the entire data set? Yes.
FourthPass:
  Calculate Base_DeltaQs
  Calculate GPM per Base_DeltaQ  - Need number of residences and average household water usage in GPD
FifthPass:
  Convert levels(ratios) to flows - multiply each Daily_DeltaQ by GPM_per_DeltaQ

 *
 * When reporting "DeltaQ" values, you actually report DeltaQ/AveDryDayDeltaQ.
 * Essentially, normalizing the data to the AveDryDayDeltaQ
 *
 */

#define MAX_DAYS  365

#define WEEK_DAY     1
#define WEEKEND_DAY  2

typedef struct
{
  uint32_t FirstAddr;
  uint32_t LastAddr;
  int      NumberOfRecords;
  float    Sum_DailyDeltaQ;
  float    Ave_DailyDeltaQ;
  float    Max_DailyDeltaQ;
  float    Min_DailyDeltaQ;
  int      DayType;  // Weekday, or week-end day
  int      Rank;
  int      IsDryDay;

} HYDROGRAPH_DECOMP_DAY;


typedef struct
{
  // INPUT
  HYDRO_DECOMP_INPUT input;

  // Steps 1 and 2
  int TotalNumberOfDays;
  int StartIndex;  // Skip first day
  int StopIndex;   // Skip last day
  float Max_DailyDeltaQ;
  float Min_DailyDeltaQ;

  int   NumberOfWeekdays;
  float Max_WeekdayDeltaQ;
  float Min_WeekdayDeltaQ;

  int   NumberOfWeekendDays;
  float Max_WeekendDayDeltaQ;
  float Min_WeekendDayDeltaQ;

  // Step 3
  int   NumberOfDryWeekdays;
  float SumOfDryWeekdays;
  int   NumberOfDryWeekendDays;
  float SumOfDryWeekendDays;
  float AveDryWeekdayDeltaQ;
  float AveDryWeekendDayDeltaQ;
  float AveDryDayDeltaQ;

  // Step 4
  float GroundwaterDeltaQ;
  float BaseDeltaQ;
  float BaseFlow_GPD;
  float BaseFlow_GPM;
  float GPM_per_DeltaQ;

} HYDROGRAPH_META_DATA;



static HYDROGRAPH_DECOMP_DAY Day[MAX_DAYS];
static HYDROGRAPH_META_DATA  Hydrograph;

// State machine variables
static int      day_first_record = 1;
static int      day_index = 0;

static uint32_t prev_log_addr;
static log_t    prev_log_rec;


//========================================================================================
// Print a CSV file to allow inspection and analysis of decomposition.
// Output file name:  HydroGraphDecomposition.csv
// Output produced only if an empty HydroGraphDecomposition.csv file already exists.
// Otherwise, no output is produced.
//

#if 0
// need a variadic function...
static void f_sprintf_float(FIL * fOutput, char *fmt, float value)
{
	UINT sdCardBytes;
	char txt[50];
	char f_str[F_TO_STR_LEN];

	sprintf(txt, fmt, value);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);
}
#endif


static void print_input(FIL * fOutput)
{
	UINT sdCardBytes;
	char txt[50];
	char f_str[F_TO_STR_LEN];

	sprintf(txt, "FirstLogAddr,%lu\r", Hydrograph.input.FirstLogAddr);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "LastLogAddr,%lu\r", Hydrograph.input.LastLogAddr);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "VerticalMount,%s\r", f_to_str_2dec(f_str, Hydrograph.input.VerticalMount));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "Pipe_ID,%s\r", f_to_str_2dec(f_str, Hydrograph.input.Pipe_ID));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "Population,%lu\r", Hydrograph.input.Population);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "AveWaterUsage_GPD,%s\r", f_to_str_2dec(f_str, Hydrograph.input.AveWaterUsage_GPD));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);
}

static void print_meta_data(FIL * fOutput)
{
	UINT sdCardBytes;
	char txt[50];
	char f_str[F_TO_STR_LEN];

	print_input(fOutput);

	sprintf(txt, "TotalNumberOfDays,%d\r", Hydrograph.TotalNumberOfDays);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "StartIndex,%d, Skip First Day\r", Hydrograph.StartIndex);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "StopIndex,%d, Skip Last Day\r", Hydrograph.StopIndex);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "Max_DailyDeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.Max_DailyDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "Min_DailyDeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.Min_DailyDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);


	sprintf(txt, "NumberOfWeekdays,%d\r", Hydrograph.NumberOfWeekdays);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "Max_WeekdayDeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.Max_WeekdayDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "Min_WeekdayDeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.Min_WeekdayDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);


	sprintf(txt, "NumberOfWeekendDays,%d\r", Hydrograph.NumberOfWeekendDays);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "Max_WeekendDayDeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.Max_WeekendDayDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "Min_WeekendDayDeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.Min_WeekendDayDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);


	sprintf(txt, "NumberOfDryWeekdays,%d\r", Hydrograph.NumberOfDryWeekdays);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "SumOfDryWeekdays,%s\r", f_to_str_4dec(f_str, Hydrograph.SumOfDryWeekdays));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);


	sprintf(txt, "NumberOfDryWeekendDays,%d\r", Hydrograph.NumberOfDryWeekendDays);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "SumOfDryWeekendDays,%s\r", f_to_str_4dec(f_str, Hydrograph.SumOfDryWeekendDays));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);


	sprintf(txt, "AveDryWeekdayDeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.AveDryWeekdayDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "AveDryWeekendDayDeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.AveDryWeekendDayDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "AveDryDayDeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.AveDryDayDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "GroundwaterDeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.GroundwaterDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "BaseDeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.BaseDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "BaseFlow_GPD,%s\r", f_to_str_4dec(f_str, Hydrograph.BaseFlow_GPD));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "BaseFlow_GPM,%s\r", f_to_str_4dec(f_str, Hydrograph.BaseFlow_GPM));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "GPM_per_DeltaQ,%s\r", f_to_str_4dec(f_str, Hydrograph.GPM_per_DeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

}

static void print_day_header(FIL * fOutput)
{
	UINT sdCardBytes;
	char txt[150];

	sprintf(txt, "\n\nDate,FirstAddr,LastAddr,NumberOfRecords,Sum_DailyDeltaQ,Ave_DailyDeltaQ,Max_DailyDeltaQ,Min_DailyDeltaQ,");
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "DayType,IsDryDay\n");
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);
}

static void print_date(FIL * fOutput, uint32_t addr)
{
	UINT sdCardBytes;
	char txt[150];

	log_t log_rec;
	uint32_t this_sec;
	struct tm this_date;

	log_GetRecord(&log_rec, addr, LOG_FORWARD);

	this_sec = log_rec.datetime;

	sec_to_dt(&this_sec, &this_date);
	sprintf(txt, "%02d/%02d/%02d,", this_date.tm_mon, this_date.tm_mday, this_date.tm_year);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

}

static void print_day(FIL * fOutput, int i)
{
	UINT sdCardBytes;
	char txt[50];
	char f_str[F_TO_STR_LEN];

	print_date(fOutput, Day[i].FirstAddr);

	sprintf(txt, "%lu,", Day[i].FirstAddr);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "%lu,", Day[i].LastAddr);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "%d,", Day[i].NumberOfRecords);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "%s,", f_to_str_4dec(f_str, Day[i].Sum_DailyDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "%s,", f_to_str_4dec(f_str, Day[i].Ave_DailyDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "%s,", f_to_str_4dec(f_str, Day[i].Max_DailyDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "%s,", f_to_str_4dec(f_str, Day[i].Min_DailyDeltaQ));
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "%d,", Day[i].DayType);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

	sprintf(txt, "%d\n", Day[i].IsDryDay);
	f_write(fOutput, txt, strlen(txt), &sdCardBytes);

}

static void Print_CSV_File(const TCHAR * ImportedFilename)
{
	FRESULT fr;
	FIL fOutput;
	char fname[150];

	sprintf(fname,"%s.HydroDecomp.csv", ImportedFilename);

    // Open file
    fr = f_open(&fOutput, fname, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) return;

    print_meta_data(&fOutput);

    print_day_header(&fOutput);

	for (int i=0; i < Hydrograph.TotalNumberOfDays; i++)
	{
		print_day(&fOutput, i);
	}

	f_close(&fOutput);

}

//========================================================================================

static int is_same_day(log_t *pPrevLogRec, log_t * pLogRec)
{
	uint32_t prev_sec;
	uint32_t this_sec;
	struct tm prev_date;
	struct tm this_date;
	prev_sec = pPrevLogRec->datetime;
	this_sec = pLogRec->datetime;

	sec_to_dt(&prev_sec, &prev_date);
	sec_to_dt(&this_sec, &this_date);

	if (prev_date.tm_year == this_date.tm_year)
	{
		if (prev_date.tm_mon == this_date.tm_mon)
		{
			if (prev_date.tm_mday == this_date.tm_mday) return 1;
		}
	}
	return 0;
}

static int get_day_type(log_t * pLogRec)
{

	uint32_t this_sec;
	struct tm this_date;

	this_sec = pLogRec->datetime;

	sec_to_dt(&this_sec, &this_date);

	if ((this_date.tm_wday == 6) || (this_date.tm_wday == 7)) return WEEKEND_DAY;

	return WEEK_DAY;
}

static float CalcDeltaQ(log_t * pLogRec)
{
	// Compute the DeltaQ
	float level;
	float qratio;

	level = Hydrograph.input.VerticalMount - pLogRec->distance;
	if (level < .1)
		level = .1;

	qratio = Delta_Q(Hydrograph.input.Pipe_ID, level);

	return qratio;
}



static void Step1_FinalizeCurrentDay(void)
{
	Day[day_index].Ave_DailyDeltaQ = Day[day_index].Sum_DailyDeltaQ / Day[day_index].NumberOfRecords;
	Day[day_index].DayType = get_day_type(&prev_log_rec);
	Day[day_index].IsDryDay = 0;
	Day[day_index].LastAddr = prev_log_addr;
}

static void Step1_StartCurrentDay(float deltaQ, uint32_t LogRecAddr)
{
	Day[day_index].Max_DailyDeltaQ = deltaQ;
	Day[day_index].Min_DailyDeltaQ = deltaQ;
	Day[day_index].Sum_DailyDeltaQ = 0;

	Day[day_index].FirstAddr = LogRecAddr;
}


static void Step1_ProcessRecord(uint32_t LogRecAddr, log_t * pLogRec)
{

    float deltaQ;

	deltaQ = CalcDeltaQ(pLogRec);

    // Is this the very first record?
	if (day_first_record)
	{
		// This is the very first record processed, sync the previous record
		prev_log_addr = LogRecAddr;
		prev_log_rec  = *pLogRec;

		Step1_StartCurrentDay(deltaQ, LogRecAddr);

		day_first_record = 0;
	}

	// Which day is this record a part of?	The current day or the next day?
	if (!is_same_day(&prev_log_rec, pLogRec))
	{
		// Finalize the current day before moving on
		Step1_FinalizeCurrentDay();

		day_index++;

		Step1_StartCurrentDay(deltaQ, LogRecAddr);
	}

	Day[day_index].Sum_DailyDeltaQ += deltaQ;

	if (deltaQ > Day[day_index].Max_DailyDeltaQ) Day[day_index].Max_DailyDeltaQ = deltaQ;
	if (deltaQ < Day[day_index].Min_DailyDeltaQ) Day[day_index].Min_DailyDeltaQ = deltaQ;

	Day[day_index].NumberOfRecords++;

	// Get ready for next record
	prev_log_addr = LogRecAddr;
	prev_log_rec = *pLogRec;

}

static void Init_Hydrograph_Results(void)
{
  int i;

  day_first_record = 1;
  day_index = 0;

  for (i = 0; i < MAX_DAYS; i++)
	{
	  Day[i].Ave_DailyDeltaQ = 0;
	  Day[i].DayType = 0;
	  Day[i].FirstAddr = 0;
	  Day[i].IsDryDay = 0;
	  Day[i].LastAddr = 0;
	  Day[i].Max_DailyDeltaQ = 0;
	  Day[i].Min_DailyDeltaQ = 0;
	  Day[i].NumberOfRecords = 0;
	  Day[i].Sum_DailyDeltaQ = 0;
	}
}




// Note: Mark does not want to use partial days worth of data.
// In particular, when setting up the iTracker, there may be a few
// data points that are taken before installation is complete.
// So, skip data on the first partial days.
// This may pose a more general problem as the iTracker is
// quite often removed to download data, then reinstalled.
// If one assumes the WiFi connection is active, then no new
// data is logged during that time.  But, if the unit is
// placed after the WiFi connection is terminated, then the
// same problem will exist in the middle of a data set...
// Unclear how to solve that problem.
static void Step1_SkipData(void)
{
	  Hydrograph.StartIndex = 1;  // Skip first day
	  Hydrograph.StopIndex  = Hydrograph.TotalNumberOfDays - 1;   // Skip last day

}

static void Step1(void)
{
  uint32_t log_addr;
  log_t    log_rec;

  Init_Hydrograph_Results();



  log_EnumStart(Hydrograph.input.FirstLogAddr, LOG_FORWARD);

  while (log_EnumGetNextRecord(&log_rec, LogTrack.next_logrec_addr))
  {
	  log_addr = log_EnumGetLastAddr();

	  Step1_ProcessRecord(log_addr, &log_rec);

	  // Stop enumerating records at the requested stop address
	  if (log_addr == Hydrograph.input.LastLogAddr) break;
  }

  Step1_FinalizeCurrentDay();

  Hydrograph.TotalNumberOfDays = day_index + 1;

  // Skip First Day and Last
  Step1_SkipData();
}



static void Step3_FindMaxMinValues(void)
{
	// Find the max and min DailyDeltaQ
	for (int i = Hydrograph.StartIndex; i < Hydrograph.StopIndex; i++)
	{
		if (i==Hydrograph.StartIndex)
		{
			Hydrograph.Max_DailyDeltaQ = Day[i].Max_DailyDeltaQ;
			Hydrograph.Min_DailyDeltaQ = Day[i].Min_DailyDeltaQ;
			Hydrograph.NumberOfWeekdays = 0;
			Hydrograph.NumberOfWeekendDays = 0;
		}

		if (Hydrograph.Max_DailyDeltaQ < Day[i].Max_DailyDeltaQ) Hydrograph.Max_DailyDeltaQ = Day[i].Max_DailyDeltaQ;
		if (Hydrograph.Min_DailyDeltaQ > Day[i].Min_DailyDeltaQ) Hydrograph.Min_DailyDeltaQ = Day[i].Min_DailyDeltaQ;

		if (Day[i].DayType == WEEK_DAY)
		{
			if (Hydrograph.NumberOfWeekdays == 0)
			{
				// Init max and min values
				Hydrograph.Max_WeekdayDeltaQ = Day[i].Max_DailyDeltaQ;
				Hydrograph.Min_WeekdayDeltaQ = Day[i].Min_DailyDeltaQ;
			}
			Hydrograph.NumberOfWeekdays++;
			if (Hydrograph.Max_WeekdayDeltaQ < Day[i].Max_DailyDeltaQ) Hydrograph.Max_WeekdayDeltaQ = Day[i].Max_DailyDeltaQ;
			if (Hydrograph.Min_WeekdayDeltaQ > Day[i].Min_DailyDeltaQ) Hydrograph.Min_WeekdayDeltaQ = Day[i].Min_DailyDeltaQ;
		}
		else
		{
			if (Hydrograph.NumberOfWeekendDays == 0)
			{
				// Init max and min values
				Hydrograph.Max_WeekendDayDeltaQ = Day[i].Max_DailyDeltaQ;
				Hydrograph.Min_WeekendDayDeltaQ = Day[i].Min_DailyDeltaQ;
			}

			Hydrograph.NumberOfWeekendDays++;
			if (Hydrograph.Max_WeekendDayDeltaQ < Day[i].Max_DailyDeltaQ) Hydrograph.Max_WeekendDayDeltaQ = Day[i].Max_DailyDeltaQ;
			if (Hydrograph.Min_WeekendDayDeltaQ > Day[i].Min_DailyDeltaQ) Hydrograph.Min_WeekendDayDeltaQ = Day[i].Min_DailyDeltaQ;
		}
	}
}


// Sorting days by rank (driest to wettest)
// 365 days per year implies 260 weekdays and 105 weekend_days, to be safe use 262 and 108.
// at 4 bytes per array element, (262+108) * 4 = 1,480 bytes.
// Could potentially do sequential sorting and ranking (weekdays first, then weekends) and
// share the same sort array: 262*4 = 1,048 bytes.  Very close to the size of the TX buffers.
// We can potentially use one of the TX buffers (WiFi or Cell).
// Or restrict data to a max of 183 days?
// Or maybe just keep track of the lowest days, using an insertion sort.
// If we assume the 20% cutoff, then 20% of 260 is 52.  Only keep that many days.
// When adding a day, if the buffer is empty, add the day.
// If the buffer has the max allowed day count for the sample set, then insert the entry
// into the array and push out the largest number.
#define MAX_RANK  52
#define EMPTY_SLOT -1;
static int max_count;
static int next_index;
static int rank[MAX_RANK];  // this contains the index of a Day[] that meets the ranking


static void rank_init(int number_of_samples, int percentage)
{
	max_count = (number_of_samples * percentage + 50) / 100;  // integer division with round up

	if (max_count < 1) max_count = 1;  // Must always have at least one sample data point
	if (max_count> MAX_RANK) max_count = MAX_RANK;
	next_index = 0;

	for (int i=0; i < max_count; i++)
	{
		rank[i] = EMPTY_SLOT;
	}
}

static void displace_day(int insertion_point, int day_index)
{
	for (int i = next_index - 1; i > insertion_point; i--)
	{
		rank[i] = rank[i-1];
	}
	rank[insertion_point] = day_index;
}

static void insert_day(int insertion_point, int day_index)
{
	next_index++; // create a gap at the end
	for (int i = next_index; i > insertion_point; i--)
	{
		rank[i] = rank[i-1];
	}
	rank[insertion_point] = day_index;
}

static int find_insertion_point(int day_index)
{
	int i;
	for (i = 0; i < next_index; i++)
	{
		if (Day[day_index].Ave_DailyDeltaQ < Day[rank[i]].Ave_DailyDeltaQ )
		{
			return i;
		}
	}
	return i;
}

static void rank_add_day(int day_index)
{
	int insertion_point;

	insertion_point = find_insertion_point(day_index);

	if (next_index < max_count)
	{
		insert_day(insertion_point, day_index);
	}
	else
	{
		displace_day(insertion_point, day_index);
	}
}

static void rank_apply_ranking(void)
{
	for (int i = 0; i < next_index; i++)
	{
		Day[rank[i]].IsDryDay = 1;
	}
}

// The "PercentThreshold" refers to the number of days out of the sample set to use for finding the dry day average.
// That is, if there are 100 weekday samples (5 days * 20 weeks) and 40 weekend day samples (2 days * 20 weeks)
// and the threshold is 20%, then for the weekday samples you would use the lowest 100*.2 = 20 samples for the average,
// and 40 * 0.2 = 8 samples for the weekend days.
//
static void Step3_AssignRank(int PercentThreshold)
{


	Hydrograph.SumOfDryWeekdays = 0;
	Hydrograph.SumOfDryWeekendDays = 0;

	// Rank the weekdays
	rank_init(Hydrograph.NumberOfWeekdays, PercentThreshold);
	for (int i = Hydrograph.StartIndex; i < Hydrograph.StopIndex; i++)
	{
		if (Day[i].DayType == WEEK_DAY) rank_add_day(i);
	}
	rank_apply_ranking();

	// Rank the weekend days
	rank_init(Hydrograph.NumberOfWeekendDays, PercentThreshold);
	for (int i = Hydrograph.StartIndex; i < Hydrograph.StopIndex; i++)
	{
		if (Day[i].DayType == WEEKEND_DAY) rank_add_day(i);
	}
	rank_apply_ranking();

    // Count the days below the threshold
	for (int i = Hydrograph.StartIndex; i < Hydrograph.StopIndex; i++)
	{
		if (Day[i].IsDryDay)
		{
			if (Day[i].DayType == WEEK_DAY)
			{
				Hydrograph.NumberOfDryWeekdays++;
				Hydrograph.SumOfDryWeekdays += Day[i].Ave_DailyDeltaQ;
			}
			else
			{
				Hydrograph.NumberOfDryWeekendDays++;
				Hydrograph.SumOfDryWeekendDays += Day[i].Ave_DailyDeltaQ;
			}
		}
	}

	// Dry Weekday and Dry Weekend Day DeltaQ
	Hydrograph.AveDryWeekdayDeltaQ    = Hydrograph.SumOfDryWeekdays / Hydrograph.NumberOfDryWeekdays;
	Hydrograph.AveDryWeekendDayDeltaQ = Hydrograph.SumOfDryWeekendDays / Hydrograph.NumberOfDryWeekendDays;

	// Average Dry Day DeltaQ is based on a full week: 5 weekdays and 2 weekend days
	Hydrograph.AveDryDayDeltaQ = ((Hydrograph.AveDryWeekdayDeltaQ * 5.0) + (Hydrograph.AveDryWeekendDayDeltaQ * 2.0 )) / 7.0;
}

static void Step3(void)
{

	Step3_FindMaxMinValues();

	Step3_AssignRank(20);  // Threshold is 20%

}

static void Step4(void)
{
	int i;
	int count = 0;
	float sum = 0;

    // Calculate Groundwater DeltaQ
	for (i = Hydrograph.StartIndex; i < Hydrograph.StopIndex; i++)
	{
		if (Day[i].IsDryDay)
		{
			sum += Day[i].Min_DailyDeltaQ;
			count++;
		}
	}

	Hydrograph.GroundwaterDeltaQ = sum / ((float)count);

	Hydrograph.BaseDeltaQ = Hydrograph.AveDryDayDeltaQ - Hydrograph.GroundwaterDeltaQ;

	Hydrograph.BaseFlow_GPD   = Hydrograph.input.AveWaterUsage_GPD * Hydrograph.input.Population;
	Hydrograph.BaseFlow_GPM   = Hydrograph.BaseFlow_GPD / 1440.0;
	Hydrograph.GPM_per_DeltaQ = Hydrograph.BaseFlow_GPM / Hydrograph.BaseDeltaQ; // This is used to convert individual logged data points DeltaQ's to Flow.


}

void Hydro_PerformDecomp(HYDRO_DECOMP_INPUT *p)
{
  Hydrograph.input = *p;

  Step1();
  //Step2();  // included in Step1
  Step3();
  Step4();
}

static int decomp_calc_needed = 1;  // at power on, a calculation is required

void Hydro_PerformStandardDecomp(void)
{
	HYDRO_DECOMP_INPUT test;

	test.FirstLogAddr = log_FindFirstEntry();
	test.LastLogAddr  = log_FindLastEntry();

	test.VerticalMount = MainConfig.vertical_mount;
	test.Pipe_ID       = MainConfig.pipe_id;
	test.Population    = MainConfig.population;
	test.AveWaterUsage_GPD = 182;  // US National Average is 182 GPD per ?? person ??

	Hydro_PerformDecomp(&test);

}

void Hydro_NewDataLogged(void)
{
	decomp_calc_needed = 1;  // trigger a decomp calculation at the next request
}

float Hydro_CalcFlowFromDeltaQ(float qratio)
{
	if (decomp_calc_needed)
	{
		Hydro_PerformStandardDecomp();
		decomp_calc_needed = 0;
	}

	return qratio * Hydrograph.GPM_per_DeltaQ;
}

float Hydro_CalcNormalizedDeltaQ(float qratio)
{
	if (decomp_calc_needed)
	{
		Hydro_PerformStandardDecomp();
		decomp_calc_needed = 0;
	}

	return qratio / Hydrograph.AveDryDayDeltaQ;
}

void Hydro_ProcessImportedData(const TCHAR * ImportedFilename)
{
	uint32_t tickstart;  // When the command TX was started
	uint32_t duration;   // Duration of the last command (time from TX to RX complete)

	tickstart = HAL_GetTick();

	Hydro_PerformStandardDecomp();

	duration = HAL_GetTick() - tickstart;

	Print_CSV_File(ImportedFilename);  // output to SD card for development

	// kludge for breakpoint
	if (duration > 0)
		duration = 1;
}





void qratio_test(void)
{

	FRESULT fr;
	FIL fOutput;
	char fname[150];

	UINT sdCardBytes;
	char txt[50];
	char f_str[F_TO_STR_LEN];


	float qratio;
	float level;
	float pipe_id = 8.0;
	float reverse_level;

	sprintf(fname,"DeltaQ.csv");

    // Open file
    fr = f_open(&fOutput, fname, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) return;

	sprintf(txt, "pipe_id,%s\n\n", f_to_str_4dec(f_str, pipe_id));
	f_write(&fOutput, txt, strlen(txt), &sdCardBytes);

    sprintf(txt, "level,DeltaQ,level2\r");
    f_write(&fOutput, txt, strlen(txt), &sdCardBytes);

	for (level = 0.5; level < 7.9; level += 0.5)
	{
		qratio = Delta_Q(pipe_id, level);

		reverse_level = Get_Level_From_Delta_Q(pipe_id, qratio);


		sprintf(txt, "%s,", f_to_str_4dec(f_str, level));
		f_write(&fOutput, txt, strlen(txt), &sdCardBytes);

		sprintf(txt, "%s,", f_to_str_4dec(f_str, qratio));
		f_write(&fOutput, txt, strlen(txt), &sdCardBytes);

		sprintf(txt, "%s\n", f_to_str_4dec(f_str, reverse_level));
		f_write(&fOutput, txt, strlen(txt), &sdCardBytes);



	}

	f_close(&fOutput);

}

