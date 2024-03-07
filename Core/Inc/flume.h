/*
 * Flume.h
 *
 */

#ifndef INC_FLUME_H_
#define INC_FLUME_H_

#include "Log.h"

extern float flume_K_Factor[5];

float flume_calc_gallons(float K, log_t * pL1, log_t * pL2, float *pQ1, float *pQ2);

float flume_get_K_factor(float pipe_id);


#endif /* INC_FLUME_H_ */
