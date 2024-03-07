/*
 * dual_boot.c
 *
 *  Created on: Jan 2, 2020
 *      Author: zg
 */

#include "stdint.h"
#include "stdlib.h"  // atoi
#include "time.h"
#include "rtc.h"
#include "fatfs.h" // SD Card
#include "sdmmc.h" // SD Card
#include "dual_boot.h"
#include "bootloader.h"
#include "version.h"
#include "debug_helper.h"
#include "WiFi_Socket.h"
#include "Sample.h"
#include "trace.h"
#include "factory.h"
#include "s2c.h"
#include "quadspi.h"




void AppendStatusMessage(const char * Msg)
{

	FRESULT fr;
	FIL fstatus;
	UINT bytesWritten;

	//return; // Bug Hunt for esp32 power on


	if (sd_init() != FR_OK) return;  // nothing to do (e.g. no SD card present)

	fr = f_open(&fstatus, "FirmwareLog.txt", FA_OPEN_APPEND | FA_WRITE);
	if (fr != FR_OK)
	{
		return;
	}

	{
	uint32_t tmpsec;
	struct tm tm_st;
	char status_msg[256];


	// Get time stamp
	tmpsec = rtc_get_time();
	sec_to_dt(&tmpsec, &tm_st);

#if 0
	static int just_once = 1;
	if (just_once)
	{
		// Note when power on occurs in both FirmwareLog.txt and Trace.txt
		get_power_on_msg(status_msg);
		fr = f_write(&fstatus, status_msg, strlen(status_msg), &bytesWritten );
		trace_AppendMsg(status_msg);
		just_once = 0;
	}
#endif

	// Timestamped message (up to 100)

	//sprintf(status_msg, "SITE %s  %02d-%02d-%04d  %02d:%02d:%02d  %s\n", MainConfig.site_name, tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year, tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec, Msg);
	sprintf(status_msg, "%02d-%02d-%04d  %02d:%02d:%02d  %s\n", tm_st.tm_mon, tm_st.tm_mday, tm_st.tm_year, tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec, Msg);

	fr = f_write(&fstatus, status_msg, strlen(status_msg), &bytesWritten );
	}


	f_close(&fstatus);

}

#if 0
static void RenameForceLoadFile(void)
{
  //FRESULT sd_res;
  f_rename (FIRMWARE_FORCE_LOAD_ON_NAME, FIRMWARE_FORCE_LOAD_OFF_NAME);	// Rename the version.txt file to prevent infinite loops when auto-rebooting
}
#endif




static uint8_t IsNewerVersion(void)
{
	FIL SDFile;
	FRESULT sd_res;
	uint32_t bytesread=0;
	int major, minor, build;
	char rtext[100]; /* File read buffer */
    char msg[100];

    // Get firmware version from file named FirmwareVersion.txt
	sd_res = f_open(&SDFile, FIRMWARE_VERSION_NAME, FA_READ);
    if (sd_res != FR_OK) return 0;

	// Read text data from the file
	sd_res = f_read(&SDFile, rtext, sizeof(rtext), (void *) &bytesread);
    f_close(&SDFile);  // close file in any case
	if (sd_res != FR_OK) return 0;

	//parse the text
	rtext[bytesread] = 0;  // null-terminate

	parse_major_minor_build(rtext, &major, &minor, &build);

#if 0
	char * p;
	char * pend;
	char * pmajor;
	char * pminor;
	char * pbuild;
   	// parse major, minor, and build
    pend = rtext + strlen(rtext); // find the end
    p = rtext;
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
    major = atoi(pmajor);
    minor = atoi(pminor);
    build = atoi(pbuild);
#endif

	// Report version and if running from Bank 1 or Bank 2 THIS IS DONE EARLIER NOW...

	sprintf(msg, "Serial Number %s  MAC %s", MainConfig.serial_number, MainConfig.MAC);
	AppendStatusMessage(msg);

	sprintf(msg, "SD Card contains %d.%d.%d", major, minor, build);
	AppendStatusMessage(msg);



    if (major > VERSION_MAJOR) return 1;
    if (major < VERSION_MAJOR) return 0;

    if (minor > VERSION_MINOR) return 1;
    if (minor < VERSION_MINOR) return 0;

    if (build > VERSION_BUILD) return 1;
    if (build < VERSION_BUILD) return 0;

    return 0;  // versions are equal, don't install
}


static uint8_t IsForceLoadRequested(void)
{
	FIL SDFile;
	FRESULT sd_res;

    // Check if "ForceFirmwareLoad.ON" exists
	sd_res = f_open(&SDFile, FIRMWARE_FORCE_LOAD_ON_NAME, FA_READ);
    if (sd_res != FR_OK) return 0;

    f_close(&SDFile);  // close file in any case

	return 1;
}



