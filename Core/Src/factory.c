/*
 * factory.c
 *
 *  Created on: Mar 24, 2020
 *      Author: Zachary
 */

#include "stdint.h"
#include "stdlib.h"  // atoi
#include "time.h"
#include "rtc.h"
#include "fatfs.h" // SD Card
#include "sdmmc.h" // SD Card
#include "common.h"
#include "dual_boot.h"
#include "bootloader.h"
#include "version.h"
#include "debug_helper.h"
#include "WiFi_Socket.h"
#include "Sample.h"
#include "trace.h"
#include "factory.h"
#include "testalert.h"
#include "zentri.h"


#define MAX_INPUT_LINE 50

int sd_RunningFromBank2 = 0;   // suspect trace_xxx functions do not run reliably from bank 2
int sd_WiFiWakeLogEnabled = 0;
int sd_WiFiCmdLogEnabled = 1;
int sd_WiFiDataLogEnabled = 1;
int sd_CellCmdLogEnabled = 1;
int sd_CellDataLogEnabled = 1;
int sd_SleepAtPowerOnEnabled = 0;
int sd_TraceVoltageEnabled = 0;
int sd_DataLoggingEnabled = 1;
int sd_ESP32_Enabled = 1;
int sd_ForceType = FORCE_TYPE_USE_HARDWARE;
int sd_Disable_Cell = 0;

int sd_UartBufferSizeLogEnabled = 0;

int sd_EVM_1000 = 0;
int sd_EVM_460 = 0;
int sd_CMD_460 = 0;
int sd_EVM_460_Trace = 0;
int sd_UART3_Echo = 0;          // 115200, 8, N, 2

int sd_UART1_Test = 0;          // 115200, 8, N, 1   CELL   uses VEL 1 (UART4) to pc
int sd_UART2_Test = 0;          // 115200, 8, N, 1   WIFI
int sd_UART3_Test = 0;          // 115200, 8, N, 1   RS-485
int sd_UART4_Test = 0;          // 115200, 8, N, 1   VELOCITY 1
int sd_LPUART1_Test = 0;        // 115200, 8, N, 1   VELOCITY 2

         // Pass-Thru UART4 (PC) to UART1 (CELL) to talk to pc:  115200, 8, N, 1   VELOCITY 1 PORT UART4
         // UART1 uses hardware handshaking

int sd_UART_1SPY5 = 0;        // UART1 (CELL) spies on UART5 (VEL2) used to test cables
int sd_UART_5SPY1 = 0;        // UART5 (VEL2) spies on UART1 (CELL) might be useful for CELL testing
int sd_UART_5SPY2 = 0;        // UART5 (VEL2) spies on UART2 (WIFI) used for ESP32 bootload file capture
int sd_UART_2SPY4 = 0;        // UART2 (WIFI) spies on UART4 (VEL1) used to test cables
int sd_UART_4SPY2 = 0;        // UART4 (VEL1) spies on UART2 (WIFI)  useful for BT testing with a console to observe traffic

int sd_TestCellData = 0;
int sd_TestCellAlerts = 0;
int sd_DemoCellLoop = 0;
int sd_TestCellNoTx = 0;


int sd_TraceSensor = 0;
int sd_StressTest = 0;
int sd_FactoryTest = 0;


FactoryTest_t FactoryTest;


//Power On
void get_power_on_msg(char * msg)
{
	int bank;
	char mcu[20];


	mcu[0]=0;
	if (isGen4()) strcpy(mcu,"Gen4");
	if (isGen5()) strcpy(mcu,"Gen5");

	if  (GetBFB2() == OB_BFB2_ENABLE)
	{
		bank = 2;
	}
	else
	{
		bank = 1;
	}

	// Report version and if running from Bank 1 or Bank 2
	sprintf(msg, "Power on at %lu. Running %d.%d.%d from Bank %d, MCUID = %lu, %s, BoardId = %lu\n", TimeAtPowerOn, VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD, bank, stm32_DEVID, mcu, BoardId);

}

// Diagnostic feature:  enable specific logging functions


