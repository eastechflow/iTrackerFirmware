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

#ifndef COMMON_H_
#define COMMON_H_

#include "stdint.h"
#include "time.h"

//#define iTRACKER_TYPE "-WIFI-EA" //Wifi-External Antenna
//#define iTRACKER_TYPE "-WIFI-IA"
//#define iTRACKER_TYPE "-ATT-IA"
#define iTRACKER_TYPE "-VER-IA"    // Verizon, Internal Antenna

#define pi		3.14159265358979

#define SEC_30DAYS 2592000
#define CLK_FORMAT RTC_FORMAT_BIN// );RTC_FORMAT_BCD

#define OFF 	0
#define ON 		1

#define NONE 	0
#define DOWN 	1
#define UP 		2



#define NO_ALARM        0
#define ALARMLEVEL1 	1
#define ALARMLEVEL2 	2


//#define ALARMLEVEL3 	3
//#define ALARMBATT 		4

extern uint8_t USBwifi_mirror;
extern uint8_t USB_Received;
extern uint8_t USB_wifi_CMD;
extern uint8_t USBwifi_status;
extern uint8_t USBmode;


#if 0
typedef struct {
	char site_name[20];
	float val;						// Battery?
	char serial_number[10];
	float wd_current;
	float we_current;
	float setLevel;
	char AddText[24];				// For alarms?
	char PhoneNum[6][16];
	uint32_t log_interval;
	char units;
	char MAC[18];
	float pipe_id;
	float vertical_mount;
	char damping;
	char SSID[40];
	uint32_t population;
	char PassKey[12];
	float AlertLevel1;
	float AlertLevel2;
	float ClearLevel;
	float AlarmBatt;
	uint8_t WifiConfigured;		// Has the wifi had an initial configuration?
	uint8_t CellConfigured;		// Has the cell had an initial configuration?
} jason_config_t;
#endif

// Comm_Mode Bit definitions
// MSB 7  WiFi Configured
//     6  Cell Configured
//     5  WiFi Internal Antenna (default is External)
//     4  iTracker (0) or FirstResponseIQ (1)
//     3  BLE name (0 = from hardware; 1 = override as iTracker)
//     2  reserved. set to 0
//     1  reserved. set to 0
//     0  reserved. set to 0
//
#define IsWiFiConfigured()     (MainConfig.Comm_Mode & 0x80)   // appears to be unused ?
#define IsCellEnabled()        (MainConfig.Comm_Mode & 0x40)
#define IsWiFiInternalAnt()    (MainConfig.Comm_Mode & 0x20)   // 0 = external; 1 = internal
#define IsFirstResponse()      (MainConfig.Comm_Mode & 0x10)   // 0 = iTracker; 1 = FirstResponse
#define IsBLENameOverride()    (MainConfig.Comm_Mode & 0x08)   // 0 = hardware; 1 = iTracker

#define SetWiFiConfigured()    (MainConfig.Comm_Mode |= 0x80)
#define SetCellEnabled()       (MainConfig.Comm_Mode |= 0x40)
#define SetWiFiInternalAnt()   (MainConfig.Comm_Mode |= 0x20)
#define SetFirstResponse()     (MainConfig.Comm_Mode |= 0x10)
#define SetBLENameOverride()   (MainConfig.Comm_Mode |= 0x08)

#define ClearWiFiConfigured()  (MainConfig.Comm_Mode &= 0x7F)
#define ClearCellEnabled()     (MainConfig.Comm_Mode &= 0xBF)
#define ClearWiFiInternalAnt() (MainConfig.Comm_Mode &= 0xDF)
#define ClearFirstResponse()   (MainConfig.Comm_Mode &= 0xEF)
#define ClearBLENameOverride() (MainConfig.Comm_Mode &= 0xF7)

#define ClearUnusedFlags()     (MainConfig.Comm_Mode &= 0xF8)

//#define CONFIGURED_AS_ITRACKER    1
//#define CONFIGURED_AS_SEWERWATCH  2
//#define CONFIGURED_AS_SMARTFLUME  3  // SmartFlume

int get_configuration(void);

#define UNITS_METRIC  1   // Note: this value was stored in flash and should not be changed --- it is different than the value sent by the Cell protocol
#define UNITS_ENGLISH 2   // Note: this value was stored in flash and should not be changed --- it is different than the value sent by the Cell protocol

