/*
 * Flume.c
 *
 */

#include <math.h>
#include "flume.h"

// Flume   K Factor  Max Flow (gpm)  Max Head Rise (inches)
// 4"      2.88        90             5.84
// 6"      3.72       250             8.65
// 8"      4.21       550            12.17
// 10"     4.75      1000            15.54
// 12"     5.34      1500            18.00



float flume_K_Factor[5] = {
		2.88,
		3.72,
		4.21,
		4.75,
		5.34
};

#if 0
#define FLUME_4_INCH    0
#define FLUME_6_INCH    1
#define FLUME_8_INCH    2
#define FLUME_10_INCH   3
#define FLUME_12_INCH   4
static float flume_get_K_Factor(int FlumeId)
{
	switch (FlumeId)
	{
	case FLUME_4_INCH: return flume_K_Factor[0]; break;
	case FLUME_6_INCH: return flume_K_Factor[1]; break;

	case FLUME_8_INCH: return flume_K_Factor[2]; break;
	case FLUME_10_INCH: return flume_K_Factor[3]; break;
	case FLUME_12_INCH: return flume_K_Factor[4]; break;
	}

	return flume_K_Factor[2];  // default is 8 inch Flume
}
#endif

float flume_get_K_factor(float pipe_id)
{
	float K;

	// For SmartFlume
	switch ((int)pipe_id)
	{
	case 4:  K = flume_K_Factor[0]; break;
	case 6:  K = flume_K_Factor[1]; break;
	default:
	case 8:  K = flume_K_Factor[2]; break;
	case 10: K = flume_K_Factor[3]; break;
	case 12: K = flume_K_Factor[4]; break;
	}

	return K;
}

static float calc_gpm(uint32_t T1, float Q1, uint32_t T2, float Q2)
{
	float Ave_Q;
	float Delta_T;

	// Calculate total flow from Time1 to Time2:
	// Calculate Ave_Q
	// Calculate time difference in minutes
	// Calculate volume of flow as Delta_T * Ave_Q

	Ave_Q = (Q1 + Q2) / 2.0;  // in GPM

	Delta_T = T2 - T1;  // in seconds
	Delta_T /= 60.0;    // in minutes

	return Delta_T * Ave_Q;   // in gallons
}



float flume_calc_gallons(float K, log_t * pL1, log_t * pL2, float *pQ1, float *pQ2)  // computes volume of flow in gallons between two logged data points
{
	uint32_t T1;  // in seconds
	float    H1;  // Head rise (level) in inches
	float    Q1;  // in gpm


	uint32_t T2;  // in seconds
	float    H2;  // Head rise (level) in inches
	float    Q2;  // in gpm


	T1 = pL1->datetime;
	H1 = log_get_level(pL1);  // TODO - should negative levels be clamped to zero?  Should levels above pipe_id be clamped?
	Q1 = K * pow(H1, 1.95);

	T2 = pL2->datetime;
	H2 = log_get_level(pL2);  // TODO - should negative levels be clamped to zero?  Should levels above pipe_id be clamped?
	Q2 = K * pow(H2, 1.95);

	// report calculated Qs if requested
	if (pQ1)
	{
		*pQ1 = Q1;
	}
	if (pQ2)
	{
		*pQ2 = Q2;
	}

	return calc_gpm(T1, Q1, T2, Q2);
}

