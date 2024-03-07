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

#ifndef INC_S25FL_H_
#define INC_S25FL_H_

//Define memory type (must be S25FL128S or S25FL256S)
#define S25FL256S

#define S25FL128S_ID                    0x00000117
#define S25FL256S_ID                    0x00000118

#if defined(S25FL128S)
//S25FL128 ID
#define S25FL_ID    S25FL128S_ID
//--- S25FL128 Capacity is 16,777,216 Bytes ---/
//  (256 sectors) * (65,536 bytes per sector)
//  (65536 pages) * (256 bytes per page)
//Sector size 1 << 16 = 65,536
//Page size 1 << 8 = 256
#elif defined(S25FL256S)
//S25FL128 ID
#define S25FL_ID    S25FL256S_ID
//--- S25FL256 Capacity is 33,554,432 Bytes ---/
//  (512 sectors) * (65,536 bytes per sector)
//  (131072 pages) * (256 bytes per page)
//Sector size 1 << 16 = 65,536
//Page size 1 << 8 = 256
#else
#error "No S25FL Memory Selected"
#endif

#define S25FL_TX_RX_BUF_LENGTH          520

/****************************************************************************************/
//  S25FLxxx Commands
/****************************************************************************************/
//--- Read Device Identification --------------------------------------------------------/
#define FLASH_CMD_READ_ID      0x90    //RDID   (Read Manufacturer & Device ID)
#define FLASH_CMD_RDID         0x9F    //RDID   (Read Identification)
#define FLASH_CMD_RES          0xAB    //RDID   (Read Electronic Signature)
//--- Register Access -------------------------------------------------------------------/
#define FLASH_CMD_RDSR1        0x05    //RDSR1  (Read Status Register 1)
#define   SR1_WIP              0x01    //WIP    (Write In Progress)
#define   SR1_WEL              0x02    //WEL    (Write Enable Latch)
#define   SR1_BP0              0x04    //BP0    (Block Protection)
#define   SR1_BP1              0x08    //BP1
#define   SR1_BP2              0x10    //BP2
#define   SR1_E_ERR            0x20    //E_ERR  (Erase Error Occured)
#define   SR1_P_ERR            0x40    //P_ERR  (Program Error Occured)
#define   SR1_SRWD             0x80    //SRWD   (Status Register Write Disable)
#define FLASH_CMD_RDSR2        0x07    //RDSR2  (Read Status Register 2)
#define   SR2_PS               0x01    //PS     (Program Suspend)
#define   SR2_ES               0x02    //ES     (Erase Suspend)
#define FLASH_CMD_RDCR         0x35    //RDCR   (Read Configuration Register 1)
#define FLASH_CMD_WRR          0x01    //WRR    (Write Status & Configuration Registers)
#define FLASH_CMD_WRDI         0x04    //WRDI   (Write Disable)
#define FLASH_CMD_WREN         0x06    //WREN   (Write Enable)
#define FLASH_CMD_CLSR         0x30    //CLSR   (Reset Erise & Program Fail Flags)
#define FLASH_CMD_ABRD         0x14    //ABRD   (AutoBoot Register Read)
#define FLASH_CMD_ABWR         0x15    //ABWR   (AutoBoot Register Write)
#define FLASH_CMD_BRRD         0x16    //BRRD   (Bank Register Read)
#define FLASH_CMD_BRWR         0x17    //BRWR   (Bank Register Write)
#define FLASH_CMD_BRAC         0xB9    //BRAC   (Bank Register Access)
#define FLASH_CMD_DLPRD        0x41    //DLPRD  (Data Learning Pattern Read)
#define FLASH_CMD_PNVDLR       0x43    //PNVDLR (Program NV Data Learning Register)
#define FLASH_CMD_WVDLR        0x4A    //WVDLR  (Write Volatile Data Learning Register)
//--- Read Flash Array ------------------------------------------------------------------/
#define FLASH_CMD_READ         0x03    //READ       (Read (3- or 4-byte address))
#define FLASH_CMD_4READ        0x13    //4READ      (Read (4-byte address))
#define FLASH_CMD_FAST_READ    0x0B    //FAST_READ  (Fast Read (3- or 4-byte address))
#define FLASH_CMD_4FAST_READ   0x0C    //4FAST_READ (Fast Read (4-byte address))
#define FLASH_CMD_DDRFR        0x0D    //DDRFR      (DDR Fast Read (3- or 4-byte address))
#define FLASH_CMD_4DDRFR       0x0E    //4DDRFR     (DDR Fast Read (4-byte address))
#define FLASH_CMD_DOR          0x3B    //DOR        (Read Dual Out (3- or 4-byte address))
#define FLASH_CMD_4DOR         0x3C    //4DOR       (Read Dual Out (4-byte address))
#define FLASH_CMD_QOR          0x6B    //QOR        (Read Quad Out (3- or 4-byte address))
#define FLASH_CMD_4QOR         0x6C    //4QOR       (Read Quad Out (4-byte address))
#define FLASH_CMD_DIOR         0xBB    //DIOR       (Dual I/O Read (3- or 4-byte address))
#define FLASH_CMD_4DIOR        0xBC    //4DIOR      (Dual I/O Read (4-byte address))
#define FLASH_CMD_DDRDIOR      0xBD    //DDRDIOR    (DDR Dual I/O Read (3- or 4-byte address))
#define FLASH_CMD_4DDRDIOR     0xBE    //4DDRDIOR   (DDR Dual I/O Read (4-byte address))
#define FLASH_CMD_QIOR         0xEB    //QIOR       (Quad I/O Read (3- or 4-byte address))
#define FLASH_CMD_4QIOR        0xEC    //4QIOR      (Quad I/O Read (4-byte address))
#define FLASH_CMD_DDRQIOR      0xED    //DDRQIOR    (DDR Quad I/O Read (3- or 4-byte address))
#define FLASH_CMD_4DDRQIOR     0xEE    //4DDRQIOR   (DDR Quad I/O Read (4-byte address))
//--- Program Flash Array ---------------------------------------------------------------/
#define FLASH_CMD_PP           0x02    //PP    (Page Program (3- or 4-byte address))
#define FLASH_CMD_4PP          0x12    //4PP   (Page Program (4-byte address))
#define FLASH_CMD_QPP          0x32    //QPP   (Quad Page Program (3- or 4-byte address))
#define FLASH_CMD_QPPA         0x38    //QPP   (Quad Page Program - Alternate instruction)
#define FLASH_CMD_4QPP         0x34    //4QPP  (Quad Page Program (4-byte address))
#define FLASH_CMD_PGSP         0x85    //PGSP  (Program Suspend)
#define FLASH_CMD_PGRS         0x8A    //PGRS  (Program Resume)
//--- Erase Flash Array -----------------------------------------------------------------/
#define FLASH_CMD_P4E          0x20    //P4E   (Parameter 4-kB, sector Erase (3- or 4-byte address))
#define FLASH_CMD_4PE4         0x21    //4P4E  (Parameter 4-kB, sector Erase (4-byte address))
#define FLASH_CMD_BE           0x60    //BE    (Bulk Erase)
#define FLASH_CMD_BEA          0xC7    //BE    (Bulk Erase (Alternate command))
#define FLASH_CMD_SE           0xD8    //SE    (Erase 64 kB or 256 kB (3- or 4-byte address))
#define FLASH_CMD_4SE          0xDC    //4SE   (Erase 64 kB or 256 kB (4-byte address))
#define FLASH_CMD_ERSP         0x75    //ERSP  (Erase Suspend)
#define FLASH_CMD_ERRS         0x7A    //ERRS  (Erase Resume)
//--- One Time Program Array ------------------------------------------------------------/
#define FLASH_CMD_OTPP         0x42    //OTPP  (OTP Program)
#define FLASH_CMD_OTPR         0x4B    //OTPR  (OTP Read)
//--- Advanced Sector Protection --------------------------------------------------------/
#define FLASH_CMD_DYBRD        0xE0    //DYBRD   (DYB Read)
#define FLASH_CMD_DYBWR        0xE1    //DYBWR   (DYB Write)
#define FLASH_CMD_PPBRD        0xE2    //PPBRD   (PPB Read)
#define FLASH_CMD_PPBP         0xE3    //PPBP    (PPB Program)
#define FLASH_CMD_PPBE         0xE4    //PPBE    (PPB Erase)
#define FLASH_CMD_ASPRD        0x2B    //ASPRD   (ASP Read)
#define FLASH_CMD_ASPP         0x2F    //ASPP    (ASP Program)
#define FLASH_CMD_PLBRD        0xA7    //PLBRD   (PPB Lock Bit Read)
#define FLASH_CMD_PLBWR        0xA6    //PLBWR   (PPB Lock Bit Write)
#define FLASH_CMD_PASSRD       0xE7    //PASSRD  (Password Read)
#define FLASH_CMD_PASSP        0xE8    //PASSP   (Password Program)
#define FLASH_CMD_PASSU        0xE9    //PASSU   (Password Unlock)
//--- Reset -----------------------------------------------------------------------------/
#define FLASH_CMD_RESET        0xF0    //RESET   (Software Reset)
#define FLASH_CMD_MBR          0xFF    //MBR     (Mode Bit Reset)
//---------------------------------------------------------------------------------------/

