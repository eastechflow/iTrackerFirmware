/*
 * factory.h
 *
 *  Created on: Mar 24, 2020
 *      Author: Zachary
 */

#ifndef INC_FACTORY_H_
#define INC_FACTORY_H_

// Factory features accessible from SD card:

#define FIRMWARE_BINARY_NAME           "Firmware.bin"
#define FIRMWARE_VERSION_NAME          "FirmwareVersion.txt"    // format 4 0 0
#define FIRMWARE_FORCE_LOAD_ON_NAME    "ForceFirmwareLoad.ON"   //
#define FIRMWARE_FORCE_LOAD_OFF_NAME   "ForceFirmwareLoad.OFF"  // used to prevent infinite loops when force loading from SD card.  File is renamed.

#define FACTORY_WEB_APP_MANIFEST_NAME     "Manifest.txt"    // first line: 4 0 0 (version code); next lines: Filename or d Filename  (d = delete)
// Note: on the SD card all WebApp files reside in a directory named WebApp
// On the Zentri, the files reside at root.

#define FACTORY_CONFIG_NAME               "FactoryConfig.txt"
#define FACTORY_CELL_OVERRIDE_NAME        "FactoryCellOverride.txt"
#define FACTORY_WIFI_EXTERNAL_NAME        "FactoryWiFiExternal.txt"  // force WiFi external antenna
#define FACTORY_WIFI_INTERNAL_NAME        "FactoryWiFiInternal.txt"  // force WiFi internal antenna, if both present this overrides
#define FACTORY_EXTRACT_ENABLE_NAME       "FactoryExtract.txt"
#define FACTORY_FILLMEM_ENABLE_NAME       "FactoryFillMemory.txt"
#define FACTORY_LOAD_SAMPLE_DATA_NAME     "FactoryLoadSampleData.txt"   // will also trigger HydroDecomp output file to be written
#define FACTORY_RESET_ZENTRI_ENABLE_NAME  "FactoryResetZentri.txt"
#define FACTORY_ERASE_ZENTRI_ENABLE_NAME  "FactoryEraseZentri.txt"
#define FACTORY_TEST                      "FactoryTest.txt"     // iTracker001

#define FACTORY_IMPORT_GRID_NAME       "FactoryImportGrid.txt"
#define FACTORY_EXPORT_GRID_NAME       "FactoryExportGrid.txt"

#define ENABLE_UART_SIZE_LOG_NAME    "Enable_UartSizeLog.txt"
#define ENABLE_WIFI_WAKE_LOG_NAME    "Enable_WakeLog.txt"
#define ENABLE_WIFI_CMD_LOG_NAME     "Enable_WiFiCmdLog.txt"
#define ENABLE_WIFI_DATA_LOG_NAME    "Enable_WiFiDataLog.txt"
#define ENABLE_CELL_CMD_LOG_NAME     "Enable_CellCmdLog.txt"
#define ENABLE_CELL_DATA_LOG_NAME    "Enable_CellDataLog.txt"
#define ENABLE_WIFI_SLEEP_AT_POWERON "Enable_WiFiSleepAtPowerOn.txt"
#define DISABLE_DATA_LOGGING         "Disable_DataLogging.txt"
#define DISABLE_ESP32                "Disable_ESP32.txt"
#define DISABLE_CELL                 "Disable_Cell.txt"
#define FORCE_ITRACKER               "Force_iTracker.txt"
#define FORCE_FIRSTRESPONSE          "Force_FirstResponse.txt"
#define FORCE_SEWERWATCH             "Force_SewerWatch.txt"
#define FORCE_SMARTFLUME             "Force_SmartFlume.txt"


#define ENABLE_EVM_1000               "Enable_EVM_1000.txt"
#define ENABLE_EVM_460                "Enable_EVM_460.txt"
#define ENABLE_EVM_460_TRACE          "Enable_EVM_460_Trace.txt"
#define ENABLE_UART3_ECHO             "Enable_UART3_Echo.txt"
#define ENABLE_UART1_TEST             "Enable_UART1_Test.txt"
#define ENABLE_LPUART1_TEST           "Enable_LPUART1_Test.txt"

#define ENABLE_UART_1SPY5             "Enable_UART_1SPY5.txt"    // UART1 (CELL) spies on UART5 (VEL2) used to test cables
#define ENABLE_UART_5SPY1             "Enable_UART_5SPY1.txt"    // UART5 (VEL2) spies on UART1 (CELL) might be useful for CELL testing
#define ENABLE_UART_2SPY4             "Enable_UART_2SPY4.txt"    // UART2 (WIFI) spies on UART4 (VEL1) used to test cables
#define ENABLE_UART_4SPY2             "Enable_UART_4SPY2.txt"    // UART4 (VEL1) spies on UART2 (WIFI)  useful for BT testing with a console to observe traffic


