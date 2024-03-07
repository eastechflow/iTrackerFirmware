/*
 * s2c.h
 *
 *  Created on: APR 25, 2019
 *      Author: S2C
 */

#ifndef INC_S2C_H_
#define INC_S2C_H_

#include "log.h"


#define ACK_TIMEOUT            0  // keep this at zero
#define ACK_RECEIVED           1
#define ACK_RESEND             2
#define ACK_RESEND_VERBOSE     3



int s2c_ConfirmCellModemHardware(void);


void S2C_CELLULAR_LOG(log_t *pLogRec);   // latch alerts with each logged data point
int  S2C_CELLULAR_MAKE_CALL(void);
int  S2C_CELLULAR_SEND(void);  // send data via cellular
void S2C_TRIGGER_CELLULAR_SEND(void);  // send data via cellular

void S2C_POWERUP(void);
void S2C_POWERDOWN(void);
void S2C_TRY_BAND(int Band);





int s2c_timeslice(void);
int s2c_is_cell_active(void);

void s2c_trigger_confirm_cell_hw(void);
void s2c_trigger_factory_config_from_ui(void);
void s2c_trigger_cell_band_from_ui(uint16_t band);
void s2c_trigger_internal_SIM_from_ui(void);
void s2c_trigger_external_SIM_from_ui(void);
void s2c_trigger_power_on_ping_from_ui(void);
int  s2c_trigger_initial_power_on_ping(void);  // returns 1 if trigger, 0 if no trigger
void s2c_trigger_cellular_send(void);

void s2c_trigger_power_on(void);
void s2c_trigger_power_off(void);



int AT_OK(const char *Cmd, uint32_t Timeout_ms);

int s2c_Test(void);


#define S2C_STATUS_MAX 50   // See config


extern char s2c_imei[15+1];

#define S2C_USA          0
#define S2C_NEW_ZEALAND  1

void telit_reset(void);
void xbee3_reset(void);


//void laird_min_from_power_on(void);
//void laird_query_config(void);

//#define S2C_SERVER  1
//#define AWS_SERVER  2

// See also the following symbols in Common.h...maybe merge these someday...
//#define CELL_MOD_NONE   0
//#define CELL_MOD_TELIT  1
//#define CELL_MOD_XBEE3  2
//#define CELL_MOD_LAIRD  3

#define S2C_NO_MODEM      0
#define S2C_TELIT_MODEM   1
#define S2C_XBEE3_MODEM   2
#define S2C_LAIRD_MODEM   3


extern int s2c_CellModemFound;  // See values above
//extern int s2c_ConfirmCellModem;

extern uint32_t s2c_Wake_Time;


//extern int s2c_cell_power_is_on;


//uint32_t s2c_powerup_cell_hardware(void);
//int s2c_is_cell_powerup_complete(void);

void s2c_prep_settings_packet(char * outbuf);
uint32_t s2c_prep_log_packet(char * outbuf, char packet_id, uint32_t StartingLogRecNumber, uint32_t MaxArraySize);  // max array size is 12, but can request less, e.g. 1

void s2c_log_alert(char *desc, float distance, float level);

void s2c_check_for_alert(float distance);

extern int s2c_current_alarm_state;
extern int s2c_last_alarm_state_sent;
extern log_t * s2c_pAlertLogRec;

#endif /* INC_S2C_H_ */
