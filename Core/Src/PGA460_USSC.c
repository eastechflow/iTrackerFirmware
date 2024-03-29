/*
	PGA460_USSC.cpp

	BSD 2-clause "Simplified" License
	Copyright (c) 2017, Texas Instruments
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY TEXAS INSTRUMENTS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL TEXAS INSTRUMENTS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	The views and conclusions contained in the software and documentation are those
	of the authors and should not be interpreted as representing official policies,
	either expressed or implied, of the FreeBSD Project.

	Last Updated: Aug 2020
	By: A. Whitehead <make@energia.nu>
 */

//#include "all_header.h"
#include "usart.h"
#include "lpuart1.h"
#include "uart3.h"
#include "PGA460_USSC.h"
#include "sensor.h"
#include "common.h"
#include "tim.h"
#include "trace.h"

#ifdef ENABLE_PGA460

//#define SWAP_BYTES



#define EnAutoThr	//enables autotset thresholding

//private:
static uint8_t calcChecksum(uint8_t cmd);

static void spiTransfer(uint8_t* mosi, uint8_t size);
static void spiMosiIdle(uint8_t size);

/*------------------------------------------------- Global Variables -----
 |  Global Variables
 |
 |  Purpose:  Variables shared throughout the PGA460_USSC.cpp functions
 *-------------------------------------------------------------------*/

//#pragma region globals


// Pin mapping of BOOSTXL-PGA460 to LaunchPad by pin name
#define DECPL_A 2
#define RXD_LP 3
#define TXD_LP 4
#define DECPL_D 5
#define TEST_A 6
#define TCI_CLK 7
#define TEST_D 8
#define MEM_SOMI 9
#define MEM_SIMO 10
#define TCI_RX 14
#define TCI_TX 15
#define COM_SEL 17
#define COM_PD 18
#define SPI_CS 33
#define SCLK_CLK 34
#define MEM_HOLD 36
#define MEM_CS 37
#define DS1_LED 38
#define F_DIAG_LED 39
#define V_DIAG_LED 40

// Serial read timeout in milliseconds
#define MAX_MILLIS_TO_WAIT 250

// Define UART commands by name
// Single Address
uint8_t P1BL = 0x00;
uint8_t P2BL = 0x01;
uint8_t P1LO = 0x02;
uint8_t P2LO = 0x03;
uint8_t TNLM = 0x04;
uint8_t UMR = 0x05;
uint8_t TNLR = 0x06;
uint8_t TEDD = 0x07;
uint8_t SD = 0x08;
uint8_t SRR = 0x09;
uint8_t SRW = 0x0A;
uint8_t EEBR = 0x0B;
uint8_t EEBW = 0x0C;
uint8_t TVGBR = 0x0D;
uint8_t TVGBW = 0x0E;
uint8_t THRBR = 0x0F;
uint8_t THRBW = 0x10;

//Broadcast
uint8_t BC_P1BL = 0x11;
uint8_t BC_P2BL = 0x12;
uint8_t BC_P1LO = 0x13;
uint8_t BC_P2LO = 0x14;
uint8_t BC_TNLM = 0x15;
uint8_t BC_RW = 0x16;
uint8_t BC_EEBW = 0x17;
uint8_t BC_TVGBW = 0x18;
uint8_t BC_THRBW = 0x19;
//CMDs 26-31 are reserved

// List user registers by name with default settings from TI factory
uint8_t USER_DATA1 = 0x00;
uint8_t USER_DATA2 = 0x00;
uint8_t USER_DATA3 = 0x00;
uint8_t USER_DATA4 = 0x00;
uint8_t USER_DATA5 = 0x00;
uint8_t USER_DATA6 = 0x00;
uint8_t USER_DATA7 = 0x00;
uint8_t USER_DATA8 = 0x00;
uint8_t USER_DATA9 = 0x00;
uint8_t USER_DATA10 = 0x00;
uint8_t USER_DATA11 = 0x00;
uint8_t USER_DATA12 = 0x00;
uint8_t USER_DATA13 = 0x00;
uint8_t USER_DATA14 = 0x00;
uint8_t USER_DATA15 = 0x00;
uint8_t USER_DATA16 = 0x00;
uint8_t USER_DATA17 = 0x00;
uint8_t USER_DATA18 = 0x00;
uint8_t USER_DATA19 = 0x00;
uint8_t USER_DATA20 = 0x00;
uint8_t TVGAIN0 = 0xAF;
uint8_t TVGAIN1 = 0xFF;
uint8_t TVGAIN2 = 0xFF;
uint8_t TVGAIN3 = 0x2D;
uint8_t TVGAIN4 = 0x68;
uint8_t TVGAIN5 = 0x36;
uint8_t TVGAIN6 = 0xFC;
uint8_t INIT_GAIN = 0xC0;
uint8_t FREQUENCY  = 0x8C;   // reset value is 0.  Frequency = 0.2*FREQ + 30 kHz
uint8_t DEADTIME = 0x00;
uint8_t PULSE_P1 = 0x01;
uint8_t PULSE_P2 = 0x12;
uint8_t CURR_LIM_P1 = 0x47;
uint8_t CURR_LIM_P2 = 0xFF;
uint8_t REC_LENGTH = 0x1C;
uint8_t FREQ_DIAG = 0x00;
uint8_t SAT_FDIAG_TH = 0xEE;
uint8_t FVOLT_DEC = 0x7C;
uint8_t DECPL_TEMP = 0x0A;
uint8_t DSP_SCALE = 0x00;
uint8_t TEMP_TRIM = 0x00;
uint8_t P1_GAIN_CTRL = 0x00;
uint8_t P2_GAIN_CTRL = 0x00;
uint8_t EE_CRC = 0xFF;
uint8_t EE_CNTRL = 0x00;
uint8_t P1_THR_0 = 0x88;
uint8_t P1_THR_1 = 0x88;
uint8_t P1_THR_2 = 0x88;
uint8_t P1_THR_3 = 0x88;
uint8_t P1_THR_4 = 0x88;
uint8_t P1_THR_5 = 0x88;
uint8_t P1_THR_6 = 0x84;
uint8_t P1_THR_7 = 0x21;
uint8_t P1_THR_8 = 0x08;
uint8_t P1_THR_9 = 0x42;
uint8_t P1_THR_10 = 0x10;
uint8_t P1_THR_11 = 0x80;
uint8_t P1_THR_12 = 0x80;
uint8_t P1_THR_13 = 0x80;
uint8_t P1_THR_14 = 0x80;
uint8_t P1_THR_15 = 0x80;
uint8_t P2_THR_0 = 0x88;
uint8_t P2_THR_1 = 0x88;
uint8_t P2_THR_2 = 0x88;
uint8_t P2_THR_3 = 0x88;
uint8_t P2_THR_4 = 0x88;
uint8_t P2_THR_5 = 0x88;
uint8_t P2_THR_6 = 0x84;
uint8_t P2_THR_7 = 0x21;
uint8_t P2_THR_8 = 0x08;
uint8_t P2_THR_9 = 0x42;
uint8_t P2_THR_10 = 0x10;
uint8_t P2_THR_11 = 0x80;
uint8_t P2_THR_12 = 0x80;
uint8_t P2_THR_13 = 0x80;
uint8_t P2_THR_14 = 0x80;
uint8_t P2_THR_15 = 0x80;

// Miscellaneous variables; (+) indicates OWU transmitted uint8_t offset

uint8_t ChecksumInput[44]; 		// data uint8_t array for checksum calculator
uint8_t ultraMeasResult[34+3]; 	// data uint8_t array for cmd5 and tciB+L return  4*numObj = 4*8 = 32 + 1 diag byte + 1 crc = 34
uint8_t diagMeasResult[5+3]; 		// data uint8_t array for cmd8 and index1 return
uint8_t tempNoiseMeasResult[4+3]; 	// data uint8_t array for cmd6 and index0&1 return
uint8_t tempOrNoise = 0; 			// data uint8_t to determine if temp or noise measurement is to be performed
uint8_t comm = 0; 					// indicates UART (0), TCI (1), OWU (2) communication mode
uint8_t bulkThr[34+3];				// data uint8_t array for bulk threhsold commands

//SPI, UART & OWU exclusive variables
uint8_t syncByte = 0x55; 		// data uint8_t for Sync field set UART baud rate of PGA460
uint8_t regAddr = 0x00; 		// data uint8_t for Register Address
uint8_t regData = 0x00; 		// data uint8_t for Register Data
uint8_t uartAddr = 0; 			// PGA460 UART device address (0-7). '0' is factory default address
uint8_t numObj = 1; 			// number of objects to detect





//SPI exclusive variables
uint8_t misoBuf[131]; 				// SPI MISO receive data buffer for all commands


//#pragma endregion globals





/*------------------------------------------------- initBoostXLPGA460 -----
 |  Function initBoostXLPGA460
 |
 |  Purpose:  Configure the master communication mode and BOOSTXL-PGA460 hardware to operate in UART, TCI, or OWU mode.
 |  Configures master serial baud rate for UART/OWU modes. Updates UART address based on sketch input.
 |
 |  Parameters:
 |		mode (IN) -- sets communication mode.
 |			0=UART 
 |			1=TCI 
 |			2=OWU 
 |			3-SPI (Synchronous Mode)
 |			4 = Not Used
 |			5 = Not Used
 |			6=Bus_Demo_Bulk_TVG_or_Threshold_Broadcast_is_True
 |			7=Bus_Demo_UART_Mode
 |			8=Bus_Demo_OWU_One_Time_Setup
 |			9=Bus_Demo_OWU_Mode
 | 		baud (IN) -- PGA460 accepts a baud rate of 9600 to 115.2k bps
 | 		uartAddrUpdate (IN) -- PGA460 address range from 0 to 7
 |
 |  Returns:  none
 *-------------------------------------------------------------------*/
void pga460_initBoostXLPGA460(uint8_t mode, uint32_t baud, uint8_t uartAddrUpdate)
{
	// For Eastech, used fixed parameters
	mode = 3;
	baud = 9600; // zg ? not really used anywhere ??
	uartAddrUpdate = 0;

	// check for valid UART address
	if (uartAddrUpdate > 7)
	{
		uartAddrUpdate = 0; // default to '0'
		pga460_log_msg("ERROR - Invalid UART Address!");
	}
	// globally update target PGA460 UART address and commands	
	if (uartAddr != uartAddrUpdate)
	{
		// Update commands to account for new UART addr
		// Single Address
		P1BL = 0x00 + (uartAddrUpdate << 5);
		P2BL = 0x01 + (uartAddrUpdate << 5);
		P1LO = 0x02 + (uartAddrUpdate << 5);
		P2LO = 0x03 + (uartAddrUpdate << 5);
		TNLM = 0x04 + (uartAddrUpdate << 5);
		UMR = 0x05 + (uartAddrUpdate << 5);
		TNLR = 0x06 + (uartAddrUpdate << 5);
		TEDD = 0x07 + (uartAddrUpdate << 5);
		SD = 0x08 + (uartAddrUpdate << 5);
		SRR = 0x09 + (uartAddrUpdate << 5);
		SRW = 0x0A + (uartAddrUpdate << 5);
		EEBR = 0x0B + (uartAddrUpdate << 5);
		EEBW = 0x0C + (uartAddrUpdate << 5);
		TVGBR = 0x0D + (uartAddrUpdate << 5);
		TVGBW = 0x0E + (uartAddrUpdate << 5);
		THRBR = 0x0F + (uartAddrUpdate << 5);
		THRBW = 0x10 + (uartAddrUpdate << 5);
	}
	uartAddr = uartAddrUpdate;



	// set communication mode flag

	comm = mode;

	switch (mode)
	{

	case 3: // SPI mode

		// Need to verify init code in MX_SPI1_Init() is correct for the PGA-460

#if 0
		usscSPI.begin();            	// start the SPI for BOOSTXL-PGA460 library
		usscSPI.setBitOrder(0);     	// set bit order to LSB first
		//In this mode the USART interface acts as a serial-shift register with data set on the rising edge of the clock and sampled on the falling edge of the clock.
		usscSPI.setDataMode(2);     	// set the data mode for clock is High when inactive (CPOL=1) & data is valid on clock leading edge (CPHA = 0) (SPI_MODE2)
		usscSPI.setClockDivider(baud); 	// set clock divider (16MHz master)
		Serial.begin(9600); 			// initialize COM UART serial channel
#endif
		break;

	default: break;
	}

}

