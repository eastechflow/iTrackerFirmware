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

#ifndef _ZENTRI_H_
#define _ZENTRI_H_

#include "Common.h"


#define ZENTRI_RESET 0
#define ZENTRI_ERASE 1

#define ZENTRI_CMD_MODE 1
#define ZENTRI_STR_MODE 0

#define DO_NOT_LOAD_ZENTRI_FILES  0
#define LOAD_ZENTRI_FILES         1

void zentri_config(int LoadZentriFiles);

void zentri_factory_reset(char *mac_address, uint8_t bErase);

void zentri_set_mode(uint8_t val);	// Command mode (AT commands) or String mode (data)
void zentri_enter_command_mode(void);
int  zentri_send_command_timeout(char * txBuff, uint32_t timeout_ms);
int  zentri_send_command(char * txBuff); // For modem config and sending AT commands.
void zentri_send_command_no_wait(char * txBuff);
void zentri_get_response(char * text, int maxlen);  // use sizeof(text) for maxlen
void zentri_reset(void);				// Hard reboot
//void zentri_update_SSID(void);
int8_t zentri_connected(void);		// Is the Zentri GPIO indicating a wifi connection?
int8_t zentri_socket_open(void);    // Is the Zentri GPIO indicating a user/browser connection?
int  zentri_wait_for_response(uint32_t timeout_ms);
//void zentri_get_extra_bytes_response(char * text, int maxlen);  // use sizeof(text) for maxlen

void zentri_wakeup(void);
void zentri_sleep(void);

#endif