void EnableTraceFunctions(void)
{
	FRESULT fr;
	FIL fstatus;

	if (sd_init() != FR_OK) return;  // nothing to do (e.g. no SD card present)

	fr = f_open(&fstatus, ENABLE_UART_SIZE_LOG_NAME, FA_READ);
	if (fr == FR_OK)
	{
		sd_UartBufferSizeLogEnabled = 1;
		AppendStatusMessage("UART Buffer Size Log enabled.");
		f_close(&fstatus);
	}


	fr = f_open(&fstatus, ENABLE_WIFI_WAKE_LOG_NAME, FA_READ);
	if (fr == FR_OK)
	{
		sd_WiFiWakeLogEnabled = 1;
		AppendStatusMessage("WiFiWakeLog enabled.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_WIFI_CMD_LOG_NAME, FA_READ);
	if (fr == FR_OK)
	{
		sd_WiFiCmdLogEnabled = 1;
		AppendStatusMessage("WiFiCmdLog enabled.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_WIFI_DATA_LOG_NAME, FA_READ);
	if (fr == FR_OK)
	{
		sd_WiFiDataLogEnabled = 1;
		AppendStatusMessage("WiFiDataLog enabled.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_CELL_CMD_LOG_NAME, FA_READ);
	if (fr == FR_OK)
	{
		//SetCellEnabled();       // enable cell phone zg - THIS MIGHT NEED TO GO AWAY ONCE UI CAN ENABLE/DISABLE CELL PHONE
		sd_CellCmdLogEnabled = 1;  // enable cell logging
		AppendStatusMessage("CellCmdLog enabled.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_CELL_DATA_LOG_NAME, FA_READ);
	if (fr == FR_OK)
	{
		//SetCellEnabled();       // enable cell phone zg - THIS MIGHT NEED TO GO AWAY ONCE UI CAN ENABLE/DISABLE CELL PHONE
		sd_CellDataLogEnabled = 1;
		AppendStatusMessage("CellDataLog enabled.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_WIFI_SLEEP_AT_POWERON, FA_READ);
	if (fr == FR_OK)
	{
		sd_SleepAtPowerOnEnabled = 1;
		AppendStatusMessage("Sleep at power on enabled.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_TRACE_VOLTAGE_NAME, FA_READ);
	if (fr == FR_OK)
	{
		sd_TraceVoltageEnabled = 1;
		AppendStatusMessage("TraceVoltage enabled.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_EVM_1000, FA_READ);
	if (fr == FR_OK)
	{
		sd_EVM_1000 = 1;
		AppendStatusMessage("EVM TDC1000 pass-thru enabled (UART1 to LPUART1: 9600, 8, N, 1)");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_EVM_460, FA_READ);
	if (fr == FR_OK)
	{
		sd_EVM_460 = 1;
		AppendStatusMessage("EVM PGA460 pass-thru enabled (UART1 to UART3: 115200, 8, N, 2)");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_EVM_460_TRACE, FA_READ);
	if (fr == FR_OK)
	{
		sd_EVM_460_Trace = 1;
		AppendStatusMessage("EVM PGA460 pass-thru trace enabled");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_UART3_ECHO, FA_READ);
	if (fr == FR_OK)
	{
		sd_UART3_Echo = 1;
		AppendStatusMessage("UART3 Echo enabled (UART3: 115200, 8, N, 2)");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_UART1_TEST, FA_READ);
	if (fr == FR_OK)
	{
		sd_UART1_Test = 1;
		AppendStatusMessage("UART1 Test enabled (UART1: 115200, 8, N, 1)");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_LPUART1_TEST, FA_READ);
	if (fr == FR_OK)
	{
		sd_LPUART1_Test = 1;
		AppendStatusMessage("LPUART1 Test enabled (LPUART1: 115200, 8, N, 1)");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_UART_1SPY5, FA_READ);
	if (fr == FR_OK)
	{
		sd_UART_1SPY5 = 1;
		sd_UART_5SPY1 = 0;
		AppendStatusMessage("UART 1SPY5 Enabled");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_UART_5SPY1, FA_READ);
	if (fr == FR_OK)
	{
		sd_UART_5SPY1 = 1;
		sd_UART_1SPY5 = 0;
		AppendStatusMessage("UART 5SPY1 Enabled");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_UART_2SPY4, FA_READ);
	if (fr == FR_OK)
	{
		sd_UART_2SPY4 = 1;
		sd_UART_4SPY2 = 0;
		AppendStatusMessage("UART 2SPY4 Enabled");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_UART_4SPY2, FA_READ);
	if (fr == FR_OK)
	{
		sd_UART_4SPY2 = 1;
		sd_UART_2SPY4 = 0;
		AppendStatusMessage("UART 4SPY2 Enabled");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_TEST_CELL_DATA_NAME, FA_READ);
	if (fr == FR_OK)
	{
		sd_TestCellData = 1;

		testalert_init(0);  // 0 = do not test alerts; 1 = test alerts

		AppendStatusMessage("Testing cell data - using simulated data.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_TEST_CELL_ALERTS_NAME, FA_READ);
	if (fr == FR_OK)
	{

		sd_TestCellAlerts = 1;

		AppendStatusMessage("Testing cell alerts - using simulated data.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_DEMO_CELL_LOOP_NAME, FA_READ);
	if (fr == FR_OK)
	{
		sd_DemoCellLoop = 1;

		AppendStatusMessage("Demo loop of cell alerts is active.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_TEST_CELL_NO_TX_NAME, FA_READ);
	if (fr == FR_OK)
	{

		sd_TestCellNoTx = 1;

		AppendStatusMessage("Testing cell - no TX to cloud.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_TRACE_SENSOR_NAME, FA_READ);
	if (fr == FR_OK)
	{

		sd_TraceSensor = 1;

		AppendStatusMessage("TraceSensor is active.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, ENABLE_STRESS_TEST_NAME, FA_READ);
	if (fr == FR_OK)
	{

		sd_StressTest = 1;

		AppendStatusMessage("Stress Test of transmit FET is active.");
		f_close(&fstatus);
	}

#if 0
	if (IsCellEnabled())
	{
		AppendStatusMessage("Cell enabled.");
	}
	else
	{
		AppendStatusMessage("Cell disabled.");
	}
#endif

	fr = f_open(&fstatus, DISABLE_DATA_LOGGING, FA_READ);
	if (fr == FR_OK)
	{
		sd_DataLoggingEnabled = 0;  // to bypass data logging when testing known sample data
		AppendStatusMessage("Data logging disabled.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, DISABLE_ESP32, FA_READ);
	if (fr == FR_OK)
	{
		sd_ESP32_Enabled = 0;  // to keep ESP32 from turning on
		AppendStatusMessage("ESP32 disabled.");
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, DISABLE_CELL, FA_READ);
	if (fr == FR_OK)
	{
		sd_Disable_Cell = 1;
		f_close(&fstatus);
	}

	// For ProductType, if multiple "force" files exist, must establish a priority:


	fr = f_open(&fstatus, FORCE_ITRACKER, FA_READ);
	if (fr == FR_OK)
	{
		sd_ForceType = FORCE_TYPE_ITRACKER;
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, FORCE_FIRSTRESPONSE, FA_READ);
	if (fr == FR_OK)
	{
		sd_ForceType = FORCE_TYPE_FIRSTRESPONSE;
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, FORCE_SEWERWATCH, FA_READ);
	if (fr == FR_OK)
	{
		sd_ForceType = FORCE_TYPE_SEWERWATCH;
		f_close(&fstatus);
	}

	fr = f_open(&fstatus, FORCE_SMARTFLUME, FA_READ);
	if (fr == FR_OK)
	{
		sd_ForceType = FORCE_TYPE_SMARTFLUME;
		f_close(&fstatus);
	}




}

void save_log_to_sd_card_as_csv(void)
{
	int saved_value;

	saved_value = wifi_report_seconds;

	wifi_report_seconds = 1;  // set flag to report seconds when saving data log from FactoryExtract.txt file on SD card

	wifi_send_download(DL_TYPE_SDCARD, 0);  // Extract all data to SD card !!! THIS REINITIALIZES SD CARD FAT FILE SYSTEM...

	wifi_report_seconds = saved_value;  // restore to normal CSV output
}

// This function allows data to be saved to the SD Card
// in the event that data cannot be extracted via Wifi.
//
static void PerformFactoryExtract(void)
{
	FRESULT fr;
	FIL fstatus;

    // Open file named "FactoryExtract.txt"
    fr = f_open(&fstatus, FACTORY_EXTRACT_ENABLE_NAME, FA_READ);
    if (fr != FR_OK) return;  // no request to extract data
	f_close(&fstatus);

    AppendStatusMessage("Performing factory extract...");

    log_ExtractDataRecords();  // Report entire contents of flash memory as FactoryExtract0001.txt file

    save_log_to_sd_card_as_csv();

    AppendStatusMessage("Factory extract complete.");
}

static void PerformFactoryCellOverride(void)
{
	FRESULT fr;
	FIL fstatus;

    fr = f_open(&fstatus, FACTORY_CELL_OVERRIDE_NAME, FA_READ);
    if (fr != FR_OK) return;  // no request to override cell modem ctx and apn
	f_close(&fstatus);

    AppendStatusMessage("Setting cell override ctx and apn.");

    CellConfig.ctx = 1;  // Use ATT
    strcpy(CellConfig.apn,"flolive.net");

    saveCellConfig = 1;

}

static void PerformFactoryWiFiExternal(void)
{
	FRESULT fr;
	FIL fstatus;

    fr = f_open(&fstatus, FACTORY_WIFI_EXTERNAL_NAME, FA_READ);
    if (fr != FR_OK) return;  // no request to override antenna setting
	f_close(&fstatus);

    AppendStatusMessage("Setting WiFi antenna to external.");

    ClearWiFiInternalAnt();
    //SetWiFiInternalAnt();

    saveMainConfig = 1;
}

static void PerformFactoryWiFiInternal(void)
{
	FRESULT fr;
	FIL fstatus;

    fr = f_open(&fstatus, FACTORY_WIFI_INTERNAL_NAME, FA_READ);
    if (fr != FR_OK) return;  // no request to override antenna setting
	f_close(&fstatus);

    AppendStatusMessage("Setting WiFi antenna to internal.");

    SetWiFiInternalAnt();

    saveMainConfig = 1;
}

static void PerformFactoryFillMemory(void)
{

	FRESULT fr;
	FIL fstatus;

    // Open file named "FactoryFillMemory.txt"
    fr = f_open(&fstatus, FACTORY_FILLMEM_ENABLE_NAME, FA_READ);
    if (fr != FR_OK) return;  // no request to fill memory
	f_close(&fstatus);

    AppendStatusMessage("Performing factory fill memory...");

	loadSampleData(SAMPLE_DATA_FILL_MEMORY, SAMPLE_DATA_TYPE_PIPE_8);

    AppendStatusMessage("Factory fill memory complete.");
}

static void PerformFactoryLoadSampleDataFromCSV(void)
{

	FRESULT fr;
	FIL fstatus;
	TCHAR * pLine;
	char rtext[100]; /* File read buffer */

    // Open file named "FactoryLoadSampleData.txt"
    fr = f_open(&fstatus, FACTORY_LOAD_SAMPLE_DATA_NAME, FA_READ);
    if (fr != FR_OK) return;  // no request to load sample data

	// read a line terminated with '\n' up to max size.  Strip '\r'. Return a null-terminated string usually ending with '\n' unless EOF is reached.
	pLine = f_gets(rtext, sizeof(rtext), &fstatus);
	f_close(&fstatus);

	if (pLine == NULL)
	{
		AppendStatusMessage("No Sample Data File specified.");
	}
	else
	{
		char msg[80];
		sprintf(msg,"Loading sample data from %s ...", pLine);
		AppendStatusMessage(msg);
	}


    if (Import_CSV_File(pLine))
    {
        AppendStatusMessage("Sample data loaded.");
    }
    else
    {
    	AppendStatusMessage("Error: Sample data not loaded.");
    }

#if 0
    // Automatically turn off the loading of sample data -- user must manually turn it on again --- not as useful as first thought...
    sprintf(rtext,"off%s", FACTORY_LOAD_SAMPLE_DATA_NAME);
	f_rename(FACTORY_LOAD_SAMPLE_DATA_NAME, rtext);

#endif
}






// Parse major, minor, build from a line of text (must be null terminated)
void parse_major_minor_build(char *text, int *major, int *minor, int *build)
{
	char * p;
	char * pend;
	char * pmajor;
	char * pminor;
	char * pbuild;

   	// parse major, minor, and build
    pend = text + strlen(text); // find the end
    p = text;
    pmajor = p;
    while (p < pend)
    {
    	if (*p == ' ') break;
    	p++;
    }
    pminor = p;
    p++;
    while (p < pend)
    {
    	if (*p ==  ' ') break;
    	p++;
    }
    pbuild = p;
    *major = atoi(pmajor);
    *minor = atoi(pminor);
    *build = atoi(pbuild);
}



// Get a line ending with an LF.  Null terminate the line at the LF character
uint8_t get_line(FIL * pSDFile, char *pLine, int maxlen)
{
	FRESULT sd_res;
	uint32_t bytesread=0;
	char input_ch; /* File read buffer */
	uint8_t line_len = 0;
	char *start_of_line;
	start_of_line = pLine;

	*pLine = '\0';  // return NULL by default

	do {
		// read one character
		sd_res = f_read(pSDFile, &input_ch, 1, (void *) &bytesread);
		if (sd_res != FR_OK) return 0;
		if (bytesread < 1)  // force end of line at EOF
		{
			input_ch = '\n';
		}
		if (input_ch != '\r')  // skip CR characters
		{
			// save it to the output
			*pLine = input_ch;
			pLine++;
			line_len++;
			if (line_len == maxlen)
			{
			    AppendStatusMessage("Error: line too long in file xyz. Skipping file.");
				return 0;
			}
		}
	} while (input_ch != '\n');

	if (pLine > start_of_line) pLine--;  // backup to the \n

	*pLine = '\0';  // null-terminate

	return strlen(start_of_line);  // return 0 for empty lines, non-zero otherwise

	//return 1;  // found a complete line
}

uint8_t get_line_skip_comments(FIL * pSDFile, char *pLine, int maxlen)
{
	uint8_t result;
	do
	{
		result = get_line(pSDFile, pLine, maxlen);
		if ((pLine[0] != '/') || (pLine[1] != '/'))
		{
			return result;  // not a commented line
		}
		// Otherwise, read the next line
	} while(result);

	return result;

}




#if 0

static void copy_serial_number(const char *input_line, char * serial_no)
{
	//           1         2         3
	// 0123456789012345678901234567890
	// 00:00:00:00:00:00 123456789\n
	//                   012345678
	int i, j;
	j=0;
	for (i = 18; i < 27; i++)
	{
		if (isdigit(input_line[i]))
		{
		  serial_no[j++] = input_line[i];
		}
		if (input_line[i] == '\n') i = 27;  // exit the loop if LF found
	}
	serial_no[j] = '\0';
}

static uint8_t find_match(const char *input_line, const char * MAC, char * serial_no)
{
	if (strstr(input_line, MAC) == input_line)
	{
		copy_serial_number(input_line, serial_no);
		return 1;
	}
	return 0;
}

void factory_autoconfig_serial_number(void)
{
	// The format of each line in the file must be 28 characters, last char "\n".  A "\r" will be skipped on input, which allows for CRLF line endings.
	// Example:
	// 00:00:00:00:00:00 123456789\n
	// If not all digits of the serial number are used, then fill with ?? \n
	// Definition of serial_no:  char serial_no[10];
	// Where serial_no[9] = '\0';  and index positions 0..8 are available for use (9 digit serial number)
	FIL SDFile;
	FRESULT sd_res;
	char input_line[29];  // 28 chars, plus null-terminator


	// Check for factory MAC to SerialNumber text file
	sd_res = f_open(&SDFile, FACTORY_MAC_TO_SN, FA_READ);
	if (sd_res != FR_OK) return;

    AppendStatusMessage("Performing factory MAC to SN...");

	// parse the file one line at a time
	while (get_line(&SDFile, input_line, sizeof(input_line)))
	{
		if (find_match(input_line, MainConfig.MAC, MainConfig.serial_number))
		{
			char msg[50];
			sprintf(msg, "%s assigned %s", MainConfig.MAC, MainConfig.serial_number);
		    AppendStatusMessage(msg);
		    saveMainConfig = 1;
			break;
		}
	}

	f_close(&SDFile);  // close file

    AppendStatusMessage("Factory MAC to SN complete.");
}
#endif

static void strip_trailing_spaces_and_comments(char *input_line)
{
	int i;

    for (i=0; i < strlen(input_line); i++)
    {
    	// scan for single quote
    	if (input_line[i] == '\'')
    	{
    		int j;
    		if (i==0)   // line consists only of a comment
    		{
    			input_line[0] = '\0';
    			return;
    		}
    		// truncate trailing spaces
    		j = i - 1;
    		while ((j > 0) && (input_line[j] == ' ')) j--;

    		if (input_line[j] != ' ')
    		{
    		  input_line[j+1] = '\0';
    		  return;
    		}
    		else  // line consists only of blanks
    		{
    			input_line[0] = '\0';
    			return;
    		}
    	}
    }
}


static int parse_factory_test_file(void)
{
	FIL SDFile;
	FRESULT sd_res;
	char input_line[MAX_INPUT_LINE];


	// Assign defaults
	strcpy(FactoryTest.base_ssid,"DUT_001");
	FactoryTest.number_of_readings = 480;
	FactoryTest.min_gain = 127;
	FactoryTest.max_gain = 252;
	FactoryTest.min_distance = 28.0;
	FactoryTest.max_distance = 32.0;
	FactoryTest.status = FACTORY_TEST_IN_PROGRESS;

	// Check for factory test text file
	sd_res = f_open(&SDFile, FACTORY_TEST, FA_READ);
	if (sd_res != FR_OK) return 0;


    get_line(&SDFile, input_line, sizeof(input_line));
    strip_trailing_spaces_and_comments(input_line);
    if (strlen(input_line) == 0)
    {
    	strcpy(input_line, "DUT_001");
    }
	strcpy(FactoryTest.base_ssid, input_line);

    get_line(&SDFile, input_line, sizeof(input_line));
    strip_trailing_spaces_and_comments(input_line);
    if (strlen(input_line) == 0)
    {
    	strcpy(input_line, "480");
    }
    FactoryTest.number_of_readings = atoi(input_line);

    get_line(&SDFile, input_line, sizeof(input_line));
    strip_trailing_spaces_and_comments(input_line);
    if (strlen(input_line) == 0)
    {
    	strcpy(input_line, "127");
    }
    FactoryTest.min_gain = atoi(input_line);

    get_line(&SDFile, input_line, sizeof(input_line));
    strip_trailing_spaces_and_comments(input_line);
    if (strlen(input_line) == 0)
    {
    	strcpy(input_line, "252");
    }
    FactoryTest.max_gain = atoi(input_line);



    get_line(&SDFile, input_line, sizeof(input_line));
    strip_trailing_spaces_and_comments(input_line);
    if (strlen(input_line) == 0)
    {
    	strcpy(input_line, "28.0");
    }
    FactoryTest.min_distance = strtof(input_line, 0);


    get_line(&SDFile, input_line, sizeof(input_line));
    strip_trailing_spaces_and_comments(input_line);
    if (strlen(input_line) == 0)
    {
    	strcpy(input_line, "32.0");
    }
    FactoryTest.max_distance = strtof(input_line, 0);


    f_close(&SDFile);  // close file

    return 1;
}

void PerformFactoryTest(void)
{

	char msg[128];

	if (!parse_factory_test_file()) return;

	// Enable Factory test
	sd_FactoryTest = 1;

    // Establish known baseline settings (defaults)

	Reset_MainConfig(DO_NOT_RESET_SERIAL_NUMBER);
	Reset_CellConfig();

	// Override hardware defaults with Factory Test values

	strcpy(MainConfig.SSID, FactoryTest.base_ssid);
	MainConfig.log_interval = 60;  // 1 minutes * 60 sec/min = 60 sec

	ClearCellEnabled(); // disable cell phone

	saveMainConfig = 1;
	saveCellConfig = 1;

	rtc_set_factory_date();  // Default RTC date is:  1/1/2018  00:00:00

	log_start(0);

	sprintf(msg, "Factory Test configured for MAC %s,  SSID %s", MainConfig.MAC, MainConfig.SSID);
    AppendStatusMessage(msg);
}

static void PerformFactoryResetZentri(void)
{
	FRESULT fr;
	FIL fstatus;
	char msg[75];
	char input_line[MAX_INPUT_LINE];

	uint32_t tickstart;
	uint32_t duration;

#if 0  // unclear if this is needed just yet.  Gen4 has Zentri, Gen5 has ESP32
	if (GetWiFiModule() != WIFI_MOD_ZENTRI)
	{
		AppendStatusMessage("Zentri module not configured.");
		return;
	}
#endif

    // Open file named "FactoryResetZentri.txt"
    fr = f_open(&fstatus, FACTORY_RESET_ZENTRI_ENABLE_NAME, FA_READ);
    if (fr != FR_OK) return;  // no request to reset Zentri

    // get the MAC address from the file if any
    get_line(&fstatus, input_line, sizeof(input_line));
    strip_trailing_spaces_and_comments(input_line);
    // note input_line may be empty (eg first char is null terminator)

	f_close(&fstatus);

    AppendStatusMessage("Performing Zentri Factory reset...");

    sd_WiFiCmdLogEnabled = 1;
    AppendStatusMessage("WiFiCmdLog enabled.");

	tickstart = HAL_GetTick();

    zentri_factory_reset(input_line, ZENTRI_RESET);

	duration = HAL_GetTick() - tickstart;

    sprintf(msg, "Zentri factory reset complete in %lu ms.", duration);

    AppendStatusMessage(msg);
}

/*
 * PerformFactoryEraseZentri
 *
 * For some units, the Zentri extended flash becomes corrupt and the files cannot be updated.
 * This will erase the extended flash and allow the normal SewerWatch upgrade process to work from the SD card.
 * However, it wipes out the Zentri Factory "webapp".
 * The "webapp" consists of four files named:
 *
 *   webapp/index.html
 *   webapp/unauthorized.html
 *   webapp/zentrios.css.gz
 *   webapp/zentrios.js.gz
 *
 * No accurate copies of these files could be found for the 4.3.6 release.
 * If such files can be found in the future, the Manifest.txt file will allow
 * a forced installation of those files, for example:
 *
 *     9 9 9  // Version: major minor build use 9 9 9 to force a load
 *     // Add or replace files
 *     // For Zentri folders use underscore for slash in SD card filename
 *     webapp/index.html
 *     webapp/unauthorized.html
 *     webapp/zentrios.css.gz
 *     webapp/zentrios.js.gz
 *
 * See the sample SDCard_ZentriWebApp image.
 *
 * For now, the objective is to get bricked units working with SewerWatch
 * even if that means wiping out the Zentri webapp.
 */
static void PerformFactoryEraseZentri(void)
{
	FRESULT fr;
	FIL fstatus;
	char msg[75];
	char input_line[MAX_INPUT_LINE];

	uint32_t tickstart;
	uint32_t duration;

    // Open file named "FactoryEraseZentri.txt"
    fr = f_open(&fstatus, FACTORY_ERASE_ZENTRI_ENABLE_NAME, FA_READ);
    if (fr != FR_OK) return;  // no request to erase Zentri


    // get the MAC address from the file if any
    get_line(&fstatus, input_line, sizeof(input_line));
    strip_trailing_spaces_and_comments(input_line);
    // note input_line may be empty (eg first char is null terminator)

	f_close(&fstatus);

    AppendStatusMessage("Performing Zentri Factory erase...");

    sd_WiFiCmdLogEnabled = 1;
    AppendStatusMessage("WiFiCmdLog enabled.");

	tickstart = HAL_GetTick();

    zentri_factory_reset(input_line, ZENTRI_ERASE);

	duration = HAL_GetTick() - tickstart;

    sprintf(msg, "Zentri factory erase complete in %lu ms.", duration);

    AppendStatusMessage(msg);

    // Kludge
    PerformFactoryResetZentri();

    while (1);   // Major Kludge
}



#if 1  // EXPORT_GRID

static void output_line(FIL * sdFileName, char *line)
{
	if (BSP_SD_IsDetected() == SD_PRESENT)  // zg - not sure if this needs to be here.  If the file write was started, it will finish pretty fast...
	{
		UINT sdCardBytes;
		f_write(sdFileName, line, strlen((char *)line), &sdCardBytes);
	}
}

static void export_grid_lines(FIL * sdFileName)
{
	uint8_t i;
	char msg[80];

	sprintf(msg,";GRID_USER_MEMSPACE\r\n"); output_line(sdFileName, msg);
	i = 0x00; sprintf(msg,"00 (USER_DATA1),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x01; sprintf(msg,"01 (USER_DATA2),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x02; sprintf(msg,"02 (USER_DATA3),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x03; sprintf(msg,"03 (USER_DATA4),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x04; sprintf(msg,"04 (USER_DATA5),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x05; sprintf(msg,"05 (USER_DATA6),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x06; sprintf(msg,"06 (USER_DATA7),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x07; sprintf(msg,"07 (USER_DATA8),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x08; sprintf(msg,"08 (USER_DATA9),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x09; sprintf(msg,"09 (USER_DATA10),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x0A; sprintf(msg,"0A (USER_DATA11),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x0B; sprintf(msg,"0B (USER_DATA12),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x0C; sprintf(msg,"0C (USER_DATA13),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x0D; sprintf(msg,"0D (USER_DATA14),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x0E; sprintf(msg,"0E (USER_DATA15),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x0F; sprintf(msg,"0F (USER_DATA16),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x10; sprintf(msg,"10 (USER_DATA17),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x11; sprintf(msg,"11 (USER_DATA18),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x12; sprintf(msg,"12 (USER_DATA19),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x13; sprintf(msg,"13 (USER_DATA20),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x14; sprintf(msg,"14 (TVGAIN0),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x15; sprintf(msg,"15 (TVGAIN1),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x16; sprintf(msg,"16 (TVGAIN2),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x17; sprintf(msg,"17 (TVGAIN3),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x18; sprintf(msg,"18 (TVGAIN4),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x19; sprintf(msg,"19 (TVGAIN5),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x1A; sprintf(msg,"1A (TVGAIN6),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x1B; sprintf(msg,"1B (INIT_GAIN),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x1C; sprintf(msg,"1C (FREQUENCY),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x1D; sprintf(msg,"1D (DEADTIME),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x1E; sprintf(msg,"1E (PULSE_P1),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x1F; sprintf(msg,"1F (PULSE_P2),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x20; sprintf(msg,"20 (CURR_LIM_P1),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x21; sprintf(msg,"21 (CURR_LIM_P2),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x22; sprintf(msg,"22 (REC_LENGTH),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x23; sprintf(msg,"23 (FREQ_DIAG),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x24; sprintf(msg,"24 (SAT_FDIAG_TH),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x25; sprintf(msg,"25 (FVOLT_DEC),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x26; sprintf(msg,"26 (DECPL_TEMP),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x27; sprintf(msg,"27 (DSP_SCALE),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x28; sprintf(msg,"28 (TEMP_TRIM),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x29; sprintf(msg,"29 (P1_GAIN_CTRL),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x2A; sprintf(msg,"2A (P2_GAIN_CTRL),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x2B; sprintf(msg,"2B (EE_CRC),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x40; sprintf(msg,"40 (EE_CNTRL),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x41; sprintf(msg,"41 (BPF_A2_MSB),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x42; sprintf(msg,"42 (BPF_A2_LSB),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x43; sprintf(msg,"43 (BPF_A3_MSB),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x44; sprintf(msg,"44 (BPF_A3_LSB),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x45; sprintf(msg,"45 (BPF_B1_MSB),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x46; sprintf(msg,"46 (BPF_B1_LSB),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x47; sprintf(msg,"47 (LPF_A2_MSB),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x48; sprintf(msg,"48 (LPF_A2_LSB),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x49; sprintf(msg,"49 (LPF_B1_MSB),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x4A; sprintf(msg,"4A (LPF_B1_LSB),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x4B; sprintf(msg,"4B (TEST_MUX),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x4C; sprintf(msg,"4C (DEV_STAT0),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x4D; sprintf(msg,"4D (DEV_STAT1),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x5F; sprintf(msg,"5F (P1_THR_0),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x60; sprintf(msg,"60 (P1_THR_1),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x61; sprintf(msg,"61 (P1_THR_2),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x62; sprintf(msg,"62 (P1_THR_3),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x63; sprintf(msg,"63 (P1_THR_4),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x64; sprintf(msg,"64 (P1_THR_5),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x65; sprintf(msg,"65 (P1_THR_6),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x66; sprintf(msg,"66 (P1_THR_7),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x67; sprintf(msg,"67 (P1_THR_8),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x68; sprintf(msg,"68 (P1_THR_9),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x69; sprintf(msg,"69 (P1_THR_10),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x6A; sprintf(msg,"6A (P1_THR_11),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x6B; sprintf(msg,"6B (P1_THR_12),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x6C; sprintf(msg,"6C (P1_THR_13),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x6D; sprintf(msg,"6D (P1_THR_14),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x6E; sprintf(msg,"6E (P1_THR_15),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x6F; sprintf(msg,"6F (P2_THR_0),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x70; sprintf(msg,"70 (P2_THR_1),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x71; sprintf(msg,"71 (P2_THR_2),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x72; sprintf(msg,"72 (P2_THR_3),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x73; sprintf(msg,"73 (P2_THR_4),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x74; sprintf(msg,"74 (P2_THR_5),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x75; sprintf(msg,"75 (P2_THR_6),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x76; sprintf(msg,"76 (P2_THR_7),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x77; sprintf(msg,"77 (P2_THR_8),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x78; sprintf(msg,"78 (P2_THR_9),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x79; sprintf(msg,"79 (P2_THR_10),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x7A; sprintf(msg,"7A (P2_THR_11),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x7B; sprintf(msg,"7B (P2_THR_12),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x7C; sprintf(msg,"7C (P2_THR_13),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x7D; sprintf(msg,"7D (P2_THR_14),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x7E; sprintf(msg,"7E (P2_THR_15),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	i = 0x7F; sprintf(msg,"7F (THR_CRC),%02X\r\n", SensorConfig.grid[i]); output_line(sdFileName, msg);
	sprintf(msg,"EOF\r\n"); output_line(sdFileName, msg);

}

int export_grid(void)
{
	  // SD Variables.
	  FIL sdFileName;
	  FRESULT sd_res;

	  sd_res = sd_init();  // Initialize the SD card and file system
	  if (sd_res != FR_OK) return sd_res;

	  // Open the file for writing
	  sd_res = f_open(&sdFileName, "GRID_OUT.TXT", FA_WRITE | FA_CREATE_ALWAYS);
	  if (sd_res != FR_OK) return sd_res;

	  // Write the lines
	  export_grid_lines(&sdFileName);

	  // Close SD file handle
	  f_close(&sdFileName);

	  return FR_OK;
}


static void PerformFactoryExportGrid(void)
{
	FRESULT fr;
	FIL fstatus;

    // Open file named "FactoryExportGrid.txt"
    fr = f_open(&fstatus, FACTORY_EXPORT_GRID_NAME, FA_READ);
    if (fr != FR_OK) return;  // no request to extract grid
	f_close(&fstatus);

    AppendStatusMessage("Performing factory export grid...");

    export_grid();

    AppendStatusMessage("Factory export grid complete.");
}
#endif // EXPORT_GRID



#if 1  // IMPORT_GRID


/*C program to convert hexadecimal Byte to integer.*/

//function : getNum
//this function will return number corresponding
//0,1,2..,9,A,B,C,D,E,F

int getNum(char ch)
{
    int num = 0;
    if (ch >= '0' && ch <= '9') {
        num = ch - 0x30;
    }
    else {
        switch (ch) {
        case 'A':
        case 'a':
            num = 10;
            break;
        case 'B':
        case 'b':
            num = 11;
            break;
        case 'C':
        case 'c':
            num = 12;
            break;
        case 'D':
        case 'd':
            num = 13;
            break;
        case 'E':
        case 'e':
            num = 14;
            break;
        case 'F':
        case 'f':
            num = 15;
            break;
        default:
            num = 0;
        }
    }
    return num;
}

unsigned int hex2int(char hex[])
{
    unsigned int x = 0;
    x = (getNum(hex[0])) * 16 + (getNum(hex[1]));
    return x;
}



TCHAR * parse_csv_text(char *pLine, char *pText, int Text_MaxLen);


//Record #,DateTime,Level,Distance,Gain,Temp
static int parse_banner(TCHAR * pLine)
{
	char label[32];

	pLine = parse_csv_text(pLine, label, sizeof(label));
    if (strcmp(label,";GRID_USER_MEMSPACE") != 0) return 0;  // unexpected label

    return 1;
}

static int import_grid_lines(FIL * pSDFile)
{
	TCHAR * pLine;
	char rtext[100]; /* File read buffer */
	char value[32];
	int i;
	int val;





	// Verify the headings

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_banner(pLine)) return 0;


	// Parse the remaining data lines
	pLine = f_gets(rtext, sizeof(rtext), pSDFile);
	while (pLine)
	{
        led_heartbeat();

#if 0
		// For development
    	trace_AppendMsg(pLine);
#endif

		// Register index and name
		pLine = parse_csv_text(pLine, value, sizeof(value));

	    if (strcmp(value,"EOF") == 0)
	    {
	    	return 1;  // end of file
	    }

		i = hex2int(value);

		// Value
		pLine = parse_csv_text(pLine, value, sizeof(value));
		val = hex2int(value);

		if (i < 128)
		{
		  SensorConfig.grid[i] = val;
		}

#if 0
		{
		  char debug[32];
		  sprintf(debug,"%02X  %02X\n", i, val); // for debug
    	  trace_AppendMsg(debug);
		}
#endif

        // Go to next line
    	pLine = f_gets(rtext, sizeof(rtext), pSDFile);
	}
	return 1;
}


int import_grid(void)
{
	  // SD Variables.
	  FIL sdFileName;
	  FRESULT sd_res;

	  sd_res = sd_init();  // Initialize the SD card and file system
	  if (sd_res != FR_OK) return sd_res;

	  // Open the file for reading
	  sd_res = f_open(&sdFileName, "GRID.TXT", FA_READ);
	  if (sd_res != FR_OK) return sd_res;

	  // Import the lines
	  import_grid_lines(&sdFileName);

	  // Close SD file handle
	  f_close(&sdFileName);

	  saveSensorConfig = 1;

	  return FR_OK;
}






static void PerformFactoryImportGrid(void)
{
	FRESULT fr;
	FIL fstatus;

    // Open file named "FactoryImportGrid.txt"
    fr = f_open(&fstatus, FACTORY_IMPORT_GRID_NAME, FA_READ);
    if (fr != FR_OK) return;  // no request to extract grid
	f_close(&fstatus);

    AppendStatusMessage("Performing factory import grid...");

    import_grid();

    AppendStatusMessage("Factory import grid complete.");
}

#endif // IMPORT_GRID





void factory_perform_sd_actions(void)
{

	if (sd_init() != FR_OK) return;  // nothing to do (e.g. no SD card present)

	//EnableTraceFunctions();  // hoisted to main.c to allow for flowtracker enhancements

	//PerformFactoryConfig();   // must do this first so f/w knows what hardware it is running on

	PerformFactoryWiFiExternal();

	PerformFactoryWiFiInternal();  // Internal will override external if both files are present on SD Card

	PerformFactoryCellOverride();

	if (isGen4())
	{
	  PerformFactoryResetZentri();

	  PerformFactoryEraseZentri();
	}

	PerformFactoryFillMemory();

	PerformFactoryLoadSampleDataFromCSV();

	PerformFactoryExtract();

	PerformFactoryImportGrid();

	PerformFactoryExportGrid();

	PerformFactoryTest();

	//factory_autoconfig_serial_number();  // zg - not compatible with actual manufacturing flow.  Serial number not assigned until hardware is functional.  MAC unknown until then.

}