static uint8_t IsFirmwareOnSDCard(FIL *pFirmwareFile, uint32_t * pAppSizeInBytes)
{
	// SD Variables.
	FRESULT sd_res;

	if (sd_init() != FR_OK) return 0;  // nothing to do (e.g. no SD card present)

	// Check if a firmware force load is requested
	if (IsForceLoadRequested())
	{
       // continue to load firmware without checking version
	}
	else
	{
		// Check FirmwareVersion.txt file for newer version
		if (!IsNewerVersion())
		{
			return 0;
		}
	}


    // Open file named "Firmware.bin"
    sd_res = f_open(pFirmwareFile, FIRMWARE_BINARY_NAME, FA_READ);
    if (sd_res != FR_OK) return 0;


    // Get size of application found on SD card
    *pAppSizeInBytes = f_size(pFirmwareFile);

#if 0

    if (Bootloader_CheckSize( *pAppSizeInBytes ) != BL_OK)
    {
    	AppendStatusMessage("Error: Firmware.bin is too large.");

        f_close(pFirmwareFile);

        return 0;
    }

    // Verify Checksum ??
#endif

    // Check for flash write protection
    if (Bootloader_GetProtectionStatus() & BL_PROTECTION_WRP)
    {
    	// Application space in flash is write protected.
    	f_close(pFirmwareFile);

    	HAL_led_delay(50);

    	// Disable write protection and generate system reset...
    	Bootloader_ConfigProtection(BL_PROTECTION_NONE);

    	// system resets here....if not, fail
    	return 0;
    }


    // Note, at this point:
    //
    //   1. SD card contains files indicating firmware is to be upgraded.
    //   2. Flash is not write protected.
    //   3. SD card is mounted.
    //   4. Binary file is open and will fit into one bank.

    //f_close(&SDFile);

    return 1;
}

uint32_t GetBFB2(void)
{

	FLASH_OBProgramInitTypeDef OBProgramInit;

	HAL_FLASHEx_OBGetConfig(&OBProgramInit);

	return  (OBProgramInit.USERConfig & FLASH_OPTR_BFB2_Msk);  // either  OB_BFB2_DISABLE or OB_BFB2_ENABLE
}

//
//  USERConfig must be:
//    OB_BFB2_DISABLE or
//    OB_BFB2_ENABLE
static void SetBFB2(uint32_t USERConfig)
{
	HAL_StatusTypeDef status;
	FLASH_OBProgramInitTypeDef OBProgramInit;

	status = HAL_FLASH_Unlock();
	status |= HAL_FLASH_OB_Unlock();
	if (status == HAL_OK)
	{

		OBProgramInit.OptionType = OPTIONBYTE_USER;
		OBProgramInit.USERType   = OB_USER_BFB2;
		OBProgramInit.USERConfig = USERConfig;  // OB_BFB2_DISABLE or OB_BFB2_ENABLE

		OBProgramInit.PCROPConfig = 0;
		OBProgramInit.PCROPEndAddr = 0;
		OBProgramInit.PCROPStartAddr = 0;
		OBProgramInit.RDPLevel = 0;
		OBProgramInit.WRPArea = 0;
		OBProgramInit.WRPEndOffset = 0;
		OBProgramInit.WRPStartOffset = 0;

		status |= HAL_FLASHEx_OBProgram(&OBProgramInit);

#if 0  // zg this doesn't seem to work...
	    if (status == HAL_OK)
	    {
	        /* Loading Flash Option Bytes - this generates a system reset. */
	        status |= HAL_FLASH_OB_Launch();
	    }
	    // Should never get here...
#endif



	    status |= HAL_FLASH_OB_Lock();
	    status |= HAL_FLASH_Lock();
	}
}




