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

#include "Common.h"
#include <stdlib.h>
#include <stdio.h>
#include "gpio.h"
#include "rtc.h"
#include "Sensor.h"
#include "quadspi.h"
#include "LTC2943.h"
#include "Gen4_max5418.h"
#include "Gen5_TempF.h"
#include "s25fl.h"
#include "s2c.h"
#include "factory.h"

__IO ITStatus UartReady = RESET;

uint8_t LogEnable = 1;

// Main configuration stored in secondary flash.
config_t MainConfig;
int ProductType = PRODUCT_TYPE_ITRACKER;

// Sensor configuration stored in secondary flash.
SensorConfig_t SensorConfig;
int pga460_burst_listen_delay_ms = 60;

// Cellular configuration stored in secondary flash.
CellConfig_t CellConfig;

char s2c_cell_status[CELL_STATUS_MAX + CELL_STATUS_MAX];  // for state machine status updates

// Log records.
log_t CurrentLog = { .datetime = 0, };
log_track_t LogTrack;
uint32_t    LogTrack_first_logrec_addr;

// USB Interface
uint8_t USBmode = 0;
uint8_t USBwifi_mirror = 1;
uint8_t USB_Received = 0;
uint8_t USB_wifi_CMD = 0;

char usbbuf[USB_INBUF_SIZE];
unsigned char usb_head_ptr = 0;
unsigned char usb_tail_ptr = 0;

char usbbuf2[USB_INBUF_SIZE];
unsigned char usb_head_ptr2 = 0;
unsigned char usb_tail_ptr2 = 0;

#if 0  // zg - not used
uint8_t usb_input(char * buf, uint32_t len) {
	unsigned char next;
	unsigned char cnt = 0;
	USB_Received = 1;
	while (len) {
		next = usb_head_ptr + 1; // do not really need next
		while (next == usb_tail_ptr) {
			if ((usb_head_ptr + 1) != usb_tail_ptr)
				break;
		} // need esc from here
		usbbuf[usb_head_ptr++] = buf[cnt];
		if (buf[cnt] == '\n') {
			USB_wifi_CMD = 1;
			wifi_send_string(&usbbuf[0]);
			usb_head_ptr = 0;
			usb_tail_ptr = 0;
		}
		cnt++;
		if (usb_head_ptr >= USB_INBUF_SIZE)
			usb_head_ptr = 0;
		len--;
	}
	return 0;
}


int8_t USB_send_buffer(const char * txBuff, uint32_t txSize) {
	uint8_t chk = 1;
	//HAL_Delay(100);

	if (txSize < (USB_INBUF_SIZE + 2)) {
		memcpy(usbbuf2, txBuff, txSize);
	}

	while (chk) {
		USBwifi_status = CDC_Transmit_FS((uint8_t*) usbbuf2, txSize);
		if (USBwifi_status != 0) {
			HAL_Delay(10);
			chk++;
			if (chk > 10)
				chk = 0;
		} else {
			chk = 0;
		}
	}
	usb_head_ptr = 0;

	if (USBwifi_status == 0)
		return 0;
	else
		return -1;
}

uint32_t TimeDiff(uint32_t start, uint32_t current) {
	uint32_t dif;

	if (current >= start) {
		dif = current - start;
	} else {
		dif = 0xffffffff - start + current;
	}

	return dif;
}

#endif


void convfloatstr_1dec(float val, int * dec1, int * dec2) {
	*dec1 = (int) val;
	*dec2 = (int) ((val - *dec1) * 10.0); // zg - note that this truncates, it does not round up
	if (*dec2 < 0)
		*dec2 = 0;
}

// Error from original rounding code deployed:
// *dec2 = (int) ((val - *dec1) * 100.0 + 0.5);  // rounds up
// Rounding code resulted
// error in converting 29.995 result is 29.100, which is not correct
// best to not round at all here



void convfloatstr_2dec(float val, int * dec1, int * dec2) {
	*dec1 = (int) val;
	*dec2 = (int) ((val - *dec1) * 100.0);  // truncates
	if (*dec2 < 0)
		*dec2 = 0;
}

void convfloatstr_3dec(float val, int * dec1, int * dec2) {
	*dec1 = (int) val;
	*dec2 = (int) ((val - *dec1) * 1000.0);  // truncates
	if (*dec2 < 0)
		*dec2 = 0;
}


void convfloatstr_4dec(float val, int * dec1, int * dec2)
{
	*dec1 = (int) val;
	*dec2 = (int) ((val - *dec1) * 10000.0);  // truncates
	if (*dec2 < 0)
		*dec2 = 0;
}