/*------------------------------------------------- defaultPGA460 -----
 |  Function defaultPGA460
 |
 |  Purpose:  Updates user EEPROM values, and performs bulk EEPROM write.
 |
 |  Parameters:
 |		xdcr (IN) -- updates user EEPROM based on predefined listing for a specific transducer.
 |			Modify existing case statements, or append additional case-statement for custom user EEPROM configurations.
 |			• 0 = Murata MA58MF14-7N
 |			• 1 = Murata MA40H1S-R
 |			• 5 = Murata MA4S4S/R
 |			• 2 = PUI Audio UTR-1440K-TT-R
  |			• 3 = Custom Eastech
 |
 |  Returns:  none
 *-------------------------------------------------------------------*/
void pga460_defaultPGA460(uint8_t xdcr)
{
	switch (xdcr)
	{
	case 0: // Murata MA58MF14-7N
		USER_DATA1 = 0x00;
		USER_DATA2 = 0x00;
		USER_DATA3 = 0x00;
		USER_DATA4 = 0x00;
		USER_DATA5 = 0x00;
		USER_DATA6 = 0x00;
		USER_DATA7 = 0x00;
		USER_DATA8 = 0x00;
		USER_DATA9 = 0x00;
		USER_DATA10 = 0x00;
		USER_DATA11 = 0x00;
		USER_DATA12 = 0x00;
		USER_DATA13 = 0x00;
		USER_DATA14 = 0x00;
		USER_DATA15 = 0x00;
		USER_DATA16 = 0x00;
		USER_DATA17 = 0x00;
		USER_DATA18 = 0x00;
		USER_DATA19 = 0x00;
		USER_DATA20 = 0x00;
		TVGAIN0 = 0xAA;
		TVGAIN1 = 0xAA;
		TVGAIN2 = 0xAA;
		TVGAIN3 = 0x82;
		TVGAIN4 = 0x08;
		TVGAIN5 = 0x20;
		TVGAIN6 = 0x80;
		INIT_GAIN = 0x60;
		FREQUENCY  = 0x8F; // 143d :: 58.6 kHz
		DEADTIME = 0xA0;

		PULSE_P1 = 0x04;

		PULSE_P2 = 0x10;
		CURR_LIM_P1 = 0x55;
		CURR_LIM_P2 = 0x55;
		REC_LENGTH = 0x19;
		FREQ_DIAG = 0x33;
		SAT_FDIAG_TH = 0xEE;
		FVOLT_DEC = 0x7C;
		DECPL_TEMP = 0x4F;
		DSP_SCALE = 0x00;
		TEMP_TRIM = 0x00;
		P1_GAIN_CTRL = 0x09;
		P2_GAIN_CTRL = 0x09;
		break;
	case 1: // Murata MA40H1SR
		USER_DATA1 = 0x00;
		USER_DATA2 = 0x00;
		USER_DATA3 = 0x00;
		USER_DATA4 = 0x00;
		USER_DATA5 = 0x00;
		USER_DATA6 = 0x00;
		USER_DATA7 = 0x00;
		USER_DATA8 = 0x00;
		USER_DATA9 = 0x00;
		USER_DATA10 = 0x00;
		USER_DATA11 = 0x00;
		USER_DATA12 = 0x00;
		USER_DATA13 = 0x00;
		USER_DATA14 = 0x00;
		USER_DATA15 = 0x00;
		USER_DATA16 = 0x00;
		USER_DATA17 = 0x00;
		USER_DATA18 = 0x00;
		USER_DATA19 = 0x00;
		USER_DATA20 = 0x00;
		TVGAIN0 = 0xAA;
		TVGAIN1 = 0xAA;
		TVGAIN2 = 0xAA;
		TVGAIN3 = 0x51;
		TVGAIN4 = 0x45;
		TVGAIN5 = 0x14;
		TVGAIN6 = 0x50;
		INIT_GAIN = 0x54;
		FREQUENCY  = 0x32; // 50d :: 40 kHz
		DEADTIME = 0xA0;

		PULSE_P1 = 0x08;

		PULSE_P2 = 0x10;
		CURR_LIM_P1 = 0x40;
		CURR_LIM_P2 = 0x40;
		REC_LENGTH = 0x19;
		FREQ_DIAG = 0x33;
		SAT_FDIAG_TH = 0xFE;
		FVOLT_DEC = 0x7C;
		DECPL_TEMP = 0x4F;
		DSP_SCALE = 0x00;
		TEMP_TRIM = 0x00;
		P1_GAIN_CTRL = 0x09;
		P2_GAIN_CTRL = 0x09;
		break;

	case 5: // Murata MA4S4S/R
		USER_DATA1 = 0x00;
		USER_DATA2 = 0x00;
		USER_DATA3 = 0x00;
		USER_DATA4 = 0x00;
		USER_DATA5 = 0x00;
		USER_DATA6 = 0x00;
		USER_DATA7 = 0x00;
		USER_DATA8 = 0x00;
		USER_DATA9 = 0x00;
		USER_DATA10 = 0x00;
		USER_DATA11 = 0x00;
		USER_DATA12 = 0x00;
		USER_DATA13 = 0x00;
		USER_DATA14 = 0x00;
		USER_DATA15 = 0x00;
		USER_DATA16 = 0x00;
		USER_DATA17 = 0x00;
		USER_DATA18 = 0x00;
		USER_DATA19 = 0x00;
		USER_DATA20 = 0x00;
		TVGAIN0 = 0xAA;
		TVGAIN1 = 0xAA;
		TVGAIN2 = 0xAA;
		TVGAIN3 = 0x51;
		TVGAIN4 = 0x45;
		TVGAIN5 = 0x14;
		TVGAIN6 = 0x50;
		INIT_GAIN = 0x54;
		FREQUENCY  = 0x32;  // 50d :: 40 kHz
		DEADTIME = 0xA0;

		PULSE_P1 = 0x02;

		PULSE_P2 = 0x10;
		CURR_LIM_P1 = 0x40;
		CURR_LIM_P2 = 0x40;
		REC_LENGTH = 0x19;
		FREQ_DIAG = 0x33;
		SAT_FDIAG_TH = 0xFE;
		FVOLT_DEC = 0x7C;
		DECPL_TEMP = 0x4F;
		DSP_SCALE = 0x00;
		TEMP_TRIM = 0x00;
		P1_GAIN_CTRL = 0x00;
		P2_GAIN_CTRL = 0x00;
		break;

	case 2: // PUI Audio UTR-1440K-TT-R
	{
		USER_DATA1 = 0x00;
		USER_DATA2 = 0x00;
		USER_DATA3 = 0x00;
		USER_DATA4 = 0x00;
		USER_DATA5 = 0x00;
		USER_DATA6 = 0x00;
		USER_DATA7 = 0x00;
		USER_DATA8 = 0x00;
		USER_DATA9 = 0x00;
		USER_DATA10 = 0x00;
		USER_DATA11 = 0x00;
		USER_DATA12 = 0x00;
		USER_DATA13 = 0x00;
		USER_DATA14 = 0x00;
		USER_DATA15 = 0x00;
		USER_DATA16 = 0x00;
		USER_DATA17 = 0x00;
		USER_DATA18 = 0x00;
		USER_DATA19 = 0x00;
		USER_DATA20 = 0x00;
		TVGAIN0 = 0x9D;
		TVGAIN1 = 0xEE;
		TVGAIN2 = 0xEF;
		TVGAIN3 = 0x2D;
		TVGAIN4 = 0xB9;
		TVGAIN5 = 0xEF;
		TVGAIN6 = 0xDC;
		INIT_GAIN = 0x03;
		FREQUENCY  = 0x32;  // 50d :: 40 kHz
		DEADTIME = 0x80;

		PULSE_P1 = 0x08;

		PULSE_P2 = 0x12;
		CURR_LIM_P1 = 0x72;
		CURR_LIM_P2 = 0x32;
		REC_LENGTH = 0x09;
		FREQ_DIAG = 0x33;
		SAT_FDIAG_TH = 0xEE;
		FVOLT_DEC = 0x7C;
		DECPL_TEMP = 0x8F;
		DSP_SCALE = 0x00;
		TEMP_TRIM = 0x00;
		P1_GAIN_CTRL = 0x09;
		P2_GAIN_CTRL = 0x29;
		break;
	}
	case 3: // User Custom XDCR
	{
		USER_DATA1 = 0x00;
		USER_DATA2 = 0x00;
		USER_DATA3 = 0x00;
		USER_DATA4 = 0x00;
		USER_DATA5 = 0x00;
		USER_DATA6 = 0x00;
		USER_DATA7 = 0x00;
		USER_DATA8 = 0x00;
		USER_DATA9 = 0x00;
		USER_DATA10 = 0x00;
		USER_DATA11 = 0x00;
		USER_DATA12 = 0x00;
		USER_DATA13 = 0x00;
		USER_DATA14 = 0x00;
		USER_DATA15 = 0x00;
		USER_DATA16 = 0x00;
		USER_DATA17 = 0x00;
		USER_DATA18 = 0x00;
		USER_DATA19 = 0x00;
		USER_DATA20 = 0x00;
		TVGAIN0 = 0xAA;
		TVGAIN1 = 0xAA;
		TVGAIN2 = 0xAA;
		TVGAIN3 = 0x82;
		TVGAIN4 = 0x08;
		TVGAIN5 = 0x20;
		TVGAIN6 = 0x80;
		INIT_GAIN = 0x60;
		FREQUENCY  = 0x8F;  // 143d :: 58.6 kHz
		DEADTIME = 0xA0;

		PULSE_P1 = 0x04;

		PULSE_P2 = 0x10;
		CURR_LIM_P1 = 0x55;
		CURR_LIM_P2 = 0x55;
		REC_LENGTH = 0x19;
		FREQ_DIAG = 0x33;
		SAT_FDIAG_TH = 0xFE;
		FVOLT_DEC = 0x7C;
		DECPL_TEMP = 0x4F;
		DSP_SCALE = 0x00;
		TEMP_TRIM = 0x00;
		P1_GAIN_CTRL = 0x09;
		P2_GAIN_CTRL = 0x09;
		break;
	}
	default: break;
	}


	uint8_t buf12[46] = {syncByte, EEBW, USER_DATA1, USER_DATA2, USER_DATA3, USER_DATA4, USER_DATA5, USER_DATA6,
			USER_DATA7, USER_DATA8, USER_DATA9, USER_DATA10, USER_DATA11, USER_DATA12, USER_DATA13, USER_DATA14,
			USER_DATA15,USER_DATA16,USER_DATA17,USER_DATA18,USER_DATA19,USER_DATA20,
			TVGAIN0,TVGAIN1,TVGAIN2,TVGAIN3,TVGAIN4,TVGAIN5,TVGAIN6,INIT_GAIN,FREQUENCY,DEADTIME,
			PULSE_P1,PULSE_P2,CURR_LIM_P1,CURR_LIM_P2,REC_LENGTH,FREQ_DIAG,SAT_FDIAG_TH,FVOLT_DEC,DECPL_TEMP,
			DSP_SCALE,TEMP_TRIM,P1_GAIN_CTRL,P2_GAIN_CTRL,calcChecksum(EEBW)};



	spiTransfer(buf12, sizeof(buf12));

	HAL_Delay(50);

	// Update targeted UART_ADDR to address defined in EEPROM bulk switch-case
	uint8_t uartAddrUpdate = (PULSE_P2 >> 5) & 0x07;
	if (uartAddr != uartAddrUpdate)
	{
		// Update commands to account for new UART addr
		// Single Address
		P1BL = 0x00 + (uartAddrUpdate << 5);
		P2BL = 0x01 + (uartAddrUpdate << 5);
		P1LO = 0x02 + (uartAddrUpdate << 5);
		P2LO = 0x03 + (uartAddrUpdate << 5);
		TNLM = 0x04 + (uartAddrUpdate << 5);
		UMR = 0x05 + (uartAddrUpdate << 5);
		TNLR = 0x06 + (uartAddrUpdate << 5);
		TEDD = 0x07 + (uartAddrUpdate << 5);
		SD = 0x08 + (uartAddrUpdate << 5);
		SRR = 0x09 + (uartAddrUpdate << 5);
		SRW = 0x0A + (uartAddrUpdate << 5);
		EEBR = 0x0B + (uartAddrUpdate << 5);
		EEBW = 0x0C + (uartAddrUpdate << 5);
		TVGBR = 0x0D + (uartAddrUpdate << 5);
		TVGBW = 0x0E + (uartAddrUpdate << 5);
		THRBR = 0x0F + (uartAddrUpdate << 5);
		THRBW = 0x10 + (uartAddrUpdate << 5);
	}
	uartAddr = uartAddrUpdate;

}