// Hardware configuration bits, stored in wifi_wait_connect:
//
// MSB 31..28  Board
//     27..24  Cell Module
//     23..20  Cell UART
//     19..16  WiFi Module
//     15..12  WiFi UART
//     11..08  Reserved for future use set to 0
//
// Previous value of wifi_wait_connect was set to 60.
// If set to 61, then Cell and WiFi bits are available.
//     7  previous value for wifi_wait_connect. set to 0
//     6  previous value for wifi_wait_connect. set to 0
//     5  previous value for wifi_wait_connect. set to 1
//     4  previous value for wifi_wait_connect. set to 1
//     3  previous value for wifi_wait_connect. set to 1
//     2  previous value for wifi_wait_connect. set to 1
//     1  previous value for wifi_wait_connect. set to 0
//     0  previous value for wifi_wait_connect. set to 0
//

#define HardwareBitsSet()      ((MainConfig.wifi_wait_connect & 0x000000FF) == 61)
#define NoHardwareBitsSet()    ((MainConfig.wifi_wait_connect & 0x000000FF) == 60)
#define SetHardwareBitsFlag()   (MainConfig.wifi_wait_connect = (MainConfig.wifi_wait_connect & 0xFFFFFF00) | 61 )

//#define GetBoard()          ((MainConfig.wifi_wait_connect & 0xF0000000)>>28)
//#define SetBoard(x)          (MainConfig.wifi_wait_connect = (MainConfig.wifi_wait_connect & 0x0FFFFFFF) | ((x) << 28))
#define GetCellModule()     ((MainConfig.wifi_wait_connect & 0x0F000000)>>24)
#define SetCellModule(x)     (MainConfig.wifi_wait_connect = (MainConfig.wifi_wait_connect & 0xF0FFFFFF) | ((x) << 24))
#define GetRTCBatt()        ((MainConfig.wifi_wait_connect & 0x00F00000)>>20)
#define SetRTCBatt(x)        (MainConfig.wifi_wait_connect = (MainConfig.wifi_wait_connect & 0xFF0FFFFF) | ((x) << 20))
#define GetWiFiModule()     ((MainConfig.wifi_wait_connect & 0x000F0000)>>16)
#define SetWiFiModule(x)     (MainConfig.wifi_wait_connect = (MainConfig.wifi_wait_connect & 0xFFF0FFFF) | ((x) << 16))
#define GetWiFiUart()       ((MainConfig.wifi_wait_connect & 0x0000F000)>>12)
#define SetWiFiUart(x)       (MainConfig.wifi_wait_connect = (MainConfig.wifi_wait_connect & 0xFFF0FFFF) | ((x) << 12))

//iTrackerGen4 config file:
//BOARD,0,                    //0=iTrackerGen4, 1=iTrackerGen5  THIS CAN BE DETECTED VIA MCU CHIP ID
//CELL_MOD,1,                 //0=NONE, 1=TELIT, 2=XBEE3, 3=LAIRD
//CELL_UART,1,                //1=UART1 (default)
//WIFI_MOD,1,                 //0=NONE, 1=ZENTRI (default), 2=Reserved for ESP32_WIFI, 3=ESP32_BT, 4=Reserved for LAIRD_BT
//WIFI_UART,2,                //2=UART2 (default)

//iTrackerGen5 config file:
//BOARD,1,                    //0=iTrackerGen4, 1=iTrackerGen5 THIS CAN BE DETECTED VIA MCU CHIP ID
//CELL_MOD,3,                 //0=NONE, 1=TELIT, 2=XBEE3, 3=LAIRD
//CELL_UART,1,                //1=UART1 (default)
//WIFI_MOD,3,                 //0=NONE, 1=ZENTRI, 2=Reserved for ESP32_WIFI, 3=ESP32_BT (default), 4=Reserved for LAIRD_BT
//WIFI_UART,2,                //2=UART2 (default)



#define RTC_BATT_RECHARGE_OFF      0
#define RTC_BATT_RECHARGE_ON_1_5   1


#define iTRACKER_GEN_4  0
#define iTRACKER_GEN_5  1

#define CELL_MOD_NONE   0
#define CELL_MOD_TELIT  1
#define CELL_MOD_XBEE3  2
#define CELL_MOD_LAIRD  3

#define CELL_UART1      1