char * f_to_str_0dec(char * str, float value)  // suggest str[16];
{
	int dec1;
	int dec2;
	convfloatstr_1dec(value, &dec1, &dec2);
	if (dec2 > 5) dec1++;   // round up
	sprintf(str, "%d", dec1);
	return str;
}

char * f_to_str_2dec(char * str, float value)  // suggest str[16];
{
	int dec1;
	int dec2;
	convfloatstr_2dec(value, &dec1, &dec2);
	sprintf(str, "%d.%02d", dec1, dec2);
	return str;
}
char * f_to_str_nn_nn(char * str, float value)  // suggest str[16];
{
	int dec1;
	int dec2;
	convfloatstr_2dec(value, &dec1, &dec2);
	sprintf(str, "%02d.%02d", dec1, dec2);
	return str;
}
char * f_to_str_4dec(char * str, float value)  // suggest str[16];
{
	int dec1;
	int dec2;
	convfloatstr_4dec(value, &dec1, &dec2);
	sprintf(str, "%d.%04d", dec1, dec2);
	return str;
}

char * tm_to_str(struct tm * p, char * str)  // str must be at least 20 chars long
{

	sprintf(str, "%02d/%02d/%04d %02d:%02d:%02d",
             p->tm_mon, p->tm_mday, p->tm_year, p->tm_hour, p->tm_min, p->tm_sec);

	return str;
}


char * time_to_str(uint32_t time, char * str)  // str must be at least 20 chars long
{
	uint32_t tmpsec;
	struct tm tm_st;

	// convert time to struct
	tmpsec = time;
	sec_to_dt(&tmpsec, &tm_st);

	return tm_to_str(&tm_st, str);
}


char * get_current_time_str(char *str)  // str must be at least 20 chars long
{
	struct tm tm_st;

	rtc_get_datetime(&tm_st);

	return tm_to_str(&tm_st, str);

}




uint16_t GetChkSum(uint8_t *DataStart, uint16_t Len)
{
	uint16_t Result = 0;
	for (int i = 0; i < Len; i++)
		Result += DataStart[i];
	return Result;
}

#if 0 // zg - not used
// ALPHA NUMERIC REGEX: ^[a-zA-Z][a-zA-Z0-9]*$
int isValidAlpha(char * val) {
	int l = strlen(val);
	for (int i = 0; i < l; i++)  {
		int chr = val[i];
		if (!(
			((chr >= 65) && (chr <= 90)) || // CAPS A-Z
			((chr >= 97) && (chr <= 122))) ) { // LOWERS a-z
			return 0; // FALSE
		}
	}
	return 1; // TRUE
}

int isValidNumeric(char * val) {
	int l = strlen(val);
	for (int i = 0; i < l; i++)  {
		int chr = val[i];
		if (!((chr >= 48) && (chr <= 57))) { // NUMBERS 0-9
			return 0; // FALSE
		}
	}
	return 1; // TRUE
}
#endif

int isValidAlphaNumeric(char * val) {
	int l = strlen(val);
	for (int i = 0; i < l; i++)  {
		int chr = val[i];
		if (!(
			((chr >= 48) && (chr <= 57)) ||	// NUMBERS 0-9
			((chr >= 65) && (chr <= 90)) || // CAPS A-Z
			((chr >= 97) && (chr <= 122))) ) { // LOWERS a-z
			return 0; // FALSE
		}
	}
	return 1; // TRUE
}


int isErased(uint8_t * pSectorData, int len)
{
	int i;
	for (i=0;i<len;i++)
	{
		if (pSectorData[i] != 0xFF) return 0;
	}
	return 1;
}

static void SaveConfigData(uint8_t* pData, uint32_t WriteAddr, uint32_t Size)
{
    int8_t status;

    // Only allow flash writes when the battery capacity indicates it is safe to do so?  Or is all the other checks enough?
    // Still have to allow user to "reset" the battery when the percentage is low, so, higher-level checks are the only option?
    //ltc2943_get_battery_percentage();

    status = BSP_QSPI_Erase_Sector(WriteAddr);
	if (status)
	{
		_Error_Handler(__FILE__, __LINE__);
	}


	status = BSP_QSPI_Write(pData, WriteAddr, Size);
	if (status)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

}



// MAIN CONFIG

