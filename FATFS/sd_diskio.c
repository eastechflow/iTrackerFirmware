#include "ff_gen_drv.h"
#include "sd_diskio.h"

#define SD_TIMEOUT 30 * 1000
#define SD_DEFAULT_BLOCK_SIZE 512

static volatile DSTATUS Stat = STA_NOINIT;
static volatile UINT WriteStatus = 0, ReadStatus = 0;
static DSTATUS SD_CheckStatus(BYTE lun);
DSTATUS SD_initialize(BYTE);
DSTATUS SD_status(BYTE);
DRESULT SD_read(BYTE, BYTE*, DWORD, UINT);
#if _USE_WRITE == 1
DRESULT SD_write(BYTE, const BYTE*, DWORD, UINT);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
DRESULT SD_ioctl(BYTE, BYTE, void*);
#endif  /* _USE_IOCTL == 1 */

const Diskio_drvTypeDef SD_Driver = { SD_initialize, SD_status, SD_read,
#if  _USE_WRITE == 1
    SD_write,
#endif /* _USE_WRITE == 1 */

#if  _USE_IOCTL == 1
    SD_ioctl,
#endif /* _USE_IOCTL == 1 */
    };

static DSTATUS SD_CheckStatus(BYTE lun) {
	Stat = STA_NOINIT;

	if (BSP_SD_GetCardState() == MSD_OK) {
		Stat &= ~STA_NOINIT;
	}

	return Stat;
}

DSTATUS SD_initialize(BYTE lun) {
	Stat = STA_NOINIT;
#if !defined(DISABLE_SD_INIT)

	if (BSP_SD_Init() == MSD_OK) {
		Stat = SD_CheckStatus(lun);
	}

#else
	Stat = SD_CheckStatus(lun);
#endif
	return Stat;
}

/**
 * @brief  Gets Disk Status
 * @param  lun : not used
 * @retval DSTATUS: Operation status
 */
DSTATUS SD_status(BYTE lun) {
	return SD_CheckStatus(lun);
}

/* USER CODE BEGIN beforeReadSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeReadSection */
/**
 * @brief  Reads Sector(s)
 * @param  lun : not used
 * @param  *buff: Data buffer to store read data
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to read (1..128)
 * @retval DRESULT: Operation result
 */
DRESULT SD_read(BYTE lun, BYTE *buff, DWORD sector, UINT count) {
	DRESULT res = RES_ERROR;
	ReadStatus = 0;

#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
	uint32_t alignedAddr;
#endif

	if (BSP_SD_ReadBlocks((uint32_t*) buff, (uint32_t) (sector), count, SDMMC_DATATIMEOUT) == MSD_OK) {
		/* wait until the read operation is finished */
		while (BSP_SD_GetCardState() != MSD_OK) {
		}
		res = RES_OK;
	}

	return res;
}

/* USER CODE BEGIN beforeWriteSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeWriteSection */
/**
 * @brief  Writes Sector(s)
 * @param  lun : not used
 * @param  *buff: Data to be written
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to write (1..128)
 * @retval DRESULT: Operation result
 */
#if _USE_WRITE == 1
DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count) {
	DRESULT res = RES_ERROR;
	WriteStatus = 0;

#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
	uint32_t alignedAddr;
	/*
	 the SCB_CleanDCache_by_Addr() requires a 32-Byte aligned address
	 adjust the address and the D-Cache size to clean accordingly.
	 */
	alignedAddr = (uint32_t)buff & ~0x1F;
	SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));
#endif

	if (BSP_SD_WriteBlocks((uint32_t*) buff, (uint32_t) (sector), count, SDMMC_DATATIMEOUT) == MSD_OK) {
		/* wait until the Write operation is finished */
		while (BSP_SD_GetCardState() != MSD_OK) {
		}
		res = RES_OK;
	}

	return res;
}
#endif /* _USE_WRITE == 1 */

/* USER CODE BEGIN beforeIoctlSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeIoctlSection */
/**
 * @brief  I/O control operation
 * @param  lun : not used
 * @param  cmd: Control code
 * @param  *buff: Buffer to send/receive control data
 * @retval DRESULT: Operation result
 */
#if _USE_IOCTL == 1
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff) {
	DRESULT res = RES_ERROR;
	BSP_SD_CardInfo CardInfo;

	if (Stat & STA_NOINIT)
		return RES_NOTRDY;

	switch (cmd) {
		/* Make sure that no pending write process */
		case CTRL_SYNC:
			res = RES_OK;
			break;

			/* Get number of sectors on the disk (DWORD) */
		case GET_SECTOR_COUNT:
			BSP_SD_GetCardInfo(&CardInfo);
			*(DWORD*) buff = CardInfo.LogBlockNbr;
			res = RES_OK;
			break;

			/* Get R/W sector size (WORD) */
		case GET_SECTOR_SIZE:
			BSP_SD_GetCardInfo(&CardInfo);
			*(WORD*) buff = CardInfo.LogBlockSize;
			res = RES_OK;
			break;

			/* Get erase block size in unit of sector (DWORD) */
		case GET_BLOCK_SIZE:
			BSP_SD_GetCardInfo(&CardInfo);
			*(DWORD*) buff = CardInfo.LogBlockSize / SD_DEFAULT_BLOCK_SIZE;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
	}

	return res;
}
#endif /* _USE_IOCTL == 1 */

void BSP_SD_WriteCpltCallback(void) {
	WriteStatus = 1;
}

void BSP_SD_ReadCpltCallback(void) {
	ReadStatus = 1;
}
