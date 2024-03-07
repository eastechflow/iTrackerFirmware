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

#include "math.h"
#include "Log.h"
#include "time.h"
#include "Common.h"
#include "DeltaQ.h"
#include "Log.h"
#include "time.h"
#include "WiFi.h"
#include "quadspi.h"
#include "rtc.h"


float RadiansToDegrees(float radians) {
	return radians * (180 / pi);
}

float DegreesToRadians(float degrees) {
	return degrees * (pi / 180);
}




static float circWettedperimeter(float db1ID, float dblLevel) {
	float result = 0;

	float dblCircumference;
	float dblRadious;

	dblCircumference = pi * db1ID;
	dblRadious = 0.5 * db1ID;

	if (dblLevel < dblRadious) {
		result = dblCircumference * (acos((dblRadious - dblLevel) / (dblRadious)) / pi);
	} else {
		result = dblCircumference - (dblCircumference * (acos((dblRadious - (db1ID - dblLevel)) / dblRadious) / pi));
	}

	return result;

}

static float circWettedArea(float db1ID, float dblLevel) {
	float result;
	float dblRadious;
	dblRadious = 0.5 * db1ID;

	result = (0.5
	    * ((dblRadious * (pi / 90 * dblRadious * RadiansToDegrees(acos(1 - (dblLevel / dblRadious)))))
	        - ((2 * sqrt(dblLevel * (db1ID - dblLevel))) * (dblRadious - dblLevel))));

	return result;
}


float Delta_Q(float PipeID, float LevelWet)
{
	float dCircleWettedAreaDry;
	float dCircleWettedAreaWet;
	float dCircleWettedPermDry;
	float dCircleWettedPermWet;
	float AreaDiff;
	float WetDry;
	float result;
	float LevelDry = 0.1;  // zg - this is always a constant.  Therefore, all derived values are also constant.  No need to waste energy re-computing for each data point.
	float DryRatio;



	// zg - hoisted 4-28-2020
	if (LevelWet > PipeID)	 LevelWet = PipeID;
	if (LevelWet < LevelDry) LevelWet = LevelDry;

	//uint32_t tickstart;
	//uint32_t duration;
	//tickstart = HAL_GetTick();

	// zg - These three values are constant --- no need to constantly compute.  But, computation takes less than 1 ms, so...
	dCircleWettedAreaDry = circWettedArea(PipeID, LevelDry);
	dCircleWettedPermDry = circWettedperimeter(PipeID, LevelDry);
	DryRatio = dCircleWettedAreaDry / dCircleWettedPermDry;

	// kludge for breakpoint to read duration
	//duration = HAL_GetTick() - tickstart;
	//if (duration > 0) duration = 1;

	dCircleWettedAreaWet = circWettedArea(PipeID, LevelWet);
	dCircleWettedPermWet = circWettedperimeter(PipeID, LevelWet);

#if 0
	// zg - Error detected 4-28-2020.  This code snippet should be hoisted up, before the LevelWet is used in calculations.  It has no effect here.
	if (LevelWet > PipeID)	 LevelWet = PipeID;
	if (LevelWet < LevelDry) LevelWet = LevelDry;
#endif

	AreaDiff = dCircleWettedAreaWet / dCircleWettedAreaDry;
	WetDry = (dCircleWettedAreaWet / dCircleWettedPermWet) / (DryRatio);

	result = pow(WetDry, 0.6666666666666667); //  0.6666666666666667);// (2 / 3));
	result *= AreaDiff;
	return result;
}

#if 0

float Get_Level_From_Delta_Q(float PipeID, float DeltaQ)
{
	// Assert:  level = vertical_mount - distance
	//
	float level;
	float delta_q_trial;
	float difference;
	float prev_difference = -1.0;

	// brute force linear search
	for (level = 0.02; level < PipeID; level += 0.01)
	{
		delta_q_trial = Delta_Q(PipeID, level);
		if (DeltaQ > delta_q_trial)
			difference = DeltaQ - delta_q_trial;
		else
			difference = delta_q_trial - DeltaQ;

		if (prev_difference != -1.0)
		{
			float epsilon;

			// See if we are close enough
			if (prev_difference >= difference)
				epsilon = prev_difference - difference;
			else
				epsilon = difference - prev_difference;

			if (epsilon <= 0.01) return level;  //Criterion is 0.01   e.g. 1%

		}
		// keep going
		prev_difference = difference;
	}
	// If we get here, we did not converge
	return 0.0;



}