void Reset_MainConfig(int ResetSerialNumber)
{

	MainConfig.log_interval 			= 900;  // 15 minutes * 60 sec/min = 900 sec
	MainConfig.wifi_wakeup_interval 	= 0;    // off by default, previous was 300

	if (ResetSerialNumber)
	{
		// Manage the MainConfig.Comm_Mode and MainConfig.wifi_wait_connect bit fields here.
		// Normally, these would only be changed by explicit JSON commands, not a generic reset.
		// However, when resetting the serial number, they need to be set to factory defaults.

		ClearWiFiConfigured();
		ClearCellEnabled();
		ClearWiFiInternalAnt();
        ClearFirstResponse();
        ClearBLENameOverride();

		ClearUnusedFlags();

		MainConfig.wifi_wait_connect = 61;   // This is now used for other bit patterns

		SetCellModule(CELL_MOD_NONE);  // Force production to use the UI to initialize the cell modem.

		SetRTCBatt(RTC_BATT_RECHARGE_OFF);  // Force production to use the UI to initialize battery charging
	}


	MainConfig.wifi_browser_connect 	= 180;
	MainConfig.inactivity_timeout 		= 180;
	MainConfig.max_connect_time 		= 600;
	if (ResetSerialNumber || (MainConfig.battery_reset_datetime == 5))
	{
	  MainConfig.battery_reset_datetime = rtc_get_time(); // previously wifi_livesend_interval 	= 5;
	}
	MainConfig.powerdown_mode_normal 	= 1;
	MainConfig.units					= UNITS_ENGLISH;
	MainConfig.setLevel                 = 30;
	MainConfig.val                      = MAX_BATTERY;
	MainConfig.wd_current				= 80;   // a value of 1 will result in a correct daily average until either wd or we is calculated (which takes 24 hrs)
	MainConfig.we_current				= 35;   // a value of 1 will result in a correct daily average until either wd or we is calculated (which takes 24 hrs)
	strcpy(MainConfig.MAC, "00:00:00:00:00:00");
	strcpy(MainConfig.SSID, "iTracker");
	strcpy(MainConfig.PassKey, "00000000");



	strcpy(MainConfig.site_name,"?????");
	MainConfig.pipe_id 				= 8.0;
	MainConfig.damping 				= 3;
	MainConfig.population 			= 100;
	MainConfig.sensor_freq 			= 66000;

	//strcpy(MainConfig.AddText, " ");
	for (int i=0; i < 24; i++)
	{
			MainConfig.AddText[i] = 0;
	}
	MainConfig.AddText[0] = ' ';  // keep 1st character a blank like previous releases
	for (int i=0; i < 6; i++)
	{
		for (int j=0; j < 16; j++)
		{
	      MainConfig.PhoneNum[i][j] = 0;
		}
	}
	if (ResetSerialNumber)
	{
		for (int i=0; i < 10; i++)
		{
			MainConfig.serial_number[i] = 0;
		}
	}
#if 0 // original
	for (int i=0; i < 8; i++)
	{
		MainConfig.ChID[i] = 0;
	}
#else
	MainConfig.ForceType = FORCE_TYPE_USE_HARDWARE;
	MainConfig.spare_2 = 0;
	MainConfig.spare_3 = 0;
	MainConfig.spare_4 = 0;
	MainConfig.spare_5 = 0;
	MainConfig.spare_6 = 0;
	MainConfig.spare_7 = 0;
	MainConfig.spare_8 = 0;
#endif

	MainConfig.AlertLevel1		= 0;
	MainConfig.AlertLevel2		= 0;
	MainConfig.ClearLevel		= 0;
	MainConfig.AlarmBatt		= 20;
	MainConfig.vertical_mount	= 0;

}

#if 0
static void report_serial_number_and_MAC(void)
{
    char msg[100];
	sprintf(msg, "Serial Number %s  MAC %s", MainConfig.serial_number, MainConfig.MAC);
	AppendStatusMessage(msg);
}
#endif