/*** Bootloader ***************************************************************/
static void FlashNewFirmware(FIL *pFirmwareFile, uint32_t AppSizeInBytes)
{
    FRESULT  fr;
    UINT     num;

    uint8_t  status;
    uint64_t data;
    uint32_t cntr = 0;

    uint32_t bank;
    char     msg[100] = {0x00};
    int prev_pattern;
    uint16_t raw_acc_at_start;
    uint16_t raw_acc_at_end;

    // Note: on entry, the SD card is mounted and the firmware file is open

    raw_acc_at_start = ltc2943_get_raw_accumulator();

    prev_pattern = led_set_pattern(LED_PATTERN_FAST_RED);

    // Flash the bank we are not running in
	if  (GetBFB2() == OB_BFB2_ENABLE)
	{
	    bank = FLASH_BANK_1;
	}
	else
	{
	    bank = FLASH_BANK_2;
	}



    // Step 1: Init Bootloader and Flash
    //AppendStatusMessage("Init Bootloader");

    Bootloader_Init();

    // Step 2: Erase Flash
    AppendStatusMessage("Erasing flash...");

    //Bootloader_Erase();
    Bootloader_EraseBank(bank, AppSizeInBytes);


    //AppendStatusMessage("Flash erase finished.");


    // Step 3: Programming
    AppendStatusMessage("Programming flash...");


	Bootloader_FlashBeginBank(bank);

    do
    {
        data = 0xFFFFFFFFFFFFFFFF;
        fr = f_read(pFirmwareFile, &data, 8, &num);
        if (fr != FR_OK)
        {
            /* f_read failed */
            f_close(pFirmwareFile);

            sprintf(msg, "f_read failed error: %u", fr);
            AppendStatusMessage(msg);
            led_set_pattern(prev_pattern);
            return;
        }

        if (num)
        {
            status = Bootloader_FlashNext(data);
            //status = BL_OK;  // zg for debug
            if (status == BL_OK)
            {
                cntr++;
            }
            else
            {
                f_close(pFirmwareFile);

            	sprintf(msg, "Flash error at: %lu", (cntr*8));
                AppendStatusMessage(msg);
                led_set_pattern(prev_pattern);
                return;
            }
        }
        //led_heartbeat();
        if (cntr % 256 == 0)
        {
            /* Toggle LED during programming */
        	STATUS_LED_TOGGLE;
        }

    } while ((fr == FR_OK) && (num == 8));


    /* Step 4: Finalize Programming */
    Bootloader_FlashEnd();

    ltc2943_get_voltage(0);  // attempt to read a bunch of info from the chip (chip may not be online)

    raw_acc_at_end = ltc2943_last_raw_acc_reported;

    if (ltc2943_are_batteries_present())
    {
    	float new_value;
    	new_value = raw_acc_at_end * Qlsb;
    	if (LogTrack.accumulator > new_value)
    	{
    		// Save the updated value
    		LogTrack.accumulator = new_value;
    		BSP_Log_Track_Write(0);  // Do not read the LTC2943
    	}
    }


    sprintf(msg, "Flashed %lu bytes. Firmware installed OK.  bacc %u, %u, %u.", (cntr*8), raw_acc_at_start, raw_acc_at_end, (raw_acc_at_start - raw_acc_at_end));
    AppendStatusMessage(msg);

#if 0
    // This verification step is in addition to the word-by-word verification already done
    // rewind file pointer for verification step
    fr = f_lseek (pFirmwareFile, 0);
    if (fr != FR_OK)
    {
        /* f_lseek failed */

        sprintf(msg, "f_lseek failed error: %u", fr);
        led_set_pattern(prev_pattern);

        return;
    }

#if 0
    /* Open file for verification */
    fr = f_open(pFirmwareFile, BINARY_FILENAME, FA_READ);
    if (fr != FR_OK)
    {
        /* f_open failed */

        sprintf(msg, "FatFs error code: %u", fr);
        led_set_pattern(prev_pattern);

        return;
    }
#endif

    /* Step 5: Verify Flash Content */
    uint32_t addr;
    cntr = 0;
    addr = Bootloader_GetBankAddress(bank);
    do
    {
        data = 0xFFFFFFFFFFFFFFFF;
        fr = f_read(pFirmwareFile, &data, 4, &num);
        if (num)
        {
            if (*(uint32_t*)addr == (uint32_t)data)
        	//if (1) // zg debug
            {
                addr += 4;
                cntr++;
            }
            else
            {
                sprintf(msg, "Verification error at: %lu", (cntr*4));
                AppendStatusMessage(msg);

                f_close(pFirmwareFile);

                led_set_pattern(prev_pattern);
                return;
            }
        }
        //led_heartbeat();
        if (cntr % 256 == 0)
        {
            /* Toggle LED during programming */
        	STATUS_LED_TOGGLE;
        }

    } while((fr == FR_OK) && (num > 0));

    AppendStatusMessage("Verification passed.  Firmware installed OK.");
#endif



    f_close(pFirmwareFile);

    // Set option bit to boot from the newly flashed bank at next power cycle

	if  (GetBFB2() == OB_BFB2_ENABLE)
	{
	    SetBFB2(OB_BFB2_DISABLE);
	}
	else
	{
	    SetBFB2(OB_BFB2_ENABLE);
	}

	//Wait and then initiate a reset ??


    // Enable flash write protection ??
#if (USE_WRITE_PROTECTION)
    print("Enablig flash write protection and generating system reset...");
    if (Bootloader_ConfigProtection(BL_PROTECTION_WRP) != BL_OK)
    {
        print("Failed to enable write protection.");
        print("Exiting Bootloader.");
    }
#endif

    led_set_pattern(prev_pattern);
}



void Bootload(void)
{
	FIL FirmwareFile;
	uint32_t AppSizeInBytes;

    if (IsFirmwareOnSDCard(&FirmwareFile, &AppSizeInBytes))
    {
    	AppendStatusMessage("New firmware found.");
    	FlashNewFirmware(&FirmwareFile, AppSizeInBytes);
        // When forcing an older version of firmware to overwrite a newer version
    	// it was easy to put a bogus, but larger, firmware version into the
    	// FirmwareVersion.txt file.  It was assumed the card would be removed and
    	// a manual power-cycle would occur.  However, that can lead to an infinite
    	// loop if the hardware reset is done automatically.
    	// A better approach is to introduce a "ForceFirmwareLoad.ON" file that
    	// is automatically renamed to "ForceFirmwareLoad.OFF" once used.  That will
    	// force the user to remove the SD card and rename the file to get the
    	// force function to work again.
    	//RenameForceLoadFile();  // This is intended to prevent infinite loops when the system resets itself.
		//HAL_NVIC_SystemReset();   // This does NOT work reliably.  Just force the user to do a manual power-cycle just as in Gen4
    }

}