/*------------------------------------------------- initThresholds -----
 |  Function initThresholds
 |
 |  Purpose:  Updates threshold mapping for both presets, and performs bulk threshold write
 |
 |  Parameters:
 |		thr (IN) -- updates all threshold levels to a fixed level based on specific percentage of the maximum level. 
 |			All times are mid-code (1.4ms intervals).
 |			Modify existing case statements, or append additional case-statement for custom user threshold configurations.
 |			• 0 = 25% Levels 64 of 255
 |			• 1 = 50% Levels 128 of 255
 |			• 2 = 75% Levels 192 of 255
 |
 |  Returns:  none
 *-------------------------------------------------------------------*/
void pga460_initThresholds(uint8_t thr)
{
	switch (thr)
	{
	case 0: //25% Levels 64 of 255
		P1_THR_0 = 0x88;
		P1_THR_1 = 0x88;
		P1_THR_2 = 0x88;
		P1_THR_3 = 0x88;
		P1_THR_4 = 0x88;
		P1_THR_5 = 0x88;
		P1_THR_6 = 0x42;
		P1_THR_7 = 0x10;
		P1_THR_8 = 0x84;
		P1_THR_9 = 0x21;
		P1_THR_10 = 0x08;
		P1_THR_11 = 0x40;
		P1_THR_12 = 0x40;
		P1_THR_13 = 0x40;
		P1_THR_14 = 0x40;
		P1_THR_15 = 0x00;

		P2_THR_0 = 0x88;
		P2_THR_1 = 0x88;
		P2_THR_2 = 0x88;
		P2_THR_3 = 0x88;
		P2_THR_4 = 0x88;
		P2_THR_5 = 0x88;
		P2_THR_6 = 0x42;
		P2_THR_7 = 0x10;
		P2_THR_8 = 0x84;
		P2_THR_9 = 0x21;
		P2_THR_10 = 0x08;
		P2_THR_11 = 0x40;
		P2_THR_12 = 0x40;
		P2_THR_13 = 0x40;
		P2_THR_14 = 0x40;
		P2_THR_15 = 0x00;
		break;

	case 1: //50% Level (midcode) 128 of 255
		P1_THR_0 = 0x88;
		P1_THR_1 = 0x88;
		P1_THR_2 = 0x88;
		P1_THR_3 = 0x88;
		P1_THR_4 = 0x88;
		P1_THR_5 = 0x88;
		P1_THR_6 = 0x84;
		P1_THR_7 = 0x21;
		P1_THR_8 = 0x08;
		P1_THR_9 = 0x42;
		P1_THR_10 = 0x10;
		P1_THR_11 = 0x80;
		P1_THR_12 = 0x80;
		P1_THR_13 = 0x80;
		P1_THR_14 = 0x80;
		P1_THR_15 = 0x00;

		P2_THR_0 = 0x88;
		P2_THR_1 = 0x88;
		P2_THR_2 = 0x88;
		P2_THR_3 = 0x88;
		P2_THR_4 = 0x88;
		P2_THR_5 = 0x88;
		P2_THR_6 = 0x84;
		P2_THR_7 = 0x21;
		P2_THR_8 = 0x08;
		P2_THR_9 = 0x42;
		P2_THR_10 = 0x10;
		P2_THR_11 = 0x80;
		P2_THR_12 = 0x80;
		P2_THR_13 = 0x80;
		P2_THR_14 = 0x80;
		P2_THR_15 = 0x00;
		break;

	case 2: //75% Levels 192 of 255
		P1_THR_0 = 0x88;
		P1_THR_1 = 0x88;
		P1_THR_2 = 0x88;
		P1_THR_3 = 0x88;
		P1_THR_4 = 0x88;
		P1_THR_5 = 0x88;
		P1_THR_6 = 0xC6;
		P1_THR_7 = 0x31;
		P1_THR_8 = 0x8C;
		P1_THR_9 = 0x63;
		P1_THR_10 = 0x18;
		P1_THR_11 = 0xC0;
		P1_THR_12 = 0xC0;
		P1_THR_13 = 0xC0;
		P1_THR_14 = 0xC0;
		P1_THR_15 = 0x00;

		P2_THR_0 = 0x88;
		P2_THR_1 = 0x88;
		P2_THR_2 = 0x88;
		P2_THR_3 = 0x88;
		P2_THR_4 = 0x88;
		P2_THR_5 = 0x88;
		P2_THR_6 = 0xC6;
		P2_THR_7 = 0x31;
		P2_THR_8 = 0x8C;
		P2_THR_9 = 0x63;
		P2_THR_10 = 0x18;
		P2_THR_11 = 0xC0;
		P2_THR_12 = 0xC0;
		P2_THR_13 = 0xC0;
		P2_THR_14 = 0xC0;
		P2_THR_15 = 0x00;
		break;

	case 3: //Custom00
		P1_THR_0 = 0x41;
		P1_THR_1 = 0x44;
		P1_THR_2 = 0x10;
		P1_THR_3 = 0x06;
		P1_THR_4 = 0x69;
		P1_THR_5 = 0x99;
		P1_THR_6 = 0xDD;
		P1_THR_7 = 0x4C;
		P1_THR_8 = 0x31;
		P1_THR_9 = 0x08;
		P1_THR_10 = 0x42;
		P1_THR_11 = 0x18;
		P1_THR_12 = 0x20;
		P1_THR_13 = 0x24;
		P1_THR_14 = 0x2A;
		P1_THR_15 = 0x00;

		P2_THR_0 = 0x41;
		P2_THR_1 = 0x44;
		P2_THR_2 = 0x10;
		P2_THR_3 = 0x06;
		P2_THR_4 = 0x09;
		P2_THR_5 = 0x99;
		P2_THR_6 = 0xDD;
		P2_THR_7 = 0x4C;
		P2_THR_8 = 0x31;
		P2_THR_9 = 0x08;
		P2_THR_10 = 0x42;
		P2_THR_11 = 0x24;
		P2_THR_12 = 0x30;
		P2_THR_13 = 0x36;
		P2_THR_14 = 0x3C;
		P2_THR_15 = 0x00;
		break;

	default: break;
	}


	uint8_t buf16[35] = {syncByte, THRBW, P1_THR_0, P1_THR_1, P1_THR_2, P1_THR_3, P1_THR_4, P1_THR_5, P1_THR_6,
			P1_THR_7, P1_THR_8, P1_THR_9, P1_THR_10, P1_THR_11, P1_THR_12, P1_THR_13, P1_THR_14, P1_THR_15,
			P2_THR_0, P2_THR_1, P2_THR_2, P2_THR_3, P2_THR_4, P2_THR_5, P2_THR_6,
			P2_THR_7, P2_THR_8, P2_THR_9, P2_THR_10, P2_THR_11, P2_THR_12, P2_THR_13, P2_THR_14, P2_THR_15,
			calcChecksum(THRBW)};


	spiTransfer(buf16, sizeof(buf16));

	HAL_Delay(100);

}

/*------------------------------------------------- initTVG -----
 |  Function initTVG
 |
 |  Purpose:  Updates time varying gain (TVG) range and mapping, and performs bulk TVG write
 |
 |  Parameters:
 |		agr (IN) -- updates the analog gain range for the TVG.
 |			• 0 = 32-64dB
 |			• 1 = 46-78dB
 |			• 2 = 52-84dB
 |			• 3 = 58-90dB
 |		tvg (IN) -- updates all TVG levels to a fixed level based on specific percentage of the maximum level. 
 |			All times are mid-code (2.4ms intervals).
 |			Modify existing case statements, or append additional case-statement for custom user TVG configurations
 |			• 0 = 25% Levels of range
 |			• 1 = 50% Levels of range
 |			• 2 = 75% Levels of range
 |
 |  Returns:  none
 *-------------------------------------------------------------------*/
void pga460_initTVG(uint8_t agr, uint8_t tvg)
{
	uint8_t gain_range = 0x4F;
	// set AFE gain range
	switch (agr)
	{
	case 3: //58-90dB
		gain_range =  0x0F;
		break;
	case 2: //52-84dB
		gain_range = 0x4F;
		break;
	case 1: //46-78dB
		gain_range = 0x8F;
		break;
	case 0: //32-64dB
		gain_range = 0xCF;
		break;
	default: break;
	}	


	regAddr = 0x26;
	regData = gain_range;
	uint8_t buf10[5] = {syncByte, SRW, regAddr, regData, calcChecksum(SRW)};

	spiTransfer(buf10, sizeof(buf10));


	//Set fixed AFE gain value
	switch (tvg)
	{
	case 0: //25% Level
		TVGAIN0 = 0x88;
		TVGAIN1 = 0x88;
		TVGAIN2 = 0x88;
		TVGAIN3 = 0x41;
		TVGAIN4 = 0x04;
		TVGAIN5 = 0x10;
		TVGAIN6 = 0x40;
		break;

	case 1: //50% Levels
		TVGAIN0 = 0x88;
		TVGAIN1 = 0x88;
		TVGAIN2 = 0x88;
		TVGAIN3 = 0x82;
		TVGAIN4 = 0x08;
		TVGAIN5 = 0x20;
		TVGAIN6 = 0x80;
		break;

	case 2: //75% Levels
		TVGAIN0 = 0x88;
		TVGAIN1 = 0x88;
		TVGAIN2 = 0x88;
		TVGAIN3 = 0xC3;
		TVGAIN4 = 0x0C;
		TVGAIN5 = 0x30;
		TVGAIN6 = 0xC0;
		break;

	default: break;
	}	


	uint8_t buf14[10] = {syncByte, TVGBW, TVGAIN0, TVGAIN1, TVGAIN2, TVGAIN3, TVGAIN4, TVGAIN5, TVGAIN6, calcChecksum(TVGBW)};

	spiTransfer(buf14, sizeof(buf14));

}

/*------------------------------------------------- ultrasonicCmd -----
 |  Function ultrasonicCmd
 |
 |  Purpose:  Issues a burst-and-listen or listen-only command based on the number of objects to be detected.
 |
 |  Parameters:
 |		cmd (IN) -- determines which preset command is run
 |			• 0 = Preset 1 Burst + Listen command
 |			• 1 = Preset 2 Burst + Listen command
 |			• 2 = Preset 1 Listen Only command
 |			• 3 = Preset 2 Listen Only command
 |			• 17 = Preset 1 Burst + Listen broadcast command
 |			• 18 = Preset 2 Burst + Listen broadcast command
 |			• 19 = Preset 1 Listen Only broadcast command
 |			• 20 = Preset 2 Listen Only broadcast command
 |		numObjUpdate (IN) -- PGA460 can capture time-of-flight, width, and amplitude for 1 to 8 objects. 
 |			TCI is limited to time-of-flight measurement data only.
 |
 |  Returns:  none
 *-------------------------------------------------------------------*/