static void report_main_config(void)
{
	char msg[80];
	int dec1, dec2;

	sprintf(msg,"MainConfig.site_name = %s", MainConfig.site_name);
	AppendStatusMessage(msg);

	sprintf(msg,"MainConfig.serial_number = %s", MainConfig.serial_number);
	AppendStatusMessage(msg);

	convfloatstr_4dec(MainConfig.val, &dec1, &dec2);
	sprintf(msg, "MainConfig.val = %d.%04d", dec1, dec2);
	AppendStatusMessage(msg);

	convfloatstr_4dec(MainConfig.wd_current, &dec1, &dec2);
	sprintf(msg, "MainConfig.wd_current = %d.%04d", dec1, dec2);
	AppendStatusMessage(msg);

	convfloatstr_4dec(MainConfig.we_current, &dec1, &dec2);
	sprintf(msg, "MainConfig.we_current = %d.%04d", dec1, dec2);
	AppendStatusMessage(msg);

	convfloatstr_4dec(MainConfig.setLevel, &dec1, &dec2);
	sprintf(msg, "MainConfig.setLevel = %d.%04d", dec1, dec2);
	AppendStatusMessage(msg);

#if 0
	sprintf(msg,"MainConfig.AddText = %s", MainConfig.AddText);
	AppendStatusMessage(msg);

	sprintf(msg,"MainConfig.PhoneNum = %s", MainConfig.PhoneNum);
	AppendStatusMessage(msg);
#endif

	sprintf(msg,"MainConfig.log_interval = %lu", MainConfig.log_interval);
	AppendStatusMessage(msg);

	sprintf(msg,"MainConfig.wifi_wakeup_interval = %lu", MainConfig.wifi_wakeup_interval);
	AppendStatusMessage(msg);
#if 0

#endif
	sprintf(msg,"MainConfig.battery_reset_datetime = %lu", MainConfig.battery_reset_datetime);
	AppendStatusMessage(msg);

	sprintf(msg,"MainConfig.units = %d", MainConfig.units);
	AppendStatusMessage(msg);

	sprintf(msg,"MainConfig.MAC = %s", MainConfig.MAC);
	AppendStatusMessage(msg);

	convfloatstr_4dec(MainConfig.pipe_id, &dec1, &dec2);
	sprintf(msg, "MainConfig.pipe_id = %d.%04d", dec1, dec2);
	AppendStatusMessage(msg);

	convfloatstr_4dec(MainConfig.vertical_mount, &dec1, &dec2);
	sprintf(msg, "MainConfig.vertical_mount = %d.%04d", dec1, dec2);
	AppendStatusMessage(msg);

	sprintf(msg,"MainConfig.damping = %d", MainConfig.damping);
	AppendStatusMessage(msg);

	sprintf(msg,"MainConfig.SSID = %s", MainConfig.SSID);
	AppendStatusMessage(msg);

	sprintf(msg,"MainConfig.population = %lu", MainConfig.population);
	AppendStatusMessage(msg);

	sprintf(msg,"MainConfig.PassKey = %s", MainConfig.PassKey);
	AppendStatusMessage(msg);

	convfloatstr_4dec(MainConfig.AlertLevel1, &dec1, &dec2);
	sprintf(msg, "MainConfig.AlertLevel1 = %d.%04d", dec1, dec2);
	AppendStatusMessage(msg);

	convfloatstr_4dec(MainConfig.AlertLevel2, &dec1, &dec2);
	sprintf(msg, "MainConfig.AlertLevel2 = %d.%04d", dec1, dec2);
	AppendStatusMessage(msg);

	convfloatstr_4dec(MainConfig.ClearLevel, &dec1, &dec2);
	sprintf(msg, "MainConfig.ClearLevel = %d.%04d", dec1, dec2);
	AppendStatusMessage(msg);

	sprintf(msg,"MainConfig.Comm_Mode = %X", MainConfig.Comm_Mode);
	AppendStatusMessage(msg);

	sprintf(msg,"MainConfig.ForceType = %d", MainConfig.ForceType);
	AppendStatusMessage(msg);


	switch (ProductType)
	{
	case PRODUCT_TYPE_ITRACKER:      sprintf(msg,"Configured as iTracker"); break;
	case PRODUCT_TYPE_FIRSTRESPONSE: sprintf(msg,"Configured as FirstResponse"); break;
	case PRODUCT_TYPE_SEWERWATCH:    sprintf(msg,"Configured as SewerWatch"); break;
	case PRODUCT_TYPE_SMARTFLUME:    sprintf(msg,"Configured as SmartFlume"); break;
	}

	AppendStatusMessage(msg);
}



