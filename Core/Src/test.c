/*
 * test.c
 *
 *  Created on: Jul 16, 2021
 *     Purpose: A place to hold various test code snippets
 */
#include "main.h"
#include "debug_helper.h" // For debugging and hacking around when testing, etc.
#include "rtc.h"
#include "adc.h"
#include "i2c.h"
#include "iwdg.h"
#include "quadspi.h"
#include "rtc.h"
#include "usart.h"
#include "uart1.h"
#include "uart2.h"
#include "uart3.h"
#include "gpio.h"
#include "Common.h"
#include "Gen4_max5418.h"
#include "WiFi.h"
#include "s25fl.h"
#include "Sensor.h"
#include "Log.h"
#include "DeltaQ.h"
#include "dual_boot.h"
#include "Sample.h"
#include "s2c.h"
#include "trace.h"
#include "factory.h"
#include "init.h"
#include "Hydro.h"
#include "testalert.h"
#include "pass_thru.h"
#include "usb_device.h"

static	uint32_t cm, cu, wm, wu;    // a bit-field can only hold 4 bits (16 values)

static void report_bit_config(void)
{

	char buffer[80];

	cm = GetCellModule();
	cu = GetRTCBatt();
	wm = GetWiFiModule();
	wu = GetWiFiUart();

	sprintf(buffer, "%lu, %lu, %lu, %lu\n", cm, cu, wm, wu);
#ifdef ENABLE_USB
    CDC_Transmit_FS((uint8_t *)buffer, sizeof(buffer));
#endif
    buffer[0] = 0;

}


static void set_values(int i)
{
#if 1
	SetCellModule(i);
	SetRTCBatt(i+1);
	SetWiFiModule(i+2);
	SetWiFiUart(i+3);
	//MainConfig.wifi_wait_connect;   // for debugging
#else


	//MainConfig.wifi_wait_connect = 0x12340000;
	MainConfig.wifi_wait_connect = 0x43210000;
	//MainConfig.wifi_wait_connect = 0x11110000;

	cm = MainConfig.wifi_wait_connect & 0xF0000000;
	cm = cm >> 28;
	cm = 0;
#endif
}

static void TestMainConfigBitFields(void)
{
	int i = 0;
	MainConfig.wifi_wait_connect = 60;
	while (1)
	{
		report_bit_config();
		set_values(i);
		i+=4;
		if (i>12) i = 0;
		//HAL_led_delay(2000);
	}

}



static void test_rtc(void)
{
	struct tm tm_st;
	char time_str[TIME_STR_SIZE];

#if 1  // Use this to set the clock to a known date/time
	extern void parse_datetime(char * strdate, struct tm * tm_st);  // Parses a string of the form "8/27/2014 13:15"  or "8/27/2014 13:15:32"
	parse_datetime("9/16/2022 14:44:30", &tm_st);
	rtc_sync_time(&tm_st);   // Cellular also syncs to the network UTC at power on.
#endif

	get_current_time_str(time_str);
}

void test(void)
{
	UNUSED(TestMainConfigBitFields);
	UNUSED(test_rtc);

    //TestMainConfigBitFields();

	//explain_tso();
	//while(1);

	//void sweep_old_code(void);
	//sweep_old_code();
	//while(1);

	//extern int find_midpoint(void);
	//int mid;
	//mid = find_midpoint();
	//while(1);

	//void test_sensor(void);
	//tim2_min_CNT = 8080705;
	//tim2_max_CNT = 0;
	//test_sensor();
	//while (1);

	//test_sensor_2();
	//void test_new_binary_search(void);
	//test_new_binary_search();
	//while (1);

	//extern void test_binary_search(void);
	//test_binary_search();
	//test_binary_search();
	//trace_SaveBuffer();
	//while (1);

	//extern void test_parse(void);
	//test_parse();
	//test_byte();
	//while (1);

	//extern uint32_t testflash(void);
	//testflash();
	//loadSampleData(2);
	//viewData();
	//wifi_send_history(1, 30);
	//loadSampleData(SAMPLE_DATA_FILL_MEMORY);
	//time_it();
	//extern void test_wifi_wakeup(void);
	//test_wifi_wakeup();
	//s2c_Test();
	//while (1);
	//{
	//	led_action_heartbeat(1);
	//	HAL_Delay(1000);
    //}
	//Hydro_ProcessImportedData();
	//qratio_test();

	//time_rtc();

	//while (1)
	//{
	//	test_rtc();  // use this to set the clock and verify it is set
	//}

	//test_laird();

	while (1);

}