void pga460_ultrasonicCmd(uint8_t cmd, uint8_t numObjUpdate)
{	
	numObj = numObjUpdate; // number of objects to detect
	uint8_t bufCmd[4] = {syncByte, 0xFF, numObj, 0xFF}; // prepare bufCmd with 0xFF placeholders
//#define INDUCE_PERIODIC_ERROR
#ifdef 	INDUCE_PERIODIC_ERROR
	static int induce_error = 0;
#endif


	//memset(objTime, 0xFF, 8*sizeof(unsigned int)); // reset and idle-high TCI object buffer


	switch (cmd)
	{
	// SINGLE ADDRESS
	case 0: // Send Preset 1 Burst + Listen command
	{
		bufCmd[1] = P1BL;
		bufCmd[3] = calcChecksum(P1BL);
		break;
	}
	case 1: // Send Preset 2 Burst + Listen command
	{
		bufCmd[1] = P2BL;
		bufCmd[3] = calcChecksum(P2BL);
		break;
	}
	case 2: // Send Preset 1 Listen Only command
	{
		bufCmd[1] = P1LO;
		bufCmd[3] = calcChecksum(P1LO);
		break;
	}
	case 3: // Send Preset 2 Listen Only command
	{
		bufCmd[1] = P2LO;
		bufCmd[3] = calcChecksum(P2LO);
		break;
	}

	// BROADCAST
	case 17: // Send Preset 1 Burst + Listen Broadcast command
	{
		bufCmd[1] = BC_P1BL;
		bufCmd[3] = calcChecksum(BC_P1BL);
		break;
	}
	case 18: // Send Preset 2 Burst + Listen Broadcast command
	{
		bufCmd[1] = BC_P2BL;
		bufCmd[3] = calcChecksum(BC_P2BL);
		break;
	}
	case 19: // Send Preset 1 Listen Only Broadcast command
	{
		bufCmd[1] = BC_P1LO;
		bufCmd[3] = calcChecksum(BC_P1LO);
		break;
	}
	case 20: // Send Preset 2 Listen Only Broadcast command
	{
		bufCmd[1] = BC_P2LO;
		bufCmd[3] = calcChecksum(BC_P2LO);
		break;
	}

	default: return;
	}		



	spiTransfer(bufCmd, sizeof(bufCmd));

	// To measure a data point 25 feet (7.6 m) away is a round trip distance of 15.2 m
	// 15.2 m x (1/331 m/s)  = 0.046 s = 46 ms
	// The entire measurement has to be completed in that amount of time.
	// Measured anywhere from 69 to 158 ms using the EVM before it would request data after sending a command.

	//6-30-2023 The grid of control parameters in use in v221 firmware suggests the device will record for 54 ms
	// See register 34 (22h) for P1_REC and P2_REC  grid[34] = 172 (0xAC)   P1_REC = 0xA = 10   P2_REC = 0xC = 12
	// P1 = 4.096 *(10 + 1) ms = 45.056 ms
	// P2 = 4.096 *(12 + 1) ms = 53.248 ms

	//HAL_Delay(175); // wait for command to complete.
	//HAL_Delay(50); // wait for command to complete.
#ifdef 	INDUCE_PERIODIC_ERROR
	induce_error++;
	if (induce_error < 4)
	{
		// normal
		HAL_Delay(pga460_burst_listen_delay_ms); // wait for command to complete.
	}
	else
	{
		induce_error = 0;
		//HAL_Delay(1); // do not wait for command to complete.
	}
#else
	HAL_Delay(pga460_burst_listen_delay_ms); // wait for command to complete.
#endif



	// There is an additional inter-field wait time of 1-bit period when UART changes direction
	// At 38400 that is about 0.3 ms

}

/*------------------------------------------------- pullUltrasonicMeasResult -----
 |  Function pullUltrasonicMeasResult
 |
 |  Purpose:  Read the ultrasonic measurement result data based on the last burst and/or listen command issued.
 |
 |
 |  Returns:  If measurement data successfully read, return true.
 *-------------------------------------------------------------------*/
void pga460_pullUltrasonicMeasResult(void)
{
	int retry_count = 0;

	uint8_t buf5[3] = {syncByte, UMR, calcChecksum(UMR)};

	TryAgain:

	memset(ultraMeasResult, 0, sizeof(ultraMeasResult));

	spiTransfer(buf5, sizeof(buf5));

	// MOSI transmit 0xFF to pull MISO return data
	spiMosiIdle(2+(numObj*4));  // 1 diag byte, 4 data bytes per object, 1 checksum byte

#define ENABLE_RETRY
#define MAX_RETRY      10
#define RETRY_DELAY_MS 10
	// At a distance of 212 to 230 inches, an induced error took at most nine retries to self-correct. So shoot for 10 ms x 10 retries = 100 ms for some margin.
	// A maximum round-trip time is estimated at 57 ms, so part of that total time might be
	// coming from the transmit time of the retry command at 38400 baud. Because the observed self-correct time
	// occured within ten retries at 1 ms intervals when it should have been closer to 57 ms.  Something was taking up the time
	// so the pga 460 could complete the measurement.

#ifdef ENABLE_RETRY
	// See section 7.3.6.2.1.6.3 Response Operation on page 41 of datasheet
	// If diag data field reports busy (bit 0 is 1) the results returned are suspect.

	if (misoBuf[0] != 0x40)
	{
      char msg[80];
      retry_count++;
      sprintf(msg, "  pullUltra err %02X, %02X, %02X, Retry %d  ", misoBuf[0], misoBuf[1], misoBuf[2], retry_count);
      trace_AppendMsg(msg);

	  if (retry_count <= MAX_RETRY)
	  {
		  HAL_Delay(RETRY_DELAY_MS);  // At a distance of 212 to 130 inches, an induced error took about ten ms to self-correct. So shoot for 10 ms x 10 retries = 100 ms for some margin.

		  goto TryAgain;
	  }

	  return;

	}
#endif


	// copy MISO global array data to local array based on number of objects
	for (int n=0; n < (2+(numObj*4)); n++)
	{

#if 0   // inject raw data from EVM, note for one object the n counter goes beyond the actual bytes received
      static int which_raw = 0;
      switch (which_raw)
      {
      case 0: misoBuf[0]=0x10; misoBuf[1]=0xFF; misoBuf[2]=0x8B; misoBuf[3]=0xB5; misoBuf[4]=0x6E; break;
      case 1: misoBuf[0]=0x11; misoBuf[1]=0x00; misoBuf[2]=0x82; misoBuf[3]=0x96; misoBuf[4]=0x95; break;
      case 2: misoBuf[0]=0x10; misoBuf[1]=0xFF; misoBuf[2]=0x86; misoBuf[3]=0xA6; misoBuf[4]=0x82; break;
      case 3: misoBuf[0]=0x10; misoBuf[1]=0xFF; misoBuf[2]=0x88; misoBuf[3]=0xA9; misoBuf[4]=0x7D; break;
      case 4: misoBuf[0]=0x10; misoBuf[1]=0xFD; misoBuf[2]=0x88; misoBuf[3]=0xB3; misoBuf[4]=0x75; break;
      case 5: misoBuf[0]=0x10; misoBuf[1]=0xFF; misoBuf[2]=0x89; misoBuf[3]=0xAE; misoBuf[4]=0x77; break;
      case 6: misoBuf[0]=0x11; misoBuf[1]=0x01; misoBuf[2]=0x8A; misoBuf[3]=0xAF; misoBuf[4]=0x73; break;
      case 7: misoBuf[0]=0x10; misoBuf[1]=0xFF; misoBuf[2]=0x8B; misoBuf[3]=0xB5; misoBuf[4]=0x6E; break;
      }
      which_raw++;
      if (which_raw == 7) which_raw = 0;
#endif
      // ultraMeasResult [0] is diag byte; [1] is dist MSB; [2] is dist LSB; [3] is width; [4] is peak amplitude; [5] is checksum

	  ultraMeasResult[n] = misoBuf[n];
	}

}

/*------------------------------------------------- printUltrasonicMeasResult -----
 |  Function printUltrasonicMeasResult
 |
 |  Purpose:  Converts time-of-flight readout to distance in meters. (zg - now in inches)
 |		Width and amplitude data only available in UART or OWU mode.
 |
 |  Parameters:
 |		umr (IN) -- Ultrasonic measurement result look-up selector:
 |				Distance (m)	Width	Amplitude
 |				--------------------------------
 |			Obj1		0		1		2
 |			Obj2		3		4		5
 |			Obj3		6		7		8
 |			Obj4		9		10		11
 |			Obj5		12		13		14
 |			Obj6		15		16		17
 |			Obj7		18		19		20
 |			Obj8		21		22		23
 |
 |  Returns:  double representation of distance (m), width (us), or amplitude (8-bit)
 *-------------------------------------------------------------------*/


//float get_speed_of_sound_inches_per_us(float tempC);


double pga460_printUltrasonicMeasResult(uint8_t umr)
{

	double speedSound_inches_per_us;
	float tempC;

	tempC = (LastGoodDegF - 32.0) * 5.0 / 9.0;  // Note: USES GLOBAL VARIABLE

    extern float get_speed_of_sound_inches_per_us(float tempC);

	speedSound_inches_per_us = get_speed_of_sound_inches_per_us(tempC);

	return pga460_printUltrasonicMeasResultExt(umr, speedSound_inches_per_us);

}





double pga460_printUltrasonicMeasResultExt(uint8_t umr, double speedSound_us)
{
	double objReturn = 0;
	double digitalDelay = 0; // TODO: compensates the burst time calculated as number_of_pulses/frequency.
	uint16_t objDist = 0;
	uint16_t objWidth = 0;
	uint16_t objAmp = 0;

	switch (umr)
	{
	case 0: //Obj1 Distance (m)
	{
		// time-of-flight in us
		objDist = (ultraMeasResult[1]<<8) + ultraMeasResult[2];
		objReturn = (objDist/2*speedSound_us) - digitalDelay;
		break;
	}
	case 1: //Obj1 Width (us)
	{
		objWidth = ultraMeasResult[3];
		objReturn= objWidth * 16;
		break;
	}
	case 2: //Obj1 Peak Amplitude
	{
		objAmp = ultraMeasResult[4];
		objReturn= objAmp;
		break;
	}

	case 3: //Obj2 Distance (m)
	{
		objDist = (ultraMeasResult[5]<<8) + ultraMeasResult[6];
		objReturn = (objDist/2*speedSound_us) - digitalDelay;
		break;
	}
	case 4: //Obj2 Width (us)
	{
		objWidth = ultraMeasResult[7];
		objReturn= objWidth * 16;
		break;
	}
	case 5: //Obj2 Peak Amplitude
	{
		objAmp = ultraMeasResult[8];
		objReturn= objAmp;
		break;
	}

	case 6: //Obj3 Distance (m)
	{
		objDist = (ultraMeasResult[9]<<8) + ultraMeasResult[10];
		objReturn = (objDist/2*speedSound_us) - digitalDelay;
		break;
	}
	case 7: //Obj3 Width (us)
	{
		objWidth = ultraMeasResult[11];
		objReturn= objWidth * 16;
		break;
	}
	case 8: //Obj3 Peak Amplitude
	{
		objAmp = ultraMeasResult[12];
		objReturn= objAmp;
		break;
	}
	case 9: //Obj4 Distance (m)
	{
		objDist = (ultraMeasResult[13]<<8) + ultraMeasResult[14];
		objReturn = (objDist/2*speedSound_us) - digitalDelay;
		break;
	}
	case 10: //Obj4 Width (us)
	{
		objWidth = ultraMeasResult[15];
		objReturn= objWidth * 16;
		break;
	}
	case 11: //Obj4 Peak Amplitude
	{
		objAmp = ultraMeasResult[16];
		objReturn= objAmp;
		break;
	}
	case 12: //Obj5 Distance (m)
	{
		objDist = (ultraMeasResult[17]<<8) + ultraMeasResult[18];
		objReturn = (objDist/2*speedSound_us) - digitalDelay;
		break;
	}
	case 13: //Obj5 Width (us)
	{
		objWidth = ultraMeasResult[19];
		objReturn= objWidth * 16;
		break;
	}
	case 14: //Obj5 Peak Amplitude
	{
		objAmp = ultraMeasResult[20];
		objReturn= objAmp;
		break;
	}
	case 15: //Obj6 Distance (m)
	{
		objDist = (ultraMeasResult[21]<<8) + ultraMeasResult[22];
		objReturn = (objDist/2*speedSound_us) - digitalDelay;
		break;
	}
	case 16: //Obj6 Width (us)
	{
		objWidth = ultraMeasResult[23];
		objReturn= objWidth * 16;
		break;
	}
	case 17: //Obj6 Peak Amplitude
	{
		objAmp = ultraMeasResult[24];
		objReturn= objAmp;
		break;
	}
	case 18: //Obj7 Distance (m)
	{
		objDist = (ultraMeasResult[25]<<8) + ultraMeasResult[26];
		objReturn = (objDist/2*speedSound_us) - digitalDelay;
		break;
	}
	case 19: //Obj7 Width (us)
	{
		objWidth = ultraMeasResult[27];
		objReturn= objWidth * 16;
		break;
	}
	case 20: //Obj7 Peak Amplitude
	{
		objAmp = ultraMeasResult[28];
		objReturn= objAmp;
		break;
	}
	case 21: //Obj8 Distance (m)
	{
		objDist = (ultraMeasResult[29]<<8) + ultraMeasResult[30];
		objReturn = (objDist/2*speedSound_us) - digitalDelay;
		break;
	}
	case 22: //Obj8 Width (us)
	{
		objWidth = ultraMeasResult[31];
		objReturn= objWidth * 16;
		break;
	}
	case 23: //Obj8 Peak Amplitude
	{
		objAmp = ultraMeasResult[32];
		objReturn= objAmp;
		break;
	}
	default: pga460_log_msg("ERROR - Invalid object result!"); break;
	}	
	return objReturn;
}