//Device ID and Common Flash Interface (ID-CFI)
typedef struct s25fl_idcfi_s {
	uint8_t ManufacturerID;
	uint16_t DeviceID;
	uint8_t Length;
	uint8_t SectorArchitecture;
	uint8_t FamilyID;
	uint8_t Model[10];
	uint8_t QueryStringQRY[3];
	uint16_t PrimaryOEMCommandSet;
	uint16_t PrimaryExtTableAddr;
	uint16_t AlternateOEMCommandSet;
	uint16_t AlternateExtTableAddr;
	uint8_t VccMin;
	uint8_t VccMax;
	uint8_t VppMin;
	uint8_t VppMax;
	uint8_t TypicalTimeOutProgramOneByte;
	uint8_t TypicalTimeOutProgramPage;
	uint8_t TypicalTimeOutSectorErase;
	uint8_t TypicalTimeOutFullChipErase;
	uint8_t MaxTimeOutProgramOneByte;
	uint8_t MaxTimeOutProgramPage;
	uint8_t MaxTimeOutSectorErase;
	uint8_t MaxTimeOutFullChipErase;
	uint8_t DeviceSize;
	uint16_t FlashDeviceInterface;
	uint16_t PageSize;
	uint8_t UniformOrBootDevice;
	uint8_t EraseBlockRegion1Info[4];
	uint8_t EraseBlockRegion2Info[4];
	uint8_t RFU1[10];
	uint8_t QueryStringPRI[3];
	uint8_t Version[2];
	uint8_t Technology;
	uint8_t EraseSuspend;
	uint8_t SectorProtect;
	uint8_t TempSectorUnprotect;
	uint8_t SectorProtectUnprotectScheme;
	uint8_t SimultaneousOperation;
	uint8_t BurstMode;
	uint8_t PageModeType;
	uint8_t ACCSupplyMin;
	uint8_t ACCSupplyMax;
	uint8_t WP;
	uint8_t ProgramSuspend;
} s25fl_idcfi_t, *s25fl_idcfi_p;

//Flash Structure
typedef struct s25fl_s {
	uint32_t ID;
	uint32_t FlashSize;
	uint32_t SectorSize;
	uint32_t PageSize;
	uint8_t TxBuf[S25FL_TX_RX_BUF_LENGTH];
	uint8_t RxBuf[S25FL_TX_RX_BUF_LENGTH];
} s25fl_t, *s25fl_p;

uint32_t S25FL_ReadID(void);

#endif /* INC_S25FL_H_ */