#define ENABLE_TEST_CELL_DATA_NAME    "Enable_TestCellData.txt"
#define ENABLE_TEST_CELL_ALERTS_NAME  "Enable_TestCellAlerts.txt"
#define ENABLE_DEMO_CELL_LOOP_NAME    "Enable_DemoCellLoop.txt"
#define ENABLE_TEST_CELL_NO_TX_NAME   "Enable_TestCellNoTx.txt"

#define ENABLE_TRACE_SENSOR_NAME      "Enable_TraceSensor.txt"
#define ENABLE_TRACE_VOLTAGE_NAME     "Enable_TraceVoltage.txt"

#define ENABLE_STRESS_TEST_NAME       "Enable_StressTest.txt"

#define FACTORY_MAC_TO_SN            "Factory_MAC_to_SN.txt"     // format 00:00:00:00:00:00 sn



extern int sd_RunningFromBank2;
extern int sd_UartBufferSizeLogEnabled;
extern int sd_WiFiWakeLogEnabled;
extern int sd_WiFiCmdLogEnabled;
extern int sd_WiFiDataLogEnabled;
extern int sd_TraceVoltageEnabled;
extern int sd_CellCmdLogEnabled;
extern int sd_CellDataLogEnabled;
extern int sd_SleepAtPowerOnEnabled;
extern int sd_DataLoggingEnabled;
extern int sd_ESP32_Enabled;
extern int sd_ForceType;
extern int sd_Disable_Cell;


extern int sd_EVM_1000;      //   9600, 8, N, 1
extern int sd_EVM_460;       //  38400, 8, N, 2
extern int sd_CMD_460;       // 115200, 8, N, 2
extern int sd_EVM_460_Trace; //
extern int sd_UART3_Echo;    // 115200, 8, N, 2

extern int sd_UART1_Test;          // 115200, 8, N, 1   CELL
extern int sd_UART2_Test;          // 115200, 8, N, 1   WIFI
extern int sd_UART3_Test;          // 115200, 8, N, 1   RS-485
extern int sd_UART4_Test;          // 115200, 8, N, 1   VELOCITY 1
extern int sd_LPUART1_Test;        // 115200, 8, N, 1   VELOCITY 2

extern int sd_UART_1SPY5;        // UART1 (CELL) spies on UART5 (VEL2) used to test cables
extern int sd_UART_5SPY1;        // UART5 (VEL2) spies on UART1 (CELL) might be useful for CELL testing
extern int sd_UART_5SPY2;        // UART5 (VEL2) spies on UART2 (WIFI) used for ESP32 bootload file capture
extern int sd_UART_2SPY4;        // UART2 (WIFI) spies on UART4 (VEL1) used to test cables
extern int sd_UART_4SPY2;        // UART4 (VEL1) spies on UART2 (WIFI)  useful for BT testing with a console to observe traffic


extern int sd_TestCellData;
extern int sd_TestCellAlerts;
extern int sd_DemoCellLoop;
extern int sd_TestCellNoTx;

//extern int sd_UseOldCellProtocol;
extern int sd_TraceSensor;
extern int sd_StressTest;
extern int sd_FactoryTest;




#define BASE_SSID_MAX  16
#define MAX_SSID_LEN   32   // maximum allowable SSID

#define FACTORY_TEST_IN_PROGRESS  1
#define FACTORY_TEST_PASSED       2
#define FACTORY_TEST_FAILED       3

typedef struct {
	char     base_ssid[BASE_SSID_MAX];
	uint16_t number_of_readings;
	uint8_t  min_gain;
	uint8_t  max_gain;
	float    min_distance;
	float    max_distance;
	int      status;
} FactoryTest_t;


extern FactoryTest_t FactoryTest;


void EnableTraceFunctions(void);
void get_power_on_msg(char * msg);
void AppendStatusMessage(const char * Msg);
void factory_perform_sd_actions(void);

void parse_major_minor_build(char *text, int *major, int *minor, int *build);
uint8_t get_line(FIL * pSDFile, char *pLine, int maxlen);
uint8_t get_line_skip_comments(FIL * pSDFile, char *pLine, int maxlen);





#endif /* INC_FACTORY_H_ */