#endif

#if 0

dq_values_t DQ;

int8_t DeltaQ_StoreValues(void) {
	int8_t status = 0;
	status = BSP_QSPI_Erase_Sector(DQ_VALUES);
	DQ.checksum = 0;
	DQ.checksum = GetChkSum((uint8_t *) &DQ, sizeof(DQ));
	BSP_QSPI_Write((uint8_t *) &DQ, DQ_VALUES, sizeof(DQ));
	return status;
}

int8_t DeltaQ_LoadValues(void) {
	uint16_t SaveChkSum;
	BSP_QSPI_Read((uint8_t *) &DQ, DQ_VALUES, sizeof(DQ));
	SaveChkSum = DQ.checksum;
	DQ.checksum = 0;
	DQ.checksum = GetChkSum((uint8_t *) &DQ, sizeof(DQ));
	if (DQ.checksum != SaveChkSum)
		Delta_Q_Reset();
	return 0;
}


int8_t DeltaQ_reCalculate(void)
{

	uint8_t buff[256];
	uint32_t addr;
	uint32_t addr2;
	uint32_t tmp32r;
	uint32_t tmp32s;
	uint8_t cont = 1;
	uint8_t first = 1;

	Delta_Q_Reset();
	addr = LOG_BOTTOM;
	while (cont)
	{
		//Backup 4K sector
		tmp32r = addr;
		tmp32s = RECALC_TEMP_STORE;

		BSP_QSPI_Erase_Sector(RECALC_TEMP_STORE);

		for (int i = 0; i < 16; i++)
		{
			BSP_QSPI_Read(buff, tmp32r, 256);
			BSP_QSPI_Write(buff, tmp32s, 256);
			tmp32r += 256;
			tmp32s += 256;
		}

		//Erase copied sector
		BSP_QSPI_Erase_Sector(addr);

		LogTrack.last_sector_erased = addr >> 12;  // zg - why change this variable here?

		//Copy back through recalc
		addr2 = addr + 0x1000;   // zg this looks wrong....
		if (addr2 >= LogTrack.next_logrec_addr)
		{
			addr2 = LogTrack.next_logrec_addr;
			cont = 0;
		}

		tmp32r = RECALC_TEMP_STORE;
		if (first)
		{
			log_readstream_ptr(&CurrentLog, tmp32r);
			log_write_recalc_init(CurrentLog.datetime);
			addr += LOG_SIZE;
			tmp32r += LOG_SIZE;
			first = 0;
		}

		while (addr < addr2)
		{
			log_readstream_ptr(&CurrentLog, tmp32r);
			log_write(CurrentLog.datetime);
			addr += LOG_SIZE;
			tmp32r += LOG_SIZE;
		}

		if (addr >= LogTrack.next_logrec_addr)
		{
			cont = 0;
		}
	}

	return 0;
}


int8_t Delta_Q_Reset(void)
{
	memset(&DQ, 0, sizeof(DQ));
	DeltaQ_StoreValues();
	return 0;
}


int8_t Delta_Q_AddDaily(float dayAvg, float dayLow)
{
	int8_t s = 0;
	uint8_t search = 1;
	while (search && (s < 30)) {
		if (DQ.DQ_Daily[s][0] > 0) {
			if (dayAvg > DQ.DQ_Daily[s][0]) {
				s++;
			} else {
				search = 0;
			}
		} else
			search = 0;

	}
	if (s < 30) {
		if (s < 29) {
			for (int i = 28; i >= s; i--) {
				DQ.DQ_Daily[i + 1][0] = DQ.DQ_Daily[i][0];
				DQ.DQ_Daily[i + 1][1] = DQ.DQ_Daily[i][1];
			}
		}
		DQ.DQ_Daily[s][0] = dayAvg;
		DQ.DQ_Daily[s][1] = dayLow;
	}

	DQ.DQ_Days++;
	if (DQ.DQ_Days > 30)
		DQ.DQ_Days = 30;
	Delta_Q_getBaseLevels();
	DeltaQ_StoreValues();
	return 0;
}