void pga460_update_data_point(PGA460_DATA_POINT *p, float speedSound_inches_per_us)
{
	// ASSERT:   a burst+listen has already been performed
	// ASSERT:   measurement results have been pulled from device
	// ASSERT:   diagnostic results have been pulled from device

	float digitalDelay = 0; // TODO: compensates the burst time calculated as number_of_pulses/frequency.
	uint16_t objDist = 0;
	uint16_t objWidth = 0;
	uint16_t objAmp = 0;

	p->raw_echo_uart  = ultraMeasResult[0];

	for (int i = 0; i < PGA460_ECHOS; i++)
	{
	  int k = (i*4);

	  objDist  = (ultraMeasResult[k+1]<<8) + ultraMeasResult[k+2];
	  objWidth = ultraMeasResult[k+3];
	  objAmp   = ultraMeasResult[k+4];

	  p->echo[i].raw_distance = objDist;
	  p->echo[i].raw_width    = objWidth;
	  p->echo[i].raw_peak     = objAmp;
	  p->echo[i].distance     = (((float)objDist)/2.0 * speedSound_inches_per_us) - digitalDelay;  // Note: in Gen4 the digitalDelay was 0.15
	  p->echo[i].width        = objWidth * 16;

	}

	p->raw_diag_uart  = diagMeasResult[0];
	p->raw_diag_freq  = diagMeasResult[1];
	p->raw_diag_decay = diagMeasResult[2];

	// convert to transducer frequency in kHz
	p->diag_freq = (1 / (diagMeasResult[1] * 0.0000005)) / 1000.0;

	// convert to decay period time in us
	p->diag_decay = diagMeasResult[2] * 16;

}



/*------------------------------------------------- runEchoDataDump -----
 |  Function runEchoDataDump
 |
 |  Purpose:  Runs a preset 1 or 2 burst and or listen command to capture 128 uint8_ts of echo data dump.
 |		Toggle echo data dump enable bit to enable/disable echo data dump mode.
 |
 |  Parameters:
 |		preset (IN) -- determines which preset command is run:
 |			• 0 = Preset 1 Burst + Listen command
 |			• 1 = Preset 2 Burst + Listen command
 |			• 2 = Preset 1 Listen Only command
 |			• 3 = Preset 2 Listen Only command
 |			• 17 = Preset 1 Burst + Listen broadcast command
 |			• 18 = Preset 2 Burst + Listen broadcast command
 |			• 19 = Preset 1 Listen Only broadcast command
 |			• 20 = Preset 2 Listen Only broadcast command
 |
 |  Returns:  none
 *-------------------------------------------------------------------*/
void pga460_runEchoDataDump(uint8_t preset)
{

	// enable Echo Data Dump bit
	regAddr = 0x40;
	regData = 0x80;
	uint8_t writeType = SRW; // default to single address register write (cmd10)
	if(preset>16) // update to broadcast register write if broadcast TOF preset command given
	{
		writeType = BC_RW; // cmd22
	}

	uint8_t buf10[5] = {syncByte, writeType, regAddr, regData, calcChecksum(writeType)};


	spiTransfer(buf10, sizeof(buf10));     // pga460_registerWrite

	HAL_Delay(10);


	// run preset 1 or 2 burst and or listen command
	pga460_ultrasonicCmd(preset, 1);

	// Disable Echo Data Dump bit
	regData = 0x00;
	buf10[3] = regData;
	buf10[4] = calcChecksum(writeType);

	spiTransfer(buf10, sizeof(buf10));     // pga460_registerWrite

}

/*------------------------------------------------- pullEchoDataDump -----
 |  Function pullEchoDataDump
 |
 |  Purpose:  Read out 128 uint8_ts of echo data dump (EDD) from latest burst and or listen command. 
 |		For UART and OWU, readout individual echo data dump register values, instead in bulk.
 |		For TCI, perform index 12 read of all echo data dump values in bulk.
 |
 |  Parameters:
 |		element (IN) -- element from the 128 uint8_t EDD memory
 |
 |  Returns:  uint8_t representation of EDD element value
 *-------------------------------------------------------------------*/
uint8_t pga460_pullEchoDataDump(uint8_t element)
{
	// run read bulk transducer echo data dump command on first iteration
	if (element == 0)
	{
		uint8_t buf7[3] = {syncByte, TEDD, calcChecksum(TEDD)};

		spiTransfer(buf7, sizeof(buf7));
		// MOSI transmit 0xFF to pull MISO return data:
		spiMosiIdle(130);  // 1 diag byte, 128 data bytes, 1 checksum byte
		//// copy MISO global array data to local array based on number of objects
		//for(int n=0; n<129; n++)
		//{
		//   localArry[n] = misoBuf[n];
		//}
	}
	return misoBuf[element];
}

/*------------------------------------------------- pullEchoDataDumpBulk -----
 |  Function pullEchoDataDumpBulk
 |
 |  Purpose:  Bulk read out 128 uint8_ts of echo data dump (EDD) from latest burst and or listen command. 
 |		For UART and OWU, readout bulk echo data dump register values.
 |		For TCI, perform index 12 read of all echo data dump values in bulk.
 |
 |  Parameters:
 |		none
 |
 |  Returns:  comma delimited string of all EDD values
 *-------------------------------------------------------------------*/
void pga460_pullEchoDataDumpBulk()
{
#if 0
	String bulkString = "";


	// run read bulk transducer echo data dump command on first iteration
	uint8_t buf7[3] = {syncByte, TEDD, calcChecksum(TEDD)};
	spiTransfer(buf7, sizeof(buf7));
	// MOSI transmit 0xFF to pull MISO return data
	spiMosiIdle(130);  // 1 diag byte, 128 data bytes, 1 checksum byte
	for(int n=0; n<128; n++)
	{
		bulkString = bulkString + "," + misoBuf[n];
	}
	return bulkString;


	return "ERROR - pullEchoDataDumpBulk";
#endif
}

/*------------------------------------------------- runDiagnostics -----
 |  Function runDiagnostics
 |
 |  Purpose:  Runs a burst+listen command to capture frequency, decay, and voltage diagnostic.
 |		Runs a listen-only command to capture noise level.
 |		Captures die temperature of PGA460 device.
 |		Converts raw diagnostics to comprehensive units
 |
 |  Parameters:
 |		run (IN) -- issue a preset 1 burst-and-listen command
 |		diag (IN) -- diagnostic value to return:
 |			• 0 = frequency diagnostic (kHz)
 |			• 1 = decay period diagnostic (us)
 |			• 2 = die temperature (degC)
 |			• 3 = noise level (8bit)
 |
 |  Returns:  double representation of last captured diagnostic
 *-------------------------------------------------------------------*/
double pga460_runDiagnostics(uint8_t run, uint8_t diag)
{
	double diagReturn = 0;

	int elementOffset = 0; //Only non-zero for OWU mode.

	if (run == 1) // issue  P1 burst+listen, and run system diagnostics command to get latest results
	{
		// run burst+listen command at least once for proper diagnostic analysis
		pga460_ultrasonicCmd(0, 1);	// always run preset 1 (short distance) burst+listen for 1 object for system diagnostic


		//HAL_Delay(100); // record time length maximum of 65ms, so add margin


		uint8_t buf8[3] = {syncByte, SD, calcChecksum(SD)};


		spiTransfer(buf8, sizeof(buf8));

		// MOSI transmit 0xFF to pull MISO return data:
		spiMosiIdle(4);  // 1 diag byte, 2 data bytes, 1 checksum byte

		for(int n=0; n<3; n++)
		{
			diagMeasResult[n] = misoBuf[n];
		}

	}


	if (diag == 2) //run temperature measurement
	{
		tempOrNoise = 0; // temp meas
		uint8_t buf4[4] = {syncByte, TNLM, tempOrNoise, calcChecksum(TNLM)};


		spiTransfer(buf4, sizeof(buf4));


		uint8_t buf6[3] = {syncByte, TNLR, calcChecksum(TNLR)};


		spiTransfer(buf6, sizeof(buf6));

		spiMosiIdle(4);  // 1 diag byte, 2 data bytes, 1 checksum byte


		//HAL_Delay(100);
	}

	if (diag == 3) // run noise level meas
	{
		tempOrNoise = 1; // noise meas
		uint8_t buf4[4] = {syncByte, TNLM, tempOrNoise, calcChecksum(TNLM)};

		spiTransfer(buf4, sizeof(buf4));

		HAL_Delay(10);

		uint8_t buf6[3] = {syncByte, TNLR, calcChecksum(TNLR)}; //serial transmit master data to read temperature and noise results

		spiTransfer(buf6, sizeof(buf6));

		spiMosiIdle(4);  // 1 diag byte, 2 data bytes, 1 checksum byte

		//HAL_Delay(100);
	}



	for(int n=0; n<2; n++)
	{
		tempNoiseMeasResult[n] = misoBuf[n];
	}


	// if SPI mode, do not apply array offset

	//elementOffset = -1;


	//HAL_Delay(100);

	switch (diag)
	{
	case 0: // convert to transducer frequency in kHz
	{
		diagReturn = (1 / (diagMeasResult[1+elementOffset] * 0.0000005)) / 1000;
	}
	break;
	case 1: // convert to decay period time in us
	{
		diagReturn = diagMeasResult[2+elementOffset] * 16;
	}
	break;
	case 2: //convert to temperature in degC
	{
		diagReturn = (tempNoiseMeasResult[1+elementOffset] - 64) / 1.5;
	}
	break;
	case 3: //noise floor level
	{
		diagReturn = tempNoiseMeasResult[2+elementOffset];
	}
	break;
	default: break;
	}

	return diagReturn;
}


void pga460_pullDiagnosticBytes(void)
{
	int retry_count = 0;
	uint8_t buf8[3] = {syncByte, SD, calcChecksum(SD)};

	TryAgain:

	memset(diagMeasResult, 0, sizeof(diagMeasResult));

	spiTransfer(buf8, sizeof(buf8));

	// MOSI transmit 0xFF to pull MISO return data:
	spiMosiIdle(4);  // 1 diag byte, 2 data bytes, 1 checksum byte

#ifdef ENABLE_RETRY

	// See section 7.3.6.2.1.6.3 Response Operation on page 41 of datasheet
	// If diag data field reports busy (bit 0 is 1) the results returned are suspect.

	if (misoBuf[0] != 0x40)
	{
      char msg[80];
      retry_count++;
      sprintf(msg, "  pullDiag err %02X, %02X, %02X, Retry %d  ", misoBuf[0], misoBuf[1], misoBuf[2], retry_count);
      trace_AppendMsg(msg);

	  if (retry_count <= MAX_RETRY)
	  {
		  HAL_Delay(RETRY_DELAY_MS);

		  goto TryAgain;
	  }

	  return;

	}
#endif

	for(int n=0; n<4; n++)
	{
		diagMeasResult[n] = misoBuf[n];
	}
}

void pga460_get_system_diagnostics(float *pFrequency, float *pDecayPeriod)  // kHz, us
{

	// convert to transducer frequency in kHz

	*pFrequency = (1 / (diagMeasResult[1] * 0.0000005)) / 1000.0;

	// convert to decay period time in us

	*pDecayPeriod = diagMeasResult[2] * 16.0;

}

/*------------------------------------------------- burnEEPROM -----
 |  Function burnEEPROM
 |
 |  Purpose:  Burns the EEPROM to preserve the working/shadow register values to EEPROM after power
 |		cycling the PGA460 device. Returns EE_PGRM_OK bit to determine if EEPROM burn was successful.
 |
 |  Parameters:
 |		none
 |
 |  Returns:  bool representation of EEPROM program success
 *-------------------------------------------------------------------*/
uint8_t pga460_burnEEPROM()
{
	uint8_t burnStat = 0;

	uint8_t burnSuccess = PGA460_FALSE;



	// Write "0xD" to EE_UNLCK to unlock EEPROM, and '0' to EEPRGM bit at EE_CNTRL register
	regAddr = 0x40; //EE_CNTRL
	regData = 0x68;
	uint8_t buf10[5] = {syncByte, SRW, regAddr, regData, calcChecksum(SRW)};


	spiTransfer(buf10, sizeof(buf10));


	HAL_Delay(1);

	// Write "0xD" to EE_UNLCK to unlock EEPROM, and '1' to EEPRGM bit at EE_CNTRL register
	regAddr = 0x40; //EE_CNTRL
	regData = 0x69;
	buf10[2] = regAddr;
	buf10[3] = regData;
	buf10[4] = calcChecksum(SRW);


	spiTransfer(buf10, sizeof(buf10));

	HAL_Delay(1000);


	// Read back EEPROM program status

	regAddr = 0x40; //EE_CNTRL
	uint8_t buf9[4] = {syncByte, SRR, regAddr, calcChecksum(SRR)};


	spiTransfer(buf9, sizeof(buf9));

	HAL_Delay(10);


	spiMosiIdle(3);  // 1 diag byte, 1 data byte, 1 checksum byte

	burnStat = misoBuf[1];


	if((burnStat & 0x04) == 0x04){burnSuccess = PGA460_TRUE;} // check if EE_PGRM_OK bit is '1'

	return burnSuccess;
}

