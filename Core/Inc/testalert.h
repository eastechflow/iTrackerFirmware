/*
 * testalert.h
 *
 *  Created on: May 22, 2020
 *      Author: Zachary
 */

#ifndef INC_TESTALERT_H_
#define INC_TESTALERT_H_

#define EVENT_NO_ACTION         0
#define EVENT_TRIGGER_ALERT_1   1
#define EVENT_TRIGGER_ALERT_2   2
#define EVENT_CLEAR_ALERT       3

void testalert_init(int TestAlerts);
float testalert_get_next_sim_value(void);
void testalert_verify_action(int ActualEvent, float ActualDistance);

int testalert_get_current_sim_point(void);

#endif /* INC_TESTALERT_H_ */