int8_t Delta_Q_getBaseLevels(void) {
	uint8_t s, i;
	if (DQ.DQ_Days > 0) {
		DQ.VolumeRatio = 0;
		DQ.GroundWater = 0;
		if (DQ.DQ_Days > 14)
			s = DQ.DQ_Days * 0.2;
		else if (DQ.DQ_Days > 3)
			s = 3;
		else
			s = DQ.DQ_Days;

		for (i = 0; i < s; i++) {
			DQ.VolumeRatio += DQ.DQ_Daily[i][0];
			DQ.GroundWater += DQ.DQ_Daily[i][1];
		}

		DQ.GroundWater /= s;
		DQ.VolumeRatio /= s;
		return 1;
	}
	return 0;
}


int8_t Delta_Q_getFlow(void) {
	float temp = 0.1;
	if (DQ.VolumeRatio)
		temp = DQ.VolumeRatio;
	DQ.FlowRatio = (MainConfig.population * 182 / 1440) / temp;
	return 0;
}




float DDdq_gwpnt(void)
{
	// Combined Dry Day DeltaQ and Groundwater routine
	// scans the logger records and finds the lowest single record (minus filtering)
	// and sets that to be groundwater deltaQ.
	//
	// In addition, it averages all the logged DeltaQ values for each day over all the
	// days. The lowest day average will be the Dry Day deltaQ DDdq
	//
	log_t tmplog;
	struct tm tmpdate;
	uint32_t tmpsec;
	uint32_t currentSec;
	uint32_t logptr;
	float tmpfloat = 0;
	float gwater_point = 99999999;
	float DDdq_value = 0;
	uint16_t tmplen = 0;
	uint8_t days = 7;			// 7 days for a default scan
	uint32_t LoggedLength = 24 * days;		// 24 hour x 7 days

	uint8_t avgcnt = 0;


	currentSec = rtc_get_time();
	tmpsec = rtc_get_time() - (days * 86400);	// 86400 seconds per day
	if (tmpsec < LogTrack.start_datetime)
	{
		tmpsec = LogTrack.start_datetime;
		logptr = LOG_BOTTOM;
	}
	else
	{
		logptr = LogTrack.next_logrec_addr - (days * 86400 / MainConfig.log_interval * LOG_SIZE);
		if (logptr < LOG_BOTTOM)
			logptr = LOG_BOTTOM;
	}


	sec_to_dt(&tmpsec, &tmpdate);

	for (int i = LoggedLength; i > 0; i--)
	{
		log_readstream_ptr(&tmplog, logptr);

		if (tmplog.datetime < currentSec)
		{
			if (tmplog.datetime < tmpsec)
			{
				tmpfloat += tmplog.qratio;
				if (tmpfloat <= gwater_point)
				{
					if (tmpfloat)
					{
						gwater_point = tmpfloat;
						avgcnt++;
					}
				}
				else
				{
					if (avgcnt)
					{
						tmpfloat /= avgcnt;

						if (tmpfloat <= DDdq_value)
							DDdq_value = tmpfloat;

						sec_to_dt(&tmpsec, &tmpdate);
						avgcnt = 0;
						tmplen++;
						tmpfloat = 0;
					}
					else
					{
						tmpfloat += tmplog.qratio;
						avgcnt++;

						sec_to_dt(&tmplog.datetime, &tmpdate);
						tmpdate.tm_min = 0;
						tmpdate.tm_sec = 0;

						dt_to_sec(&tmpdate, &tmpsec);
					}
					tmpsec += 86400;
				}

			}

		}
		logptr += LOG_SIZE;
		if (logptr >= LogTrack.next_logrec_addr)
		{
			break;
		}
	}
	return DDdq_value;
	// now actual DryDay  Groundwater will be used later to compute DQpop which
	// is used for Flow Calculations and graphing. Flow Calculation uses population
	// settings and DQpop to make a FLOW CONVERSION FACTOR. That factor is multiplied
	// by the graphed Volume numbers to create Flow Numbers
}
#endif