/*------------------------------------------------- broadcast -----
 |  Function broadcast
 |
 |  Purpose:  Send a broadcast command to bulk write the user EEPROM, TVG, and/or Threshold values for all devices, regardless of UART_ADDR.
 |		Placehold for user EEPROM broadcast available. Note, all devices will update to the same UART_ADDR in user EEPROM broadcast command.
 |		This function is not applicable to TCI mode.
 |
 |  Parameters:
 |		eeBulk (IN) -- if true, broadcast user EEPROM
 |		tvgBulk (IN) -- if true, broadcast TVG
 |		thrBulk (IN) -- if true, broadcast Threshold
 |
 |  Returns: none
 *-------------------------------------------------------------------*/
void pga460_broadcast(uint8_t eeBulk, uint8_t tvgBulk, uint8_t thrBulk)
{

	// TVG broadcast command:
	if (tvgBulk == PGA460_TRUE)
	{
		uint8_t buf24[10] = {syncByte, BC_TVGBW, TVGAIN0, TVGAIN1, TVGAIN2, TVGAIN3, TVGAIN4, TVGAIN5, TVGAIN6, calcChecksum(BC_TVGBW)};


		spiTransfer(buf24, sizeof(buf24));

		HAL_Delay(10);
	}

	// Threshold broadcast command:
	if (thrBulk == PGA460_TRUE)
	{
		uint8_t buf25[35] = {syncByte, BC_THRBW, P1_THR_0, P1_THR_1, P1_THR_2, P1_THR_3, P1_THR_4, P1_THR_5, P1_THR_6,
				P1_THR_7, P1_THR_8, P1_THR_9, P1_THR_10, P1_THR_11, P1_THR_12, P1_THR_13, P1_THR_14, P1_THR_15,
				P2_THR_0, P2_THR_1, P2_THR_2, P2_THR_3, P2_THR_4, P2_THR_5, P2_THR_6,
				P2_THR_7, P2_THR_8, P2_THR_9, P2_THR_10, P2_THR_11, P2_THR_12, P2_THR_13, P2_THR_14, P2_THR_15,
				calcChecksum(BC_THRBW)};



		spiTransfer(buf25, sizeof(buf25));

		HAL_Delay(10);
	}

	// User EEPROM broadcast command (placeholder):
	if (eeBulk == PGA460_TRUE)
	{
		uint8_t buf23[46] = {syncByte, BC_EEBW, USER_DATA1, USER_DATA2, USER_DATA3, USER_DATA4, USER_DATA5, USER_DATA6,
				USER_DATA7, USER_DATA8, USER_DATA9, USER_DATA10, USER_DATA11, USER_DATA12, USER_DATA13, USER_DATA14,
				USER_DATA15,USER_DATA16,USER_DATA17,USER_DATA18,USER_DATA19,USER_DATA20,
				TVGAIN0,TVGAIN1,TVGAIN2,TVGAIN3,TVGAIN4,TVGAIN5,TVGAIN6,INIT_GAIN,FREQUENCY,DEADTIME,
				PULSE_P1,PULSE_P2,CURR_LIM_P1,CURR_LIM_P2,REC_LENGTH,FREQ_DIAG,SAT_FDIAG_TH,FVOLT_DEC,DECPL_TEMP,
				DSP_SCALE,TEMP_TRIM,P1_GAIN_CTRL,P2_GAIN_CTRL,calcChecksum(BC_EEBW)};



		spiTransfer(buf23, sizeof(buf23));

		HAL_Delay(50);
	}
}


/*------------------------------------------------- calcChecksum -----
 |  Function calcChecksum
 |
 |  Purpose:  Calculates the UART checksum value based on the selected command and the user EERPOM values associated with the command
 |		This function is not applicable to TCI mode. 
 |
 |  Parameters:
 |		cmd (IN) -- the UART command for which the checksum should be calculated for
 |
 |  Returns: uint8_t representation of calculated checksum value
 *-------------------------------------------------------------------*/
static uint8_t calcChecksum(uint8_t cmd)
{
	int checksumLoops = 0;

	cmd = cmd & 0x001F; // zero-mask command address of cmd to select correct switch-case statement

	switch(cmd)
	{
	case 0 : //P1BL
	case 1 : //P2BL
	case 2 : //P1LO
	case 3 : //P2LO
	case 17 : //BC_P1BL
	case 18 : //BC_P2BL
	case 19 : //BC_P1LO
	case 20 : //BC_P2LO
		ChecksumInput[0] = cmd;
		ChecksumInput[1] = numObj;
		checksumLoops = 2;
		break;
	case 4 : //TNLM
	case 21 : //TNLM
		ChecksumInput[0] = cmd;
		ChecksumInput[1] = tempOrNoise;
		checksumLoops = 2;
		break;
	case 5 : //UMR
	case 6 : //TNLR
	case 7 : //TEDD
	case 8 : //SD
	case 11 : //EEBR
	case 13 : //TVGBR
	case 15 : //THRBR
		ChecksumInput[0] = cmd;
		checksumLoops = 1;
		break;
	case 9 : //RR
		ChecksumInput[0] = cmd;
		ChecksumInput[1] = regAddr;
		checksumLoops = 2;
		break;
	case 10 : //RW
	case 22 : //BC_RW
		ChecksumInput[0] = cmd;
		ChecksumInput[1] = regAddr;
		ChecksumInput[2] = regData;
		checksumLoops = 3;
		break;
	case 14 : //TVGBW
	case 24 : //BC_TVGBW
		ChecksumInput[0] = cmd;
		ChecksumInput[1] = TVGAIN0;
		ChecksumInput[2] = TVGAIN1;
		ChecksumInput[3] = TVGAIN2;
		ChecksumInput[4] = TVGAIN3;
		ChecksumInput[5] = TVGAIN4;
		ChecksumInput[6] = TVGAIN5;
		ChecksumInput[7] = TVGAIN6;
		checksumLoops = 8;
		break;
	case 16 : //THRBW
	case 25 : //BC_THRBW
		ChecksumInput[0] = cmd;
		ChecksumInput[1] = P1_THR_0;
		ChecksumInput[2] = P1_THR_1;
		ChecksumInput[3] = P1_THR_2;
		ChecksumInput[4] = P1_THR_3;
		ChecksumInput[5] = P1_THR_4;
		ChecksumInput[6] = P1_THR_5;
		ChecksumInput[7] = P1_THR_6;
		ChecksumInput[8] = P1_THR_7;
		ChecksumInput[9] = P1_THR_8;
		ChecksumInput[10] = P1_THR_9;
		ChecksumInput[11] = P1_THR_10;
		ChecksumInput[12] = P1_THR_11;
		ChecksumInput[13] = P1_THR_12;
		ChecksumInput[14] = P1_THR_13;
		ChecksumInput[15] = P1_THR_14;
		ChecksumInput[16] = P1_THR_15;
		ChecksumInput[17] = P2_THR_0;
		ChecksumInput[18] = P2_THR_1;
		ChecksumInput[19] = P2_THR_2;
		ChecksumInput[20] = P2_THR_3;
		ChecksumInput[21] = P2_THR_4;
		ChecksumInput[22] = P2_THR_5;
		ChecksumInput[23] = P2_THR_6;
		ChecksumInput[24] = P2_THR_7;
		ChecksumInput[25] = P2_THR_8;
		ChecksumInput[26] = P2_THR_9;
		ChecksumInput[27] = P2_THR_10;
		ChecksumInput[28] = P2_THR_11;
		ChecksumInput[29] = P2_THR_12;
		ChecksumInput[30] = P2_THR_13;
		ChecksumInput[31] = P2_THR_14;
		ChecksumInput[32] = P2_THR_15;
		checksumLoops = 33;
		break;
	case 12 : //EEBW
	case 23 : //BC_EEBW
		ChecksumInput[0] = cmd;
		ChecksumInput[1] = USER_DATA1;
		ChecksumInput[2] = USER_DATA2;
		ChecksumInput[3] = USER_DATA3;
		ChecksumInput[4] = USER_DATA4;
		ChecksumInput[5] = USER_DATA5;
		ChecksumInput[6] = USER_DATA6;
		ChecksumInput[7] = USER_DATA7;
		ChecksumInput[8] = USER_DATA8;
		ChecksumInput[9] = USER_DATA9;
		ChecksumInput[10] = USER_DATA10;
		ChecksumInput[11] = USER_DATA11;
		ChecksumInput[12] = USER_DATA12;
		ChecksumInput[13] = USER_DATA13;
		ChecksumInput[14] = USER_DATA14;
		ChecksumInput[15] = USER_DATA15;
		ChecksumInput[16] = USER_DATA16;
		ChecksumInput[17] = USER_DATA17;
		ChecksumInput[18] = USER_DATA18;
		ChecksumInput[19] = USER_DATA19;
		ChecksumInput[20] = USER_DATA20;
		ChecksumInput[21] = TVGAIN0;
		ChecksumInput[22] = TVGAIN1;
		ChecksumInput[23] = TVGAIN2;
		ChecksumInput[24] = TVGAIN3;
		ChecksumInput[25] = TVGAIN4;
		ChecksumInput[26] = TVGAIN5;
		ChecksumInput[27] = TVGAIN6;
		ChecksumInput[28] = INIT_GAIN;
		ChecksumInput[29] = FREQUENCY;
		ChecksumInput[30] = DEADTIME;
		ChecksumInput[31] = PULSE_P1;
		ChecksumInput[32] = PULSE_P2;
		ChecksumInput[33] = CURR_LIM_P1;
		ChecksumInput[34] = CURR_LIM_P2;
		ChecksumInput[35] = REC_LENGTH;
		ChecksumInput[36] = FREQ_DIAG;
		ChecksumInput[37] = SAT_FDIAG_TH;
		ChecksumInput[38] = FVOLT_DEC;
		ChecksumInput[39] = DECPL_TEMP;
		ChecksumInput[40] = DSP_SCALE;
		ChecksumInput[41] = TEMP_TRIM;
		ChecksumInput[42] = P1_GAIN_CTRL;
		ChecksumInput[43] = P2_GAIN_CTRL;
		checksumLoops = 44;
		break;
	default: break;
	}

	if (ChecksumInput[0]<17) //only re-append command address for non-broadcast commands.
	{
		ChecksumInput[0] = ChecksumInput[0] + (uartAddr << 5);
	}

	uint16_t carry = 0;

	for (int i = 0; i < checksumLoops; i++)
	{
		if ((ChecksumInput[i] + carry) < carry)
		{
			carry = carry + ChecksumInput[i] + 1;
		}
		else
		{
			carry = carry + ChecksumInput[i];
		}

		if (carry > 0xFF)
		{
			carry = carry - 255;
		}
	}

	carry = (~carry & 0x00FF);
	return carry;
}





/*------------------------------------------------- spiTransfer -----
 |  Function spiTransfer
 |
 |  Purpose:  Transfers one uint8_t over the SPI bus, both sending and receiving. 
 |			Captures MISO data in global uint8_t-array.
 |
 |  Parameters:
 |		mosi (IN) -- MOSI data uint8_t array to transmit over SPI
 |		size (IN) -- size of MOSI data uint8_t array
 |
 |  Returns: uint8_t representation of calculated checksum value
 *-------------------------------------------------------------------*/
#include "factory.h"
static void spiTransfer(uint8_t* mosi, uint8_t size )
{

#ifdef	ENABLE_PGA460_USE_UART3

	if (BoardId == BOARD_ID_SEWERWATCH_1)
	{
		lpuart1_flush_inbuffer();  // must flush the inbuffer
		lpuart1_tx_bytes(mosi, size, sd_EVM_460_Trace);  // note: not used in pass-thru mode
	}
	else
	{
		uart3_flush_inbuffer();  // must flush the inbuffer
		uart3_tx_bytes(mosi, size, sd_EVM_460_Trace);  // note: not used in pass-thru mode
	}


#else
	memset(misoBuf, 0x00, sizeof(misoBuf)); // idle-low receive buffer data
	for (int i = 0; i<size; i++)
	{
		//misoBuf[i] = usscSPI.transfer(mosi[i]);
		HAL_SPI_TransmitReceive(&hspi1, &mosi[i], &misoBuf[i], 1, HAL_MAX_DELAY);
	}
#endif

}