int Load_MainConfig(void)  // returns 1 if an empty or corrupt flash was detected.
{
	int    result = 0;
	int8_t stat;
	uint16_t savechk;
	char msg[80];

	stat = BSP_QSPI_Read((unsigned char *)&MainConfig, MAIN_CONFIG_STORE, sizeof(MainConfig));

	savechk = MainConfig.checksum;
	MainConfig.checksum = 0x00;  // must compute checksum using a zero
	MainConfig.checksum = GetChkSum((unsigned char *)&MainConfig, sizeof(MainConfig));

	if ((stat != 0) || (MainConfig.checksum != savechk) || isErased((uint8_t *)&MainConfig, sizeof(MainConfig)))
	{

		if (stat != 0)
		{
			sprintf(msg,"MainConfig error:  stat = %d", stat);
			AppendStatusMessage(msg);
		}
		if (MainConfig.checksum != savechk)
		{
			sprintf(msg,"MainConfig error:  checksum mismatch %d %d", MainConfig.checksum, savechk);
			AppendStatusMessage(msg);
		}
		Reset_MainConfig(RESET_SERIAL_NUMBER);
		saveMainConfig = 1;
		result = 1;  // an empty or corrupt flash was detected
	}



	// Sometimes MainConfig gets zeroed out with a valid checksum.
	// Check for logically impossible values and reset the configuration
	if ((MainConfig.log_interval < 1) ||
		(MainConfig.pipe_id < 1) ||
		(MainConfig.population < 1))
	{
		Reset_MainConfig(DO_NOT_RESET_SERIAL_NUMBER);
		saveMainConfig = 1;
		result = 1;  // an empty or corrupt flash was detected
	}

	// When the user types a quotation mark in the Site name, the firmware ends up storing ASCII 92 (backslash)
	// for that character.  Later, when the firmware sends the site name to the user, it hangs the Zentri module very bad.
	// The only remedy is a Zentri reset AND to clear out the offending character.

	// Scan for embedded backslashes in the Site name --- they will trash the Zentri module.
	for (int i=0; i < sizeof(MainConfig.site_name); i++)
	{
		if (MainConfig.site_name[i] == 0) break;
		if (MainConfig.site_name[i] == '\\')
		{
			MainConfig.site_name[i] = ' ';  // change it to a space
			saveMainConfig = 1;
			// Zentri is always reset on power-on, after this code.
			// May want to trigger a WiFi reset here because the Zentri is basically known to be trashed.
			// That would avoid having to have the FactoryResetZentri.txt file on an SD card.
			// Further, it may make sense to have some other pro-active way that the f/w can talk
			// to the Zentri to make sure it is ok without user interaction.
		}
	}


	// Handle conversion of old config files to new format
	if (MainConfig.battery_reset_datetime == 5)
	{
		MainConfig.battery_reset_datetime = rtc_get_time(); // previously wifi_livesend_interval 	= 5;
		saveMainConfig = 1;
	}

#if 1
	// Clear deprecated bitflags
	if (IsFirstResponse())
	{
		// if not being forced by a file on the SD card, force the equivalent of the old bitflag
		if (sd_ForceType == FORCE_TYPE_USE_HARDWARE)
		{
		  sd_ForceType = FORCE_TYPE_FIRSTRESPONSE;
		}
		ClearFirstResponse();
		saveMainConfig = 1;
		AppendStatusMessage("FirstResponse bitflag converted");
	}

	if (IsBLENameOverride())   // see JSON {"Command":"b1"} BLE name:
	{
		// if not being forced by a file on the SD card, force the equivalent of the old bitflag
		if (sd_ForceType == FORCE_TYPE_USE_HARDWARE)
		{
		  // Transfer the old bitflag into the new format
		  if (BoardId == BOARD_ID_SEWERWATCH_1)
		  {
		    sd_ForceType = FORCE_TYPE_ITRACKER;
		  }
		}

		ClearBLENameOverride();
		saveMainConfig = 1;
		AppendStatusMessage("BLEName bitflag converted (old Force_iTracker method)");
	}
#endif


	// ===== Override configurations if specified on SD card
	if (sd_Disable_Cell)
	{
		AppendStatusMessage("SD card: disabling cell");
	    ClearCellEnabled();
	    saveMainConfig = 1;
	}

#if 0 // original
	if (sd_ForceType)
	{
		AppendStatusMessage("SD card: forcing product type to iTracker");
		SetBLENameOverride();
	    saveMainConfig = 1;
	}
#else

	// Override MainConfig.ForceType if needed
	if (sd_ForceType != FORCE_TYPE_USE_HARDWARE)
	{
		sprintf(msg, "SD card: forcing product type to %d", sd_ForceType);
		AppendStatusMessage(msg);
		MainConfig.ForceType = sd_ForceType;
		saveMainConfig = 1;
	}


	// Use saved configuration to establish ProductType
	switch (MainConfig.ForceType)
	{
	  default:
	  case FORCE_TYPE_USE_HARDWARE:
		  if (BoardId == BOARD_ID_SEWERWATCH_1)
		  {
			  ProductType = PRODUCT_TYPE_SEWERWATCH;
		  }
		  else
		  {
			  ProductType = PRODUCT_TYPE_ITRACKER;
		  }
		  break;
	  case FORCE_TYPE_ITRACKER:
		  ProductType = PRODUCT_TYPE_ITRACKER;
		  break;
	  case FORCE_TYPE_FIRSTRESPONSE:
		  ProductType = PRODUCT_TYPE_FIRSTRESPONSE;
		  break;
	  case FORCE_TYPE_SEWERWATCH:
		  ProductType = PRODUCT_TYPE_SEWERWATCH;
		  break;
	  case FORCE_TYPE_SMARTFLUME:
		  ProductType = PRODUCT_TYPE_SMARTFLUME;
		  break;
	}

#endif

	// ===== End Override configurations



	report_main_config();

	//report_serial_number_and_MAC();

	rtc_report_battery_charging_bit();

	// zg - Both Mark Watson and I observed odd behavior when manipulating the recharge bit during power-on

	return result;
}





