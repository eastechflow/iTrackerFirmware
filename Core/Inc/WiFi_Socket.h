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

#ifndef _WIFI_SOCKET_H_
#define _WIFI_SOCKET_H_




// PAGES, ACTIONS
//#define CMD_NONE              0  // reserved for "no action"; do not use
#define CMD_LIVE_VALUES     1
#define CMD_AUTO_READINGS_LIVE_ONLY 2 // Automatic readings, live only (not written to log)
#define CMD_SLEEP			3
#define CMD_ID              4
#define CMD_SEND_CLOUD_PKT  5
#define CMD_SETTINGS        6
#define CMD_STAY_AWAKE      7
#define CMD_DOWNLOAD_DIAG   8
#define CMD_DOWNLOAD        9
//#define CMD_DATA_RESET      10
#define CMD_LOGS_PAGE	    11
//#define CMD_AVAIL           12
#define CMD_ALARM_PAGE      13
#define CMD_HISTORY_PAGE    14
#define CMD_LED2_ON         15
#define CMD_LED2_OFF        16
#define CMD_RPT_ACCUM_ON    17
#define CMD_RPT_ACCUM_OFF   18
#define CMD_SET_ACCUM_PCT   19   // shortcut:  19,pct in Admin text box  results in {"Command":"19,pct"} being sent to firmware

#define CMD_WIFI_CONFIG     21 // Trigger a full wifi config on next powerup.
#define CMD_RESET_DEFAULTS  22 // Reset default main config settings.

#define CMD_RTC_BATT_RECHARGE_OFF       23 // Set battery recharge to off
#define CMD_RTC_BATT_RECHARGE_ON_1_5K   24 //

#define CMD_CELL_NONE      30 // Sets cell module to NONE
#define CMD_CELL_TELIT     31 // Sets cell module to TELIT and performs a factory reset.
#define CMD_CELL_XBEE3     32 // Sets cell module to XBEE3 and performs a factory reset.
#define CMD_CELL_LAIRD     33 // Sets cell module to LAIRD and performs a factory reset.  {"comm":33}
//#define CMD_CELL_LAIRD_NZ  34 // Sets cell module to LAIRD, configures for New Zealand, and performs a factory reset.

#define CMD_CELL_LAIRD_ISIM  35  // internal SIM
#define CMD_CELL_LAIRD_ESIM  36  // external SIM
//#define CMD_CELL_LAIRD_ASIM  37  // auto select SIM
#define CMD_CELL_BAND      38 // Performs a cell band configure

#define CMD_CELL_PING      40 // Performs a power-on ping. Invoked from json {"comm":40}


#define CMD_BOOT_ESP32_UART4    224    // Puts ESP32 into bootload mode and bridges UART2 to UART4 (VEL1), must power-cycle to exit

#define CMD_999             999 // zg - this appears to be a "keep alive" artifact in the UI code from Jason

// Downloads
#define DL_TYPE_WIFI		1  // Download to disk
#define DL_TYPE_SDCARD		2  // Download to SD card

// Page control. In main.c
extern uint8_t  json_complete;
extern uint16_t json_cmd_id;
extern uint16_t json_sub_cmd_id;
extern char     json_cloud_packet_type;
extern uint32_t json_cloud_record_number;

extern uint8_t SleepDelayInMinutes;



//extern char DebugStringFromUI[];
//extern char DebugStringToSendBack[];

extern uint8_t CurrentGraphType;
extern uint8_t CurrentGraphDays;
extern uint32_t Download_startTime;
extern uint32_t Download_type; // Disk or SD Card

extern uint8_t wifi_report_seconds;




#define DEFAULT_SITE_NAME    "?????"
#define WIFI_SO_PASSKEY_VAL  "00000000"
#define WIFI_SO_SSID_VAL     "iTracker"

// Main method to send strings to WiFi input buffer.
// Can be AT commands or encoded binary data, for example.
// Make sure the Zentri buffer size matches the buffer sizes as configured.



// Results sent back to user for page interactions.
void wifi_send_live(void);
void wifi_send_logs(void);
int wifi_send_download(uint32_t DL_Type, uint32_t start_sec);
int wifi_send_download_diag(uint32_t DL_Type);
void wifi_send_history(uint8_t graphType, uint8_t time);
void wifi_send_settings(void);
void wifi_send_alarm(void);
void wifi_send_id(void);
void wifi_send_cloud_packet(char PacketType, uint32_t record_number);
void wifi_send_download_error_code(int code);

//void wifi_send_debug(void);
//void wifi_append_debug_string(char *msg);
//void wifi_empty_debug_string(void);

// Processes incoming user/browser requests.
void wifi_parse_incoming_JSON(void);

void wifi_process_json(void);

// Tracks transmission to coordinate sleep
void wifi_start_transmission(void);
void wifi_end_transmission(void);

void parse_datetime(char * strdate, struct tm * tm_st);
uint32_t parse_timestamp(char *value);

#endif