/*------------------------------------------------- spiMosiIdle-----
 |  Function spiMosiIdle
 |
 |  Purpose:  Forces MOSI of 0xFF to idle high master output, while
 |			MISO pin returns valid data.
 |
 |  Parameters:
 |		size (IN) -- number of MISO data uint8_ts expected from slave
 |
 |  Returns: none
 *-------------------------------------------------------------------*/
static void spiMosiIdle(uint8_t size)
{
	uint32_t timeout_ms;

	memset(misoBuf, 0x00, sizeof(misoBuf)); // idle-low receive buffer data

	//timeout_ms = 1000;  // Original hardcoded timeout value
	timeout_ms = size;  // The timeout value should be based on number of bytes to RX.  At 38400, each byte should take less than 1 ms.

#ifdef	ENABLE_PGA460_USE_UART3

	if (BoardId == BOARD_ID_SEWERWATCH_1)
	{
		lpuart1_rx_bytes(misoBuf, size, timeout_ms);
	}
	else
	{
		uart3_rx_bytes(misoBuf, size, timeout_ms);
	}

#else

	for (int i = 0; i<size; i++)
	{
		//misoBuf[i] = usscSPI.transfer(0xFE);
		HAL_SPI_Receive(&hspi1, &misoBuf[i], 1, HAL_MAX_DELAY);
	}
#endif
}




/*------------------------------------------------- triangulation -----
 |  Function triangulation
 |
 |  Purpose:  Uses the law of cosines to compute the position of the
 |			targeted object from transceiver S1.
 |
 |  Parameters:
 |		distanceA (IN) -- distance (m) from sensor module 1 (S1) to the targeted object based on UMR result
 |		distanceB (IN) -- distance (m) between sensor module 1 (S1) and sensor module 2 (S2)
 |		distanceC (IN) -- distance (m) from sensor module 2 (S2) to the targeted object based on UMR result
 |
 |  Returns:  angle (degrees) from transceiver element S1 to the targeted object
 *-------------------------------------------------------------------*/
void pga460_triangulation(double a, double b, double c)
{
#if 0
	// LAW OF COSINES
	double inAngle;
	if (a+b>c)
	{
		return inAngle =(acos(((a*a)+(b*b)-(c*c))/(2*a*b))) * 57.3; //Radian to Degree = Rad * (180/PI)
	}
	else
	{
		return 360;
	}

	// COORDINATE
	// TODO
#endif
}

/*------------------------------------------------- registerRead -----
 |  Function registerRead
 |
 |  Purpose:  Read single register data from PGA460
 |
 |  Parameters:
 |		addr (IN) -- PGA460 register address to read data from
 |
 |  Returns:  8-bit data read from register
 *-------------------------------------------------------------------*/
uint8_t pga460_registerRead(uint8_t addr)
{
	regAddr = addr;

	uint8_t buf9[4] = {syncByte, SRR, regAddr, calcChecksum(SRR)};

	spiTransfer(buf9, sizeof(buf9));

	//HAL_Delay(10);  // zg - why delay 10 ms ????

	spiMosiIdle(3);  // 1 diag byte, 1 data byte, 1 checksum byte

	return misoBuf[1];
}

/*------------------------------------------------- registerWrite -----
 |  Function registerWrite
 |
 |  Purpose:  Write single register data to PGA460
 |
 |  Parameters:
 |		addr (IN) -- PGA460 register address to write data to
 |		data (IN) -- 8-bit data value to write into register
 |
 |  Returns:  none
 *-------------------------------------------------------------------*/
void pga460_registerWrite(uint8_t addr, uint8_t data)
{
	regAddr = addr;
	regData = data;	
	uint8_t buf10[5] = {syncByte, SRW, regAddr, regData, calcChecksum(SRW)};

	spiTransfer(buf10, sizeof(buf10));

	// Datasheet Table 3 footnote (page 38) says to wait 60 us after writing
	delay_us(61);
	//HAL_Delay(1);

}

/*------------------------------------------------- autoThreshold -----
 |  Function autoThreshold
 |
 |  Purpose:  Automatically assigns threshold time and level values
 |  			based on a no-object burst/listen command
 |
 |  Parameters:
 |		cmd (IN) -- preset 1 or 2 burst and/or listen command to run
 |		noiseMargin (IN) -- margin between maximum downsampled noise
 |						value and the threshold level in intervals
 |						of 8.
 |		windowIndex (IN) -- spacing between each threshold time as an
 |						index (refer to datasheet for microsecond
 |						equivalent). To use the existing threshold
 |						times, enter a value of '16'.
 |		autoMax (IN) -- automatically set threshold levels up to this
 |					threshold point (maximum is 12). Remaining levels
 |					will not change.
 |		loops (IN) -- number of command loops to run to collect a
 |					running average of the echo data dump points.
 |
 |  Returns:  none
 *-------------------------------------------------------------------*/
