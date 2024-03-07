/*
 * testflash.c
 *
 *  Created on: Jan 19, 2020
 *      Author: Zachary
 */

#include "main.h"
#include "Common.h"
#include "rtc.h"
#include "LTC2943.h"
#include "WiFi_Socket.h"
#include "WiFi.h"
#include "Log.h"
#include "Sample.h"
#include "DeltaQ.h"
#include "Sensor.h"
#include "debug_helper.h"
#include "time.h"

#include "quadspi.h"
#include "s25fl.h"

#include "testflash.h"

#define SIXTY_FOUR_K  65536
#define FOUR_K         4096

#if 0
static uint8_t isErased(uint8_t * pSectorData)
{
	int i;
	for (i=0;i<32;i++)
	{
		if (pSectorData[i] != 0xFF) return 0;
	}
	return 1;
}
#endif

#if 0
static uint8_t isMatch(uint8_t * pData1, uint8_t * pData2)
{
	int i;
	for (i=0;i<32;i++)
	{
		if (pData1[i] != pData2[1]) return 0;
	}
	return 1;
}

static void createKnownData(uint8_t * pSectorData, uint8_t KnownValue)
{
	int i;
	for (i=0;i<32;i++)
	{
		pSectorData[i] = KnownValue;
	}
}
#endif

static void fillWithAddr(uint8_t * pSectorData, uint32_t Addr)
{
	int i;
	uint32_t * p;
	p = (uint32_t *)pSectorData;

	for (i=0;i<8;i++)
	{
		p[i] = Addr;
	}
}
static uint8_t isMatchAddr(uint8_t * pSectorData, uint32_t Addr)
{
	int i;
	uint32_t * p;
	p = (uint32_t *)pSectorData;

	for (i=0;i<8;i++)
	{
		if (p[i] != Addr) return 0;
	}
	return 1;
}

static uint8_t write_and_verify(uint32_t StartAddr, uint32_t Length)
{
	uint8_t status;
	uint32_t nextAddr;
	uint32_t stopAddr;
	uint8_t known_data[32];
	uint8_t sector_data[32];

	//createKnownData(known_data, 0x55);

	nextAddr = StartAddr;
	stopAddr = StartAddr + Length;

	while (nextAddr < stopAddr)
	{
		fillWithAddr(known_data, nextAddr);

		//status = BSP_QSPI_Write((uint8_t *) known_data, nextAddr, sizeof(known_data));

		// Kludge Introduce a known error to verify if test function works
		//if (nextAddr == LOG_BOTTOM)  known_data[0] = 'z';
		//if (nextAddr == LOG_BOTTOM + sizeof(known_data))  known_data[0] = 'z';

		status = BSP_Log_Write( known_data, nextAddr, sizeof(known_data));

		if (status != QSPI_OK)
		{
			return 0;
		}

		nextAddr += sizeof(known_data);
	}


	// Verify the pattern was indeed written
	nextAddr = StartAddr;
	stopAddr = StartAddr + Length;

	while (nextAddr < stopAddr)
	{
		log_readstream_ptr((log_t *)sector_data, nextAddr);

	    if (!isMatchAddr(sector_data, nextAddr))
		{
	    	//return 1; // kludge to force a known error
          return 0;
		}

		nextAddr += sizeof(known_data);
	}

  return 1;
}

#if 0
static uint8_t erase_and_verify(uint32_t StartAddr, uint32_t Length)
{

	uint32_t instruction;
	uint32_t nextAddr;
	uint32_t stopAddr;
	uint8_t known_data[32];
	uint8_t sector_data[32];

	createKnownData(known_data, 0xFF);

	if (Length == SIXTY_FOUR_K)
	{
		instruction = FLASH_CMD_SE; // 64K
	}
	else
	{
		instruction = FLASH_CMD_P4E;  // 4K
	}
	if (QSPI_OK != BSP_QSPI_Erase_Sector2(StartAddr, instruction))
	{
		return 0;
	}

	// Verify the memory was indeed erased (set to 0xFF)
	nextAddr = StartAddr;
	stopAddr = StartAddr + Length;

	while (nextAddr < stopAddr)
	{
		if (QSPI_OK != log_readstream_ptr((log_t *)&sector_data, nextAddr))
		{
			return 0;
		}

		if (!isMatch(known_data, sector_data))
		{
			return 0;
		}

		nextAddr += sizeof(known_data);
	}

	return 1;

}
#endif