#define WIFI_MOD_NONE          0
#define WIFI_MOD_ZENTRI        1
#define WIFI_MOD_ESP32_WIFI    2  // not implemented yet
#define WIFI_MOD_ESP32_BT      3
#define WIFI_MOD_LAIRD_BT      4  // not implemented yet

#define WIFI_UART2       2


typedef struct {
	char			site_name[20];
	float 			val;
	char			serial_number[10];
	float 			wd_current;
	float 			we_current;
	float 			setLevel;
	char			AddText[24];    // zg - Is this ever used for anything?
	char			PhoneNum[6][16];  // zg - never used, initialized to zero
	uint16_t		checksum;
	uint32_t 		log_interval;  // in seconds
	uint32_t 		wifi_wakeup_interval;
	uint32_t		wifi_wait_connect;      // unused, set to 60.  If 61 or higher, in use for board and module bit flags.
	uint32_t		wifi_browser_connect;   // unused, set to 180
	uint32_t		max_connect_time;       // unused, set to 600
	uint32_t		inactivity_timeout;     // unused, set to 180
	uint32_t		battery_reset_datetime; // previously wifi_livesend_interval set to 5
	char			powerdown_mode_normal;  // unused, set to 1
	char 			units;   // 1 = Metric, 2 = English
	char			MAC[20];  // mm:MM:mm:MM:mm:MM  18 with null term
#if 0 // original
	char			ChID[8];  // unused, set to zero
#else
	uint8_t			ForceType;
	uint8_t			spare_2;
	uint8_t			spare_3;
	uint8_t			spare_4;
	uint8_t			spare_5;
	uint8_t			spare_6;
	uint8_t			spare_7;
	uint8_t			spare_8;
#endif
	float			pipe_id;
	float			vertical_mount;
	char			damping;
	char			SSID[20];
	uint32_t		sensor_freq;  // unused, set to 66000
	uint32_t		population;
	char			PassKey[12];
	float 			AlertLevel1;  // saved in English units; 0 means alert1 is off (and, also, that alert2 is off, because there is no "reset" event
	float 			AlertLevel2;  // saved in English units; 0 means alert2 is off
	float 			ClearLevel;   //
	float 			AlarmBatt;    // possibly unused, set to 20
	uint8_t			Comm_Mode;    // bit field.  See definition.
}config_t;




#define SENSOR_STRUCT_V0  0    // for pga460
typedef struct {
	uint16_t struct_version;
	uint16_t checksum;
	uint8_t  grid[128];
} SensorConfig_t;

// CELLULAR

// Status messages:
// Data sent on mm/dd/yyyy hh:mm:ss   (33)
// Ping sent on mm/dd/yyyy hh:mm:ss   (33)
// Alert sent on mm/dd/yyyy hh:mm:ss  (34)
// Error on mm/dd/yyyy hh:mm:ss       (29)
// No data sent yet                   (17)
//

// https://1n0jzsagqj.execute-api.us-west-2.amazonaws.com/prod/echo  URL is 46

#define CELL_APN_MAX    50
#define CELL_STATUS_MAX 50              // mm/dd/yyyy hh:mm:ss
#define CELL_IP_MAX     80              // 255.255.255.255 or possibly a very long URL
#define CELL_PORT_MAX    8              // 2222

typedef struct {
	uint32_t struct_version;
	uint32_t ctx;				        // Override Context (Carrier)  0 = no override configured; 1 = ATT; 3 = Verizon
	char     apn[CELL_APN_MAX];	        // Override Carrier APN - normally set by hw found and configured at factory - only need 28, can reduce if needed
	char     status[CELL_STATUS_MAX];	// Status of last call made, or most recent error
	uint32_t SampleCnt;		            // Number of samples to send.  This depends on sample frequency...there is a fixed number per phone call...
	uint32_t spare1;                    // prior to 5.0.236 this was last alert level reported
	uint32_t spare2;
	float    spare3;
	char     error[CELL_STATUS_MAX];    // last error reported
	char     ip[CELL_IP_MAX];
	char     port[CELL_PORT_MAX];
} CellConfig_t;

extern char s2c_cell_status[CELL_STATUS_MAX + CELL_STATUS_MAX];  // for state machine status updates

void set_default_cell_ip_and_port(void);

#define GEN4_DEVID   1045     // empirically determined
#define GEN5_DEVID   1121     // empirically determined

