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

#ifndef SOURCES_FUNCTIONS_DELTAQ_H_
#define SOURCES_FUNCTIONS_DELTAQ_H_

float Delta_Q(float pipe_ID, float pipe_level);
float Get_Level_From_Delta_Q(float PipeID, float DeltaQ);

#if 0
typedef struct {
	float DQ_Daily[30][2];
	float VolumeRatio;
	float GroundWater;
	float FlowRatio;
	uint8_t DQ_Days;
	uint8_t comp;
	uint16_t checksum;
} dq_values_t;

extern dq_values_t DQ;

int8_t Delta_Q_Reset(void);

float  DDdq_gwpnt(void);
int8_t Delta_Q_AddDaily(float dayAvg, float dayLow);
int8_t Delta_Q_getBaseLevels(void);
int8_t Delta_Q_getFlow(void);
int8_t DeltaQ_StoreValues(void);
int8_t DeltaQ_LoadValues(void);
int8_t DeltaQ_reCalculate(void);
#endif

#endif