//sCommand.Instruction       = FLASH_CMD_P4E;
//sCommand.Instruction       = FLASH_CMD_SE;

#if 0

uint32_t testflash(void)
{
#if 0
	uint8_t status;
	uint8_t known_data[32];

	uint32_t readAddr;
	uint32_t stopAddr;
	uint32_t not_erased_Addr = 0;
#endif
	uint32_t testAddr;
	uint8_t sector_data[32];

#if 1
	if (!erase_and_verify(LOG_BOTTOM, SIXTY_FOUR_K))
	{
		return 0;
	}
	if (!write_and_verify(LOG_BOTTOM, SIXTY_FOUR_K))
	{
		return 0;
	}

	testAddr = LOG_BOTTOM;

	// read contents of first log data sector
	log_readstream_ptr((log_t *)&sector_data, testAddr);

	if (!erase_and_verify(testAddr, FOUR_K))
	{
		return 0;
	}

	// read contents of first log data sector
	log_readstream_ptr((log_t *)&sector_data, testAddr);

	// read contents of second 4K log data sector, should not be erased
	log_readstream_ptr((log_t *)&sector_data, testAddr + FOUR_K);

	// advance by one 4K sector
	testAddr = LOG_BOTTOM + FOUR_K;

	if (!erase_and_verify(testAddr, FOUR_K))
	{
		return 0;
	}

	// read contents of first log data sector
	log_readstream_ptr((log_t *)&sector_data, testAddr);

	// read contents of next 4K log data sector, should not be erased
	log_readstream_ptr((log_t *)&sector_data, testAddr + FOUR_K);

	#endif

#if 0
	createKnownData(known_data);

	// read contents of first log data sector
	log_readstream_ptr((log_t *)&sector_data, LOG_BOTTOM);

	status = BSP_QSPI_Erase_Sector(LOG_BOTTOM);
	if (status != QSPI_OK)
	{
		return;
	}

	// Verify the bottom sector is erased...
	readAddr = LOG_BOTTOM;
	stopAddr = readAddr + SIXTY_FOUR_K;

	while (readAddr < stopAddr)
	{
		log_readstream_ptr((log_t *)&sector_data, readAddr);


		if (!isErased(sector_data))
		{
			not_erased_Addr = readAddr;
			break;
		}
		readAddr += sizeof(sector_data);
	}

    // Write known contents to first log data sector
	BSP_Log_Write((uint8_t *) &known_data, LOG_BOTTOM, LOG_SIZE);

	// read contents of first log data sector
	log_readstream_ptr((log_t *)&sector_data, LOG_BOTTOM);

	return not_erased_Addr;
#endif
	return 0;

}

#endif


#if 1  // This tests the read cache

static uint32_t test_reverse_cache(uint32_t addr)  // Note: can return zero if no entries
{
    uint32_t expected_addr;
    //log_t    logrec;
	uint8_t sector_data[32];

    if (addr == 0) return 0;  // there are no log entries at all
    expected_addr = addr;

    // Search backwards through the log records
    log_EnumStart(addr, LOG_REVERSE);
    //while (log_EnumGetNextRecord(&logrec))
    while (log_EnumGetNextRecord((log_t *)sector_data, LogTrack.next_logrec_addr))
    {

    	addr = log_EnumGetLastAddr();  // grab address of current record
    	if (addr != expected_addr)
    	{
    		return 0;
    	}

    	// test if the expected contents are found in the retrieved record
	    if (!isMatchAddr(sector_data, expected_addr))
		{
          return 0;
		}

	    expected_addr -= sizeof(log_t);

    }

	log_EnumStop();

	return 1;  // All of the records matched the expected contents going backwards
}

uint32_t testflash(void)
{

    // This will fill the log with test data as it goes along...no need to erase entire chip

	if (!write_and_verify(LOG_BOTTOM, SECTOR_SIZE * 3L))
	{
		return 0;
	}

	if (!test_reverse_cache(LOG_BOTTOM + ((SECTOR_SIZE * 3L) - sizeof(log_t) ) ))
	{
		return 0;
	}


	// Leave the data logging system in a known good state
	log_start(0);
	//Delta_Q_Reset();

    return 1;

	//testAddr = LOG_BOTTOM;
}

#endif




