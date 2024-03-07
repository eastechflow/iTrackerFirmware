/*
 * trace.h
 *
 *  Created on: Mar 12, 2020
 *      Author: Zachary
 */

#ifndef INC_TRACE_H_
#define INC_TRACE_H_

#define TRACE_BUFFER_SIZE  1024
extern char TraceBuffer[TRACE_BUFFER_SIZE];

void trace_Init(void);
void trace_AppendMsg(const char *Msg);
void trace_AppendChar(char ch);
void trace_AppendCharAsHex(char ch); // outputs xx,

void trace_sleep(void);
void trace_wakeup(void);
void trace_SaveBuffer(void);

void trace_TimestampMsg(const char *Msg);

void TraceSensor_AppendMsg(const char *Msg);  // Specialized trace function

void tx_msg(char * msg);

#endif /* INC_TRACE_H_ */