void pga460_autoThreshold(uint8_t cmd, uint8_t noiseMargin, uint8_t windowIndex, uint8_t autoMax, uint8_t avgLoops)
{
#ifdef EnAutoThr
	// local variables
	uint8_t thrTime[6]; // threshold time values for selected preset
	uint8_t thrLevel[10]; //threshold level values for selected preset
	uint8_t thrMax[12]; // maximum echo data dump values per partition
	uint8_t presetOffset = 0; // determines if regsiter threshold address space is initialized at P1 or P2
	uint8_t thrOffset = 0; // -6 to +7 where MSB is sign value
	uint8_t thrOffsetFlag = 0; //when high, the level offset value is updated

	//read existing threshold values into thrTime array
	switch (cmd)
	{
	//Preset 1 command
	case 0:
	case 2:
		pga460_thresholdBulkRead(1);
		break;
		//Preset 2 command
	case 1:
	case 3:
		pga460_thresholdBulkRead(2);
		presetOffset = 16;
		break;
		//Invalid command
	default:
		return;
		break;
	}

	// set thrTime and thrLevel to existing threshold time and level values respectively
	for (uint8_t h = 0; h<6; h++)
	{
		thrTime[h] = bulkThr[h + presetOffset];
	}
	for (uint8_t g = 0; g<10; g++)
	{
		thrLevel[g] = bulkThr[g + 6 + presetOffset];
	}

	// replace each preset time with windowIndex for the number of points to auto-calc
	if (windowIndex >= 16)
	{
		//skip threshold-time configuration
	}
	else
	{
		for (uint8_t i = 0; i < 12; i+=2)
		{	

			if (autoMax > i)
			{
				thrTime[i/2] = thrTime[i/2] & 0x0F;
				thrTime[i/2] = (windowIndex << 4) | thrTime[i/2];
				if (autoMax > i+1)
				{
					thrTime[i/2] = thrTime[i/2] & 0xF0;
					thrTime[i/2] = (windowIndex & 0x0F) | thrTime[i/2];
				}
			}
		}
	}

	// run burst-and-listen to collect EDD data
	pga460_runEchoDataDump(cmd);

	// read the record length value for the preset
	uint8_t recLength = pga460_registerRead(0x22); // read REC_LENGTH Register
	switch(cmd)
	{
	//Preset 1 command
	case 0:
	case 2:
		recLength = (recLength >> 4) & 0x0F;
		break;
		//Preset 2 command
	case 1:
	case 3:
		recLength = recLength & 0x0F;
		break;
		//Invalid command
	default:
		return;
		break;
	}

	// convert record length value to time equivalent in microseconds
	unsigned int recTime = (recLength + 1) * 4096;

	//determine the number of threshold points that are within the record length time
	uint8_t numPoints = 0;
	uint8_t thrTimeReg = 0;
	unsigned int thrMicro = 0; // Threshold total time in microseconds
	unsigned int eddMarker[12]; // echo data dump time marker between each threshold point
	for (thrTimeReg = 0; thrTimeReg < 6; thrTimeReg++)
	{		
		// check threshold 1 of 2 in single register
		switch ((thrTime[thrTimeReg] >> 4) & 0x0F)
		{			
		case 0: thrMicro += 100; break;
		case 1: thrMicro += 200; break;
		case 2: thrMicro += 300; break;
		case 3: thrMicro += 400; break;
		case 4: thrMicro += 600; break;
		case 5: thrMicro += 800; break;
		case 6: thrMicro += 1000; break;
		case 7: thrMicro += 1200; break;
		case 8: thrMicro += 1400; break;
		case 9: thrMicro += 2000; break;
		case 10: thrMicro += 2400; break;
		case 11: thrMicro += 3200; break;
		case 12: thrMicro += 4000; break;
		case 13: thrMicro += 5200; break;
		case 14: thrMicro += 6400; break;
		case 15: thrMicro += 8000; break;
		default: break;
		}
		eddMarker[thrTimeReg*2] = thrMicro;
		if (thrMicro >= recTime)
		{
			numPoints = thrTimeReg * 2;
			thrTimeReg = 6; //exit
		}
		else
		{
			// check threshold 2 of 2 in single register
			switch (thrTime[thrTimeReg] & 0x0F)
			{
			case 0: thrMicro += 100; break;
			case 1: thrMicro += 200; break;
			case 2: thrMicro += 300; break;
			case 3: thrMicro += 400; break;
			case 4: thrMicro += 600; break;
			case 5: thrMicro += 800; break;
			case 6: thrMicro += 1000; break;
			case 7: thrMicro += 1200; break;
			case 8: thrMicro += 1400; break;
			case 9: thrMicro += 2000; break;
			case 10: thrMicro += 2400; break;
			case 11: thrMicro += 3200; break;
			case 12: thrMicro += 4000; break;
			case 13: thrMicro += 5200; break;
			case 14: thrMicro += 6400; break;
			case 15: thrMicro += 8000; break;
			default: break;
			}
			eddMarker[thrTimeReg*2+1] = thrMicro;
			if (thrMicro >= recTime)
			{
				numPoints = (thrTimeReg * 2) + 1;
				thrTimeReg = 6; //exit
			}
		}	
	}
	if (numPoints == 0) //if all points fall within the record length
	{
		numPoints = 11;
	}

	//convert up to 12 echo data dump markers from microseconds to index
	uint8_t eddIndex[13];
	eddIndex[0] = 0;
	for (uint8_t l = 0; l < 12; l++)
	{
		eddIndex[l+1] = ((eddMarker[l]/100)*128)/(recTime/100); // divide by 100 for best accuracy in MSP430
	}	

	// downsample the echo data dump based on the number of partitions
	memset(thrMax, 0x00, 12); // zero thrMax array
	uint8_t eddLevel = 0;
	for (uint8_t j = 0; j < numPoints+1; j++)
	{	
		eddLevel = 0;
		for (uint8_t k = eddIndex[j]; k < eddIndex[j+1]; k++)
		{
			eddLevel = pga460_pullEchoDataDump(k);
			if (thrMax[j] < eddLevel)
			{
				thrMax[j] = eddLevel;
			}		
		}	
	}
	//set threhsold points which exceed the record length to same value as last valid value
	if (numPoints < autoMax)
	{
		for (int o = numPoints; o < autoMax; o++)
		{
			if (numPoints ==0)
			{
				thrMax[o] = 128;
			}
			else
			{
				thrMax[o] = thrMax[numPoints-1];
			}
		}
	}

	// filter y-max for level compatibility of first eight points
	for (int m = 0; m < 8; m++)
	{
		//first eight levels must be mutliples of eight
		while ((thrMax[m] % 8 != 0) && (thrMax[m] < 248)) 
		{
			thrMax[m] += 1;
		}
	}

	// apply noise floor offset
	for (int n = 0; n < 12; n++)
	{
		if (thrMax[n] + noiseMargin >= 248 && thrMax[n] + noiseMargin < 255)
		{
			thrMax[n] = 248;
			thrOffset = 0b0110; //+6
			thrOffsetFlag = PGA460_TRUE;
		}
		else if (thrMax[n] + noiseMargin >= 255)
		{
			thrMax[n] = 248;
			thrOffset = 0b0111; // +7
			thrOffsetFlag = PGA460_TRUE;
		}
		else
		{
			thrMax[n] += noiseMargin;
		}
	}

	//convert first eight auto calibrated levels to five-bit equivalents
	uint8_t rounding = 0;
	if (autoMax >= 8)
	{
		rounding = 8;
	}
	else
	{
		rounding = autoMax;
	}
	for(uint8_t p = 0; p < rounding; p++)
	{
		thrMax[p] = thrMax[p] / 8;
	}

	// concatenate and merge threshold level register values
	if (autoMax > 0) //Px_THR_6 L1,L2
	{
		thrLevel[0] = (thrLevel[0] & ~0xF8) | (thrMax[0] << 3);
	}
	if (autoMax > 1) //Px_THR_6 L1,L2
	{
		thrLevel[0] = (thrLevel[0] & ~0x07) | (thrMax[1] >> 2);
	}

	if (autoMax > 1) //Px_THR_7 L2,L3,L4
	{
		thrLevel[1] = (thrLevel[1] & ~0xC0) | (thrMax[1] << 6);
	}
	if (autoMax > 2) //Px_THR_7 L2,L3,L4
	{
		thrLevel[1] = (thrLevel[1] & ~0x3E) | (thrMax[2] << 1);
	}
	if (autoMax > 3) //Px_THR_7 L2,L3,L4
	{
		thrLevel[1] = (thrLevel[1] & ~0x01) | (thrMax[3] >> 4 );
	}

	if (autoMax > 3) //Px_THR_8 L4,L5
	{
		thrLevel[2] = (thrLevel[2] & ~0xF0) | (thrMax[3] << 4 );
	}
	if (autoMax > 4) //Px_THR_8 L4,L5
	{
		thrLevel[2] = (thrLevel[2] & ~0x0F) | (thrMax[4] >> 1 );
	}

	if (autoMax > 4) //Px_THR_9 L5,L6,L7
	{
		thrLevel[3] = (thrLevel[3] & ~0x80) | (thrMax[4] << 7 );
	}
	if (autoMax > 5) //Px_THR_9 L5,L6,L7
	{
		thrLevel[3] = (thrLevel[3] & ~0x7C) | (thrMax[5] << 2 );
	}
	if (autoMax > 6) //Px_THR_9 L5,L6,L7
	{
		thrLevel[3] = (thrLevel[3] & ~0x03) | (thrMax[6] >> 3 );
	}

	if (autoMax > 6) //Px_THR_10 L7,L8
	{
		thrLevel[4] = (thrLevel[4] & ~0xE0) | (thrMax[6] << 5 );
	}
	if (autoMax > 7) //Px_THR_10 L7,L8
	{
		thrLevel[4] = (thrLevel[4] & ~0x1F) | (thrMax[7]);
	}

	if (autoMax > 8) //Px_THR_11 L9 
	{
		thrLevel[5] = thrMax[8];
	}
	if (autoMax > 9) //Px_THR_12 L10
	{
		thrLevel[6] = thrMax[9];
	}
	if (autoMax > 10) //Px_THR_13 L11 
	{
		thrLevel[7] = thrMax[10];
	}
	if (autoMax > 11) //Px_THR_14 L12
	{
		thrLevel[8] = thrMax[11];
	}
	if (thrOffsetFlag == PGA460_TRUE) //Px_THR_15 LOff
	{
		thrLevel[9] = thrOffset & 0x0F;
	}

	// update threshold register values
	switch(cmd)
	{
	//Preset 1 command
	case 0:
	case 2:
		P1_THR_0 = thrTime[0];
		P1_THR_1 = thrTime[1];
		P1_THR_2 = thrTime[2];
		P1_THR_3 = thrTime[3];
		P1_THR_4 = thrTime[4];
		P1_THR_5 = thrTime[5];
		P1_THR_6 = thrLevel[0];
		P1_THR_7 = thrLevel[1];
		P1_THR_8 = thrLevel[2];
		P1_THR_9 = thrLevel[3];
		P1_THR_10 = thrLevel[4];
		P1_THR_11 = thrLevel[5];
		P1_THR_12 = thrLevel[6];
		P1_THR_13 = thrLevel[7];
		P1_THR_14 = thrLevel[8];
		P1_THR_15 = thrLevel[9];

		pga460_thresholdBulkRead(2);
		presetOffset = 16;

		P2_THR_0 = bulkThr[0 + presetOffset];
		P2_THR_1 = bulkThr[1 + presetOffset];
		P2_THR_2 = bulkThr[2 + presetOffset];
		P2_THR_3 = bulkThr[3 + presetOffset];
		P2_THR_4 = bulkThr[4 + presetOffset];
		P2_THR_5 = bulkThr[5 + presetOffset];
		P2_THR_6 = bulkThr[6 + presetOffset];
		P2_THR_7 = bulkThr[7 + presetOffset];
		P2_THR_8 = bulkThr[8 + presetOffset];
		P2_THR_9 = bulkThr[9 + presetOffset];
		P2_THR_10 = bulkThr[10 + presetOffset];
		P2_THR_11 = bulkThr[11 + presetOffset];
		P2_THR_12 = bulkThr[12 + presetOffset];
		P2_THR_13 = bulkThr[13 + presetOffset];
		P2_THR_14 = bulkThr[14 + presetOffset];
		P2_THR_15 = bulkThr[15 + presetOffset];
		break;
		//Preset 2 command
	case 1:
	case 3:
		P2_THR_0 = thrTime[0];
		P2_THR_1 = thrTime[1];
		P2_THR_2 = thrTime[2];
		P2_THR_3 = thrTime[3];
		P2_THR_4 = thrTime[4];
		P2_THR_5 = thrTime[5];
		P2_THR_6 = thrLevel[0];
		P2_THR_7 = thrLevel[1];
		P2_THR_8 = thrLevel[2];
		P2_THR_9 = thrLevel[3];
		P2_THR_10 = thrLevel[4];
		P2_THR_11 = thrLevel[5];
		P2_THR_12 = thrLevel[6];
		P2_THR_13 = thrLevel[7];
		P2_THR_14 = thrLevel[8];
		P2_THR_15 = thrLevel[9];

		pga460_thresholdBulkRead(1);
		presetOffset = 0;

		P1_THR_0 = bulkThr[0 + presetOffset];
		P1_THR_1 = bulkThr[1 + presetOffset];
		P1_THR_2 = bulkThr[2 + presetOffset];
		P1_THR_3 = bulkThr[3 + presetOffset];
		P1_THR_4 = bulkThr[4 + presetOffset];
		P1_THR_5 = bulkThr[5 + presetOffset];
		P1_THR_6 = bulkThr[6 + presetOffset];
		P1_THR_7 = bulkThr[7 + presetOffset];
		P1_THR_8 = bulkThr[8 + presetOffset];
		P1_THR_9 = bulkThr[9 + presetOffset];
		P1_THR_10 = bulkThr[10 + presetOffset];
		P1_THR_11 = bulkThr[11 + presetOffset];
		P1_THR_12 = bulkThr[12 + presetOffset];
		P1_THR_13 = bulkThr[13 + presetOffset];
		P1_THR_14 = bulkThr[14 + presetOffset];
		P1_THR_15 = bulkThr[15 + presetOffset];
		break;
		//Invalid command
	default:
		return;
		break;
	}

	uint8_t p1ThrMap[16] = {P1_THR_0, P1_THR_1, P1_THR_2, P1_THR_3, P1_THR_4, P1_THR_5,
			P1_THR_6, P1_THR_7, P1_THR_8, P1_THR_9, P1_THR_10, P1_THR_11,
			P1_THR_12, P1_THR_13, P1_THR_14, P1_THR_15};
	uint8_t p2ThrMap[16] = {P2_THR_0, P2_THR_1, P2_THR_2, P2_THR_3, P2_THR_4, P2_THR_5,
			P2_THR_6, P2_THR_7, P2_THR_8, P2_THR_9, P2_THR_10, P2_THR_11,
			P2_THR_12, P2_THR_13, P2_THR_14, P2_THR_15};

	pga460_thresholdBulkWrite(p1ThrMap, p2ThrMap);

#endif
}

/*------------------------------------------------- thresholdBulkRead -----
 |  Function thresholdBulkRead
 |
 |  Purpose:  Bulk read all threshold times and levels 
 |
 |  Parameters:
 |		preset (IN) -- which preset's threshold data to read
 |
 |  Returns:  none
 *-------------------------------------------------------------------*/
void pga460_thresholdBulkRead(uint8_t preset)
{
#if 0
	uint8_t n = 0;
	uint8_t buf15[2] = {syncByte, THRBR};
	uint8_t presetOffset = 0;
	uint8_t addr = 0x5F; // beginning of threshold register space

	switch (comm)
	{
	case 0:
	case 2:
		if (preset == 2) //Preset 2 advances 16 address uint8_ts
		{
			presetOffset = 16;
		}

		for (int n = 0; n<16; n++)
		{
			bulkThr[n + presetOffset] = pga460_registerRead(addr + presetOffset);
			addr++;
		}

		// Threshold Bulk Read Command 15 too large for Serial1 receive buffer
		/*Serial1.write(buf15, sizeof(buf15));
			HAL_Delay (300);
			while (Serial1.available() > 0)
			{
				bulkThr[n] = Serial1.read();
				n++;
			}*/

		break;

	case 1: //TCI
		//TODO
		break;

	case 3: //SPI
		//TODO
		break;

	default:
		break;
	}
#endif
}

/*------------------------------------------------- thresholdBulkWrite -----
 |  Function thresholdBulkWrite
 |
 |  Purpose:  Bulk write to all threshold registers
 |
 |  Parameters:
 |		p1ThrMap (IN) -- data uint8_t array for 16 uint8_ts of Preset 1 threshold data
  |		p2ThrMap (IN) -- data uint8_t array for 16 uint8_ts of Preset 2 threshold data
 |
 |  Returns:  none
 *-------------------------------------------------------------------*/
void pga460_thresholdBulkWrite(uint8_t *p1ThrMap, uint8_t *p2ThrMap)
{
#ifdef EnAutoThr
	//bulk write new threshold values

	uint8_t buf16[35] = {syncByte,	THRBW, p1ThrMap[0], p1ThrMap[1], p1ThrMap[2], p1ThrMap[3], p1ThrMap[4], p1ThrMap[5],
			p1ThrMap[6], p1ThrMap[7], p1ThrMap[8], p1ThrMap[9], p1ThrMap[10], p1ThrMap[11], p1ThrMap[12],
			p1ThrMap[13], p1ThrMap[14], p1ThrMap[15],
			p2ThrMap[0], p2ThrMap[1], p2ThrMap[2], p2ThrMap[3], p2ThrMap[4], p2ThrMap[5],
			p2ThrMap[6], p2ThrMap[7], p2ThrMap[8], p2ThrMap[9], p2ThrMap[10], p2ThrMap[11], p2ThrMap[12],
			p2ThrMap[13], p2ThrMap[14], p2ThrMap[15],
			calcChecksum(THRBW)};


	spiTransfer(buf16, sizeof(buf16));


	HAL_Delay(100);
	return;
#endif
}

/*------------------------------------------------- eepromThreshold -----
 |  Function eepromThreshold
 |
 |  Purpose:  Copy a single preset's threshold times and levels 
 |  			to USER_DATA1-16 in EEPROM
 |
 |  Parameters:
 |		preset (IN) -- preset's threshold to copy
 |		saveLoad (IN) -- when false, copy threshold to EEPROM;
 |					when true, copy threshold from EEPROM
 |
 |  Returns:  none
 *-------------------------------------------------------------------*/
void pga460_eepromThreshold(uint8_t preset, uint8_t saveLoad)
{
#ifdef EnAutoThr
	uint8_t presetOffset = 0;
	uint8_t addr = 0x5F; // beginning of threshold memory space

	if (saveLoad == PGA460_FALSE) // save thr
	{
		//Preset 2 advances 16 address uint8_ts
		if (preset == 2 || preset == 4) 
		{
			presetOffset = 16;
		}

		for (int n = 0; n<16; n++)
		{
			bulkThr[n + presetOffset] = pga460_registerRead(addr + presetOffset);
			// write threshold values into USER_DATA1-16
			pga460_registerWrite(n, bulkThr[n + presetOffset]);
			addr++;
		}
	}
	else // load thr
	{
		//Preset 2 advances 16 address uint8_ts
		if (preset == 2 || preset == 4) //Preset 2 advances 16 address uint8_ts
		{
			presetOffset = 16;
		}

		// copy USER_DATA1-16 into selected preset threshold space
		for (int n = 0; n<16; n++)
		{
			bulkThr[n + presetOffset] = pga460_registerRead(n);
			// bulk write to threshold
			pga460_registerWrite(addr + presetOffset, bulkThr[n + presetOffset]);
			addr++;
		}
	}
#endif
}


#endif