void Save_MainConfig(void)
{
	ClearUnusedFlags();  // zg - for initial release 4.1.x be sure to wipe out lsb bits 1 and 2

#if 1
	MainConfig.checksum = 0x00;  // must compute checksum using a zero
	MainConfig.checksum = GetChkSum((unsigned char *)&MainConfig, sizeof(MainConfig));
#endif
	SaveConfigData((uint8_t*)&MainConfig, MAIN_CONFIG_STORE, sizeof(MainConfig));
}




// SENSOR
void Reset_SensorConfig(void)
{

	AppendStatusMessage("Reset SensorConfig");

	// Zero out the array
	for (int i = 0; i < 128; i++)
	{
		SensorConfig.grid[i] = 0;
	}
	SensorConfig.grid[76] = 0x80;  // to match grid export from EVM, register is read only, never written but is exported by EVM to grid.

	SensorConfig.struct_version = SENSOR_STRUCT_V0;
	SensorConfig.checksum       = 0;  // not used

	SensorConfig.grid[0] = 0;
	SensorConfig.grid[1] = 0;
	SensorConfig.grid[2] = 0;
	SensorConfig.grid[3] = 0;
	SensorConfig.grid[4] = 0;
	SensorConfig.grid[5] = 0;
	SensorConfig.grid[6] = 0;
	SensorConfig.grid[7] = 0;
	SensorConfig.grid[8] = 0;
	SensorConfig.grid[9] = 0;
	SensorConfig.grid[10] = 0;
	SensorConfig.grid[11] = 0;
	SensorConfig.grid[12] = 0;
	SensorConfig.grid[13] = 0;
	SensorConfig.grid[14] = 0;
	SensorConfig.grid[15] = 0;
	SensorConfig.grid[16] = 0;
	SensorConfig.grid[17] = 0;
	SensorConfig.grid[18] = 0;
	SensorConfig.grid[19] = 0;
	SensorConfig.grid[20] = 175;
	SensorConfig.grid[21] = 255;
	SensorConfig.grid[22] = 255;
	SensorConfig.grid[23] = 45;
	SensorConfig.grid[24] = 101;
	SensorConfig.grid[25] = 150;
	SensorConfig.grid[26] = 88;
	SensorConfig.grid[27] = 64;
	SensorConfig.grid[28] = 222;
	SensorConfig.grid[29] = 208;
	SensorConfig.grid[30] = 26;
	SensorConfig.grid[31] = 18;
	SensorConfig.grid[32] = 91;
	SensorConfig.grid[33] = 255;
	SensorConfig.grid[34] = 172;  // This register controls the pga460 preset 1 and 2 record time length.  See table 43 of data sheet.
	SensorConfig.grid[35] = 19;
	SensorConfig.grid[36] = 175;
	SensorConfig.grid[37] = 140;
	SensorConfig.grid[38] = 91;
	SensorConfig.grid[39] = 0;
	SensorConfig.grid[40] = 0;
	SensorConfig.grid[41] = 0;
	SensorConfig.grid[42] = 0;
	SensorConfig.grid[43] = 46;
	SensorConfig.grid[65] = 143;
	SensorConfig.grid[66] = 36;
	SensorConfig.grid[67] = 249;
	SensorConfig.grid[68] = 165;
	SensorConfig.grid[69] = 3;
	SensorConfig.grid[70] = 45;
	SensorConfig.grid[71] = 124;
	SensorConfig.grid[72] = 211;
	SensorConfig.grid[73] = 1;
	SensorConfig.grid[74] = 151;
	SensorConfig.grid[75] = 0;
	SensorConfig.grid[95] = 155;
	SensorConfig.grid[96] = 188;
	SensorConfig.grid[97] = 188;
	SensorConfig.grid[98] = 204;
	SensorConfig.grid[99] = 204;
	SensorConfig.grid[100] = 204;
	SensorConfig.grid[101] = 16;
	SensorConfig.grid[102] = 132;
	SensorConfig.grid[103] = 33;
	SensorConfig.grid[104] = 8;
	SensorConfig.grid[105] = 66;
	SensorConfig.grid[106] = 16;
	SensorConfig.grid[107] = 26;
	SensorConfig.grid[108] = 26;
	SensorConfig.grid[109] = 26;
	SensorConfig.grid[110] = 0;
	SensorConfig.grid[111] = 155;
	SensorConfig.grid[112] = 188;
	SensorConfig.grid[113] = 188;
	SensorConfig.grid[114] = 204;
	SensorConfig.grid[115] = 204;
	SensorConfig.grid[116] = 204;
	SensorConfig.grid[117] = 33;
	SensorConfig.grid[118] = 8;
	SensorConfig.grid[119] = 66;
	SensorConfig.grid[120] = 16;
	SensorConfig.grid[121] = 133;
	SensorConfig.grid[122] = 50;
	SensorConfig.grid[123] = 68;
	SensorConfig.grid[124] = 110;
	SensorConfig.grid[125] = 200;
	SensorConfig.grid[126] = 0;
	SensorConfig.grid[127] = 113;

}


