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

#ifndef _WIFI_H_
#define _WIFI_H_

#include "Common.h"

#define WIFI_RESPONSE_COMPLETE          0
#define WIFI_RESPONSE_WAITING_FOR_RX    1
#define WIFI_RESPONSE_ERROR             2
#define WIFI_RESPONSE_TIMEOUT           3
#define WIFI_TX_TIMEOUT                 4




void wifi_config(void);  // Perhaps this should take the SSID as an argument ?  What about MAC address extracted from hardware?

int  uart2_tx(uint32_t txSize, uint8_t trace);

int  wifi_is_active(void);

int  wifi_wakeup(void); 			// Wake up from sleep, or after initial config.

void wifi_sleep(void);				// Go to low power sleep

int8_t wifi_chkkey (char * key);	// Validation function

void main_rtc_changed(void);
void main_extend_timeout(void);

#endif