#define isGen4()   (stm32_DEVID == GEN4_DEVID)
#define isGen5()   (stm32_DEVID == GEN5_DEVID)

#define BOARD_ID_GEN4  0
#define BOARD_ID_510   1
#define BOARD_ID_511   2
#define BOARD_ID_SEWERWATCH_1   3

void Gen5_get_board_id_voltage(float * result);
void get_board_id(void);

extern uint32_t stm32_DEVID;
extern uint32_t BoardId;


extern config_t MainConfig;
extern int ProductType;
extern SensorConfig_t SensorConfig;
extern int pga460_burst_listen_delay_ms;
extern CellConfig_t CellConfig;
extern uint8_t LogEnable;
extern uint32_t LogTime;
extern uint32_t TimeAtPowerOn;
extern uint32_t TimeAtWakeUp;
extern uint32_t PrevTimeAtWakeUp;
extern uint32_t TimeWiFiPoweredOn;
extern uint32_t IsFirstWakeCycle;
extern uint32_t WifiIsAwake;				// Tracking Wifi awake status.
extern uint32_t Wifi_Sleep_Time;



extern float    LastGoodDegF;
extern float    LastGoodMeasure;
extern uint32_t Actual_Blanking;
extern uint16_t Actual_Gain;

// USB
#define USB_INBUF_SIZE 256
extern char usbbuf[USB_INBUF_SIZE];
extern unsigned char usb_head_ptr;
extern unsigned char usb_tail_ptr;

int8_t USB_send_buffer(const char * txBuff, uint32_t txSize);
uint32_t TimeDiff(uint32_t start, uint32_t curren);

void convfloatstr_1dec(float val, int * dec1, int * dec2);
void convfloatstr_2dec(float val, int * dec1, int * dec2);
void convfloatstr_3dec(float val, int * dec1, int * dec2);
void convfloatstr_4dec(float val, int * dec1, int * dec2);
#define F_TO_STR_LEN  16
char * f_to_str_0dec(char * str, float value);  // suggest str[16];
char * f_to_str_2dec(char * str, float value);  // suggest str len of at least 16
char * f_to_str_4dec(char * str, float value);  // suggest str len of at least 16

char * f_to_str_nn_nn(char * str, float value);  // suggest str[16];

#define TIME_STR_SIZE 22
char * tm_to_str(struct tm * tm_val, char * str);  // str must be at least 20 chars long
char * time_to_str(uint32_t time, char * str);  // str must be at least 20 chars long  "mm/dd/yyyy hh:mm:ss"
char * get_current_time_str(char *str);  // str must be at least 20 chars long  "mm/dd/yyyy hh:mm:ss"

uint16_t GetChkSum(uint8_t *DataStart, uint16_t Len);
int isErased(uint8_t * pSectorData, int len);

// Main Configuration
#define DO_NOT_RESET_SERIAL_NUMBER  0
#define RESET_SERIAL_NUMBER         1

void   Reset_MainConfig(int ResetSerialNumber);
int    Load_MainConfig(void);  // returns 1 if an empty or corrupt flash was detected.  This can be used to signal other initialization.
void   Save_MainConfig(void);

void reset_to_factory_defaults(void);

// Sensor Configuration
void Reset_SensorConfig(void);
void Load_SensorConfig(void);
void Save_SensorConfig(void);

// Cellular Configuration
void Reset_CellConfig(void);
void Load_CellConfig(void);
void Save_CellConfig(void);

extern int saveMainConfig;
extern int saveCellConfig;
extern int saveSensorConfig;

#define MAIN_LOOP_OK_TO_SLEEP   0
#define MAIN_LOOP_DO_NOT_SLEEP  1

// See MainConfig.ForceType
#define FORCE_TYPE_USE_HARDWARE   0
#define FORCE_TYPE_ITRACKER       1
#define FORCE_TYPE_FIRSTRESPONSE  2
#define FORCE_TYPE_SEWERWATCH     3
#define FORCE_TYPE_SMARTFLUME     4

// See ProductType global variable
#define PRODUCT_TYPE_ITRACKER      0
#define PRODUCT_TYPE_FIRSTRESPONSE 1
#define PRODUCT_TYPE_SEWERWATCH    2
#define PRODUCT_TYPE_SMARTFLUME    3
#endif