static void report_sensor_config(void)
{
	for (int i = 0; i < 128; i++)
	{
		char msg[80];
		sprintf(msg,"grid[%d] = %d", i, SensorConfig.grid[i]);
		AppendStatusMessage(msg);
	}



	{
		char msg[40];
	    uint8_t P1_REC = SensorConfig.grid[34] >> 4;
	    uint8_t P2_REC = SensorConfig.grid[34] & 0xF;
	    float time_delay_ms;

		if (P1_REC > P2_REC)
			time_delay_ms = 4.096 * (P1_REC + 1);
		else
			time_delay_ms = 4.096 * (P2_REC + 1);

		pga460_burst_listen_delay_ms = (int)time_delay_ms + 4;  // add margin to the longest record time (1 ms for truncation plus 3 additional)

		sprintf(msg, "P1_REC = %3u, P2_REC = %3u, delay_ms = %d\n", P1_REC, P2_REC, pga460_burst_listen_delay_ms);
		AppendStatusMessage(msg);
	}

}

void Load_SensorConfig(void)
{
	// Zero out the array
	for (int i = 0; i < 128; i++)
	{
		SensorConfig.grid[i] = 0;
	}

	if (BoardId == BOARD_ID_SEWERWATCH_1)
	{
		// Read the configuration from flash.
		if (BSP_QSPI_Read((unsigned char *) &SensorConfig, SENSOR_CONFIG_STORE, sizeof(SensorConfig)) != QSPI_OK)
		{
			_Error_Handler(__FILE__, __LINE__);
		}
		else
		{
			// Detect empty flash
			if (isErased((uint8_t *)&SensorConfig, sizeof(SensorConfig)))
			{
				Reset_SensorConfig();
				saveSensorConfig = 1;
			}
		}

		// Kludge to reset sensor config
		//Reset_SensorConfig();
		//saveSensorConfig = 1;
		// End Kludge

		report_sensor_config();
	}
}



void Save_SensorConfig(void)
{
	if (BoardId == BOARD_ID_SEWERWATCH_1)
	{
		SaveConfigData((uint8_t*)&SensorConfig, SENSOR_CONFIG_STORE, sizeof(SensorConfig));
	}
}



void set_default_cell_ip_and_port(void)
{
	int i;

	for (i=0; i < CELL_IP_MAX; i++) CellConfig.ip[i]=0;
	for (i=0; i < CELL_PORT_MAX; i++) CellConfig.port[i]=0;

	// Establish the original s2c ip address and port as defaults.
	// This can change with new firmware releases as needed,
	// for example the initial AWS server was:  18.236.236.185  port 2222

	//strcpy(CellConfig.ip, "35.196.120.101");
	//strcpy(CellConfig.port, "9009");


	//strcpy(CellConfig.ip, "18.236.236.185"); // For 4.3.0 switch to AWS server
	strcpy(CellConfig.ip, "52.24.219.107"); // For 4.3.1 switch to load-balanced AWS server

	strcpy(CellConfig.port, "2222");

}

