/*
	PGA460_USSC.h

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

	Last Updated: Nov 2017
	By: A. Whitehead <make@energia.nu>
 */

#define PGA460_PRESET_1  0
#define PGA460_PRESET_2  1

typedef struct
{
	uint16_t raw_distance; // time-of-flight in us
	uint8_t  raw_width;
	uint8_t  raw_peak;
	float    distance;    // inches
	uint16_t width;       // us
} PGA460_ECHO_DATA;       // 10 bytes total

//#define PGA460_OBJ_1  0
//#define PGA460_OBJ_2  1   // and so on...

#define PGA460_ECHOS  1

typedef struct
{
	uint8_t              raw_echo_uart;
	PGA460_ECHO_DATA     echo[PGA460_ECHOS];
	uint8_t              raw_diag_uart;
	uint8_t              raw_diag_freq;
	uint8_t              raw_diag_decay;
	float                diag_freq;  // kHz
	uint16_t             diag_decay; // us
} PGA460_DATA_POINT;   //

typedef struct
{
	//uint32_t raw_diag_freq;
	//uint32_t raw_diag_decay;

	//float diag_freq;  // kHz
	//float diag_decay; // us

	float distance;    // inches
	float width;       // us
	float raw_peak;
}  PGA460_AVE_DATA;




void pga460_update_data_point(PGA460_DATA_POINT *p, float speedSound_inches_per_us);
void pga460_perform_preset(int preset, PGA460_DATA_POINT *p, float speedSound_inches_per_us);


uint8_t pga460_pullEchoDataDump(uint8_t element);
uint8_t pga460_registerRead(uint8_t addr);
void    pga460_registerWrite(uint8_t addr, uint8_t data);
void    pga460_initBoostXLPGA460(uint8_t mode, uint32_t baud, uint8_t uartAddrUpdate);
void    pga460_defaultPGA460(uint8_t xdcr);
void    pga460_initThresholds(uint8_t thr);
void    pga460_initTVG(uint8_t agr, uint8_t tvg);
void    pga460_ultrasonicCmd(uint8_t cmd, uint8_t numObjUpdate);
void    pga460_runEchoDataDump(uint8_t preset);
void    pga460_broadcast(uint8_t eeBulk, uint8_t tvgBulk, uint8_t thrBulk);

void    pga460_autoThreshold(uint8_t cmd, uint8_t noiseMargin, uint8_t windowIndex, uint8_t autoMax, uint8_t avgLoops);
void    pga460_eepromThreshold(uint8_t preset, uint8_t saveLoad);
void    pga460_thresholdBulkRead(uint8_t preset);
void    pga460_thresholdBulkWrite(uint8_t p1ThrMap[], uint8_t p2ThrMap[]);
uint8_t pga460_burnEEPROM();
void    pga460_pullUltrasonicMeasResult(void);
void    pga460_pullDiagnosticBytes(void);
double  pga460_printUltrasonicMeasResult(uint8_t umr);

double  pga460_printUltrasonicMeasResultExt(uint8_t umr, double speedSound_us);
double  pga460_runDiagnostics(uint8_t run, uint8_t diag);
void    pga460_get_system_diagnostics(float *pFrequency, float *pDecayPeriod);  // kHz, us
void    pga460_triangulation(double a, double b, double c);
void    pga460_pullEchoDataDumpBulk();

void    pga460_log_msg(char * msg);




#define PGA460_TRUE  1
#define PGA460_FALSE 0