// CELLULAR
void Reset_CellConfig(void)
{
	int i;

	// Minimum required samples.
	CellConfig.SampleCnt = 96;

	CellConfig.ctx = 0;  // no override of ctx and apn

	//strcpy(CellConfig.apn, "(no apn)");  //  4.1.4041 and 4.1.4042
	for (i=0; i < CELL_APN_MAX; i++) CellConfig.apn[i]=0;

	//strcpy(CellConfig.status, "(no status)"); //  4.1.4041 and 4.1.4042
	for (i=0; i < CELL_STATUS_MAX; i++) CellConfig.status[i]=0;

	// Leave some spares...
	CellConfig.struct_version = 1;
	CellConfig.spare1 = 0;  // this was default prior to 5.0.236
	CellConfig.spare2 = 0;  // this was default prior to 4.4.3, was used briefly from 4.4.3 to 4.4.5 to identify Xbee3

	CellConfig.spare3 = 0.0;

	set_default_cell_ip_and_port();

	for (i=0; i < CELL_STATUS_MAX; i++) CellConfig.error[i]=0;
}

static void upgrade_cell_config_to_version_2(void)
{

	CellConfig.struct_version = 2;

	set_default_cell_ip_and_port();
}

static void report_cell_config(void)
{

	char msg[80];
	sprintf(msg,"CellConfig.struct_version = %lu", CellConfig.struct_version);
	AppendStatusMessage(msg);

	sprintf(msg,"CellConfig.ctx = %lu", CellConfig.ctx);
	AppendStatusMessage(msg);

	sprintf(msg,"CellConfig.apn = %s", CellConfig.apn);
	AppendStatusMessage(msg);

	sprintf(msg,"CellConfig.status = %s", CellConfig.status);
	AppendStatusMessage(msg);

	sprintf(msg,"CellConfig.SampleCnt = %lu", CellConfig.SampleCnt);
	AppendStatusMessage(msg);

	sprintf(msg,"CellConfig.spare1 = %lu", CellConfig.spare1);
	AppendStatusMessage(msg);

	sprintf(msg,"CellConfig.error = %s", CellConfig.error);
	AppendStatusMessage(msg);

	sprintf(msg,"CellConfig.ip = %s", CellConfig.ip);
	AppendStatusMessage(msg);

	sprintf(msg,"CellConfig.port = %s", CellConfig.port);
	AppendStatusMessage(msg);

	if (IsCellEnabled())
	{
		AppendStatusMessage("Cell enabled.");
	}
	else
	{
		AppendStatusMessage("Cell disabled.");
	}

}


void Load_CellConfig(void)
{
	uint8_t status;

	// Read the configuration from flash.
	status = BSP_QSPI_Read((unsigned char *) &CellConfig, CELL_CONFIG_STORE, sizeof(CellConfig));
	if (status != QSPI_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}
	else
	{
		// Detect empty flash
		if (isErased((uint8_t *)&CellConfig, sizeof(CellConfig)))
		{
			Reset_CellConfig();
			saveCellConfig = 1;
		}
	}

	// Check for old version
	if (CellConfig.struct_version == 1)
	{
		upgrade_cell_config_to_version_2();
		saveCellConfig = 1;
	}


	// Check defaults and limits


	if (strcmp(CellConfig.apn, "(no apn)") == 0)  // Correct an error in 4.1.4041 and 4.1.4042
	{
		for (int i=0; i < CELL_APN_MAX; i++) CellConfig.apn[i]=0;
		saveCellConfig = 1;
	}
	if (strcmp(CellConfig.apn, "(no status)") == 0)  // Correct an error in 4.1.4041 and 4.1.4042
	{
		for (int i=0; i < CELL_STATUS_MAX; i++) CellConfig.status[i]=0;
		saveCellConfig = 1;
	}

	// Force a known state for spare2.
	if (CellConfig.spare2 != 0)
	{
		CellConfig.spare2 = 0;  // this was default prior to 4.4.3, was used briefly from 4.4.3 to 4.4.5 to identify Xbee3
		saveCellConfig = 1;
	}

	strcpy(s2c_cell_status, CellConfig.status);

	report_cell_config();
}


void Save_CellConfig(void)
{
	SaveConfigData((uint8_t*)&CellConfig, CELL_CONFIG_STORE, sizeof(CellConfig));
}


void turn_off_battery_charging(void)
{

}
void turn_on_battery_charging(void)
{

}


