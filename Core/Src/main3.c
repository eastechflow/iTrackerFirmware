


//#include "all_header.h"
#include "usart.h"



#include "uart1.h"
#include "uart2.h"
#include "uart3.h"
#include "uart4.h"
#include "lpuart1.h"
#include "debug_helper.h"
#include "trace.h"
#include "factory.h"

#include "PGA460_USSC.h"

//#define USE_CONSOLE_FOR_PGA460

#ifdef ENABLE_PGA460

void pga460_log_msg(char * msg)
{
	if (sd_TraceSensor)
	{
      TraceSensor_AppendMsg(msg);
	}
}

void tx_msg(char * msg)
{
#ifdef USE_UART1_AND_LPUART1
	uart1_tx_string(msg);  // for iTracker
#endif

#ifdef USE_UART4_AND_LPUART1
	uart4_tx_string(msg);  // for SewerWatch
#endif

}



/*------------------------------------------------- Global Variables -----
|  Global Variables
|
|  Purpose:  Variables shared throughout the GetDistance sketch for
|    both userInput and standAlone modes. Hard-code these values to
|    the desired conditions when automatically updating the device
|    in standAlone mode.
*-------------------------------------------------------------------*/

// Configuration variables
  uint8_t commMode = 0;            // Communication mode: 0=UART, 1=TCI, 2=OneWireUART
  uint8_t fixedThr = 1;            // set P1 and P2 thresholds to 0=%25, 1=50%, or 2=75% of max; initial minDistLim (i.e. 20cm) ignored
  uint8_t xdcr = 1;                // set PGA460 to recommended settings for 0=Murata MA58MF14-7N, 1=Murata MA40H1S-R
  uint8_t agrTVG = 2;              // set TVG's analog front end gain range to 0=32-64dB, 1=46-78dB, 2=52-84dB, or 3=58-90dB
  uint8_t fixedTVG = 1;            // set fixed TVG level at 0=%25, 1=50%, or 1=75% of max
  uint8_t runDiag = 0;             // run system diagnostics and temp/noise level before looping burst+listen command
  uint8_t edd = 0;                 // echo data dump of preset 1, 2, or neither
  uint8_t burn = 0;                // trigger EE_CNTRL to burn and program user EEPROM memory
  uint8_t cdMultiplier = 1;        // multiplier for command cycle delay
  uint8_t numOfObj = 1;            // number of object to detect set to 1-8
  uint8_t uartAddrUpdate = 0;      // PGA460 UART address to interface to; default is 0, possible address 0-7
  uint8_t objectDetected = PGA460_FALSE;  // object detected flag to break burst+listen cycle when PGA460_TRUE
  uint8_t demoMode = PGA460_FALSE;        // only PGA460_TRUE when running UART/OWU multi device demo mode
  uint8_t alwaysLong = PGA460_FALSE;      // always run preset 2, regardless of preset 1 result (hard-coded only)
  double minDistLim = 0.1;      // minimum distance as limited by ringing decay of single transducer and threshold masking
  uint16_t commandDelay = 0;    // Delay between each P1 and Preset 2 command
  uint32_t baudRate = 9600;     // UART baud rate: 9600, 19200, 38400, 57600, 74800, 115200

//PUSH BUTTON used for standAlone mode
  //const int buttonPin = PUSH2;  // the number of the pushbutton pin
  //int buttonState = 0;          // variable for reading the pushbutton status

// Result variables
  double distance = 0;          // one-way object distance using preset 1
  double distance2 = 0;         // one-way object distance using preset 2
  double width = 0;             // object width in microseconds
  double peak = 0;              // object peak in 8-bit
  double diagnostics = 0;       // diagnostic selector
  uint8_t echoDataDumpElement = 0; // echo data dump element 0 to 127
  //String interruptString = "";  // a string to hold incoming data
  uint8_t stringComplete = PGA460_FALSE; // whether the string is complete

// PGA460_USSC library class
//pga460 ussc;


#ifdef USE_CONSOLE_FOR_PGA460

  static void display_header(void)
  {

      HAL_Delay(1000);
      tx_msg("PGA460-Q1 EVM SPI Energia Demo for Ultrasonic Time-of-Flight\r\n");
      tx_msg("-------------------------------------------------------------------------\r\n");
      tx_msg("Instructions: Configure the EVM by entering a uint8_t value between 0-9 or 'x' per request.\r\n");
      tx_msg("--- Input can be entered as a single string to auto-increment/fill each request. E.g. 0011211000510\r\n");
      tx_msg("--- To skip the setup of an individual item, and use the hard-coded values from thereon, enter a value of 'x'.\r\n");
      tx_msg("--- To skip the COM setup at any point, and use the hard-coded values from thereon, enter a value of 's'.\r\n");
      tx_msg("--- To reset the program, and re-prompt for user input, enter a value of 'q'.\r\n");
      tx_msg("--- To pause/resume the program when running, enter a value of 'p'.\r\n");
  }



  static void display_input_prompt(int i)
  {

      switch(i)
      {
        case 0: tx_msg("1. UNUSED Communication Mode: 0=UART, 1=TCI, 2=OneWireUART, 3=SPI ... "); break;
        case 1: tx_msg("2. UNUSED SPI Clock Divider (MHz): 0=16/1, 1=16/2, 2=16/4, 3=16/8, 4=16/16, 5=16/32, 6=16/64, 7=16/128 ... "); break;
        case 2: tx_msg("3. P1 and P2 Thresholds: 0=%25, 1=50%, or 2=75% of max ... "); break;
        case 3: tx_msg("4. Transducer Settings: 0=Murata MA58MF14-7N, 1=Murata MA40H1S-R, 2=PUIAudio UTR-1440K-TT-R ... "); break;
        case 4: tx_msg("5. TVG Range: 0=32-64dB, 1=46-78dB, 2=52-84dB, or 3=58-90dB ... "); break;
        case 5: tx_msg("6. Fixed TVG Level: 0=%25, 1=50%, or 2=75% of max ... "); break;
        case 6: tx_msg("7. Minimum Distance = 0.1m * BYTE ... "); break;
        case 7: tx_msg("8. Run System Diagnostics?: 0=No, 1=Yes ... "); break;
        case 8: tx_msg("9. Echo Data Dump: 0=None, 1=P1BL, 2=P2BL, 3=P1LO, 4=P2LO,... "); break;
        case 9: tx_msg("10. Burn/Save User EEPROM?: 0=No, 1=Yes ... "); break;
        case 10: tx_msg("11. Command Cycle Delay: 100ms * BYTE ... "); break;
        case 11: tx_msg("12. Number of Objects to Detect (1-8) = BYTE ... "); break;
        case 12: tx_msg("13. USART Address of PGA460 (0-7) = BYTE ... "); break;
      }

  }



  static int8_t serial_available(uint8_t *inByte)
  {
	  uint8_t ch;

	  while (uart1_inbuf_tail != uart1_inbuf_head)
	  {
		  // Extract the next character to be processed
		  ch = uart1_inbuf[uart1_inbuf_tail];

		  // Advance to next character in buffer (if any)
		  uart1_inbuf_tail++;

		  // Wrap (circular buffer)
		  if (uart1_inbuf_tail >= sizeof(uart1_inbuf))
			  uart1_inbuf_tail = 0;

		  // Return the extracted character
		  *inByte = ch;
		  return 1;
	  }

	  return 0;  // no characters have been received yet
  }



  static uint8_t wait_for_valid_input(void)
  {
	  // only accept input as valid if 0-9, q, s, or x; otherwise, wait until valid input
	  uint8_t validInput = 0;
	  uint8_t inByte;
	  char msg[10];

	  while (validInput == 0)
	  {
		  while (serial_available(&inByte) == 0) {};

		  if (inByte==48 || inByte==49 || inByte==50 || inByte==51 ||
			  inByte==52 || inByte==53 || inByte==54 || inByte==55 ||
			  inByte==56 || inByte==57 || inByte==113 || inByte==115 || inByte==120)
		  {
			  validInput = 1; // valid input, break while loop
		  }
		  else
		  {
			  // not a valid value
			  // echo the character typed back to the user
			  sprintf(msg,"%c?\r\n", inByte);
		      tx_msg(msg);
		  }
	  }

	  // echo the character typed back to the user
	  sprintf(msg,"%c\r\n", inByte);
      tx_msg(msg);

	  return inByte;
  }



  static void process_input_byte(int i, uint8_t inByte)
  {
      switch(i)
      {
        case 0: commMode = inByte; break;  // zg - this is unused
        case 1:

            // zg - note this is unused
            switch(inByte)
            {
              case 0: baudRate=1; break;
              case 1: baudRate=2; break;
              case 2: baudRate=4; break;
              case 3: baudRate=8; break;
              case 4: baudRate=16; break;
              case 5: baudRate=32; break;
              case 6: baudRate=64; break;
              case 7: baudRate=128; break;
              default: baudRate=16; break;
            }


          break;
        case 2: fixedThr = inByte; break;
        case 3: xdcr = inByte; break;
        case 4: agrTVG = inByte; break;
        case 5: fixedTVG = inByte; break;
        case 6: minDistLim = inByte * 0.1; break;
        case 7: runDiag = inByte; break;
        case 8: edd = inByte; break;
        case 9: burn = inByte; break;
        case 10: cdMultiplier = inByte; break;
        case 11: numOfObj = inByte; break;
        case 12: uartAddrUpdate = inByte; break;
        default: break;
      }
  }



  static void display_diagnostic(char *Tag, double Value)
  {
	  char msg[100];

	  sprintf(msg, "%s %g\r\n", Tag, Value);
	  tx_msg(msg);
  }



  static void configure_pga460(void)
  {

      tx_msg("Configuring the PGA460 with the selected settings. Wait...\r\n");
      HAL_Delay(300);



  /*------------------------------------------------- userInput & standAlone mode initialization -----
    Configure the EVM in the following order:
    1) Select PGA460 interface, device baud, and COM terminal baud up to 115.2k for targeted address.
    2) Bulk write all threshold values to clear the THR_CRC_ERR.
    3) Bulk write user EEPROM with pre-define values in PGA460_USSC.c.
    4) Update analog front end gain range, and bulk write TVG.
    5) Run system diagnostics for frequency, decay, temperature, and noise measurements
    6) Program (burn) EEPROM memory to save user EEPROM values
    7) Run a preset 1 or 2 burst and/or listen command to capture the echo data dump

    if the input is 'x' (72d), then skip that configuration
  *-------------------------------------------------------------------*/
    // -+-+-+-+-+-+-+-+-+-+- 1 : interface setup   -+-+-+-+-+-+-+-+-+-+- //
      pga460_initBoostXLPGA460(commMode, baudRate, uartAddrUpdate);

    // -+-+-+-+-+-+-+-+-+-+- 2 : bulk threshold write   -+-+-+-+-+-+-+-+-+-+- //
      if (fixedThr != 72){pga460_initThresholds(fixedThr);}
    // -+-+-+-+-+-+-+-+-+-+- 3 : bulk user EEPROM write   -+-+-+-+-+-+-+-+-+-+- //
      if (xdcr != 72){pga460_defaultPGA460(xdcr);}
    // -+-+-+-+-+-+-+-+-+-+- 4 : bulk TVG write   -+-+-+-+-+-+-+-+-+-+- //
      if (agrTVG != 72 && fixedTVG != 72){pga460_initTVG(agrTVG,fixedTVG);}
    // -+-+-+-+-+-+-+-+-+-+- 5 : run system diagnostics   -+-+-+-+-+-+-+-+-+-+- //
      if (runDiag == 1)
      {

        diagnostics = pga460_runDiagnostics(1,0);       // run and capture system diagnostics, and print freq diag result
        display_diagnostic("System Diagnostics - Frequency (kHz):", diagnostics);

        diagnostics = pga460_runDiagnostics(0,1);       // do not re-run system diagnostic, but print decay diag result
        display_diagnostic("System Diagnostics - Decay Period (us):", diagnostics);

        diagnostics = pga460_runDiagnostics(0,2);       // do not re-run system diagnostic, but print temperature measurement
        display_diagnostic("System Diagnostics - Die Temperature (C):", diagnostics);

        diagnostics = pga460_runDiagnostics(0,3);       // do not re-run system diagnostic, but print noise level measurement
        display_diagnostic("System Diagnostics - Noise Level:", diagnostics);
      }
    // -+-+-+-+-+-+-+-+-+-+- 6 : burn EEPROM   -+-+-+-+-+-+-+-+-+-+- //
      if(burn == 1)
      {
        uint8_t burnStat = pga460_burnEEPROM();
        if (burnStat) {tx_msg("EEPROM programmed successfully.\r\n");}
        else{tx_msg("EEPROM program failed.");}
      }
    // -+-+-+-+-+-+-+-+-+-+- 7 : capture echo data dump   -+-+-+-+-+-+-+-+-+-+- //
      if (edd != 0)                                   // run or skip echo data dump
      {
        tx_msg("Retrieving echo data dump profile. Wait...\r\n");
        pga460_runEchoDataDump(edd-1);                  // run preset 1 or 2 burst and/or listen command
        //Serial.print(pga460_pullEchoDataDumpBulk());
        tx_msg("");
      }
    // -+-+-+-+-+-+-+-+-+-+-  others   -+-+-+-+-+-+-+-+-+-+- //
    commandDelay = 100 * cdMultiplier;                   // command cycle delay result in ms
    if (numOfObj == 0 || numOfObj >8) { numOfObj = 1; } // sets number of objects to detect to 1 if invalid input

  }


  /*------------------------------------------------- initPGA460 -----
  |  function initPGA460
  |
  |  Purpose: One-time setup of PGA460-Q1 EVM hardware and software
  |      in the following steps:
  |    1) Configure the master to operate in UART, TCI, or OWU
  |      communication mode.
  |    2) Confgiure the EVM for compatiblity based on the selected
  |      communicaton mode.
  |    3) Option to update user EEPROM and threhsold registers with
  |      pre-defined values.
  |    4) Option to burn the EEPROM settings (not required unless
  |      values are to be preserved after power cycling device).
  |    5) Option to report echo data dump and/or system diagnostics.
  |
  |  In userInput mode, the user is prompted to enter values through
  |   the Serial COM terminal to configure the device.
  |
  |  In standAlone mode, the user must hard-code the configuration
  |   variables in the globals section for the device to
  |   auto-configure in the background.
  *-------------------------------------------------------------------*/

 static void initPGA460()
 {
      int inByte = 0;         // incoming serial uint8_t

      restart:

      display_header();

      int numInputs = 13;
      for (int i=0; i<numInputs; i++)
      {
    	  display_input_prompt(i);

    	  inByte = wait_for_valid_input();


        //subtract 48d since ASCII '0' is 48d as a printable character
        inByte = inByte - 48;


        if (inByte != 115-48 && inByte != 113-48) // if input is neither 's' or 'q'
        {
          HAL_Delay(300);

          //tx_msg(inByte);

          process_input_byte(i, inByte);

        }
        else if (inByte == 113-48) //  'q'
        {
          goto restart; // restart initialization routine
        }
        else //   's'
        {
          i = numInputs-1; // force for-loop to exit
          tx_msg("\r\n");
        }
      }

      configure_pga460();

  }
#endif


/*------------------------------------------------------
|  get_distance
|
|   The PGA460 is initiated with a Preset 1 Burst-and-Listen
|     Time-of-Flight measurement. Preset 1 is ideally configured for
|     short-range measurements (sub-1m range) when using the pre-defined
|     user EEPROM configurations.
|
|   If no object is detected, the PGA460 will then be issued a
|     Preset 2 Burst-and-Listen Time-of-Flight measurement.
|     Preset 2 is configured for long-range measurements (beyond
|     1m range).
|
|   Depending on the resulting distance, the diagnostics LEDs will
|     illuminate to represent a short, mid, or long range value.
|
|   In userInput mode, the distance, width, and/or amplitude value
|     of each object is serial printed on the COM terminal.
|
|   In standAlone mode, only distance can be represented visually
|     on the LEDs. The resulting values are still serial printed
|     on a COM terminal for debug, and to view the numerical values
|     of the data captured.
|
*-------------------------------------------------------------------*/
static void report_results(char *Tag, uint8_t ObjNum, double Distance, double Width, double Peak)
{
    char msg[100];

	int dec1, dec2, dec3, dec4, dec5, dec6 = 0;

	convfloatstr_2dec((float)Distance, &dec1, &dec2);
	convfloatstr_2dec((float)Width, &dec3, &dec4);
	convfloatstr_2dec((float)Peak, &dec5, &dec6);

    sprintf(msg, "%s Obj %d Distance (in): %d.%02d  Width (us): %d.%02d  Amplitude (dec): %d.%02d\n", Tag, ObjNum, dec1, dec2, dec3, dec4, dec5, dec6);

    pga460_log_msg(msg);
}






static double pga460_get_distance(int ReportResults)
{

    objectDetected = PGA460_FALSE;          // Initialize object detected flag to PGA460_FALSE
#if 0
    // -+-+-+-+-+-+-+-+-+-+-  PRESET 1 (SHORT RANGE) MEASUREMENT   -+-+-+-+-+-+-+-+-+-+- //
      // For safety, give the main capacitor time to charge - unclear if this is needed
  	  //ltc2943_charge_main_capacitor(ONE_TAU);  // takes 220 ms

      pga460_registerWrite(64, 0);            // EVM sends this each time
      pga460_ultrasonicCmd(0,numOfObj);       // run preset 1 (short distance) burst+listen for 1 object
      pga460_pullUltrasonicMeasResult();      // Pull Ultrasonic Measurement Result
	  pga460_pullDiagnosticBytes();           // Pull Diagnostic Measurement

	  // print diagnostic info here

      // print echo data here
      for (uint8_t i=0; i < numOfObj; i++)
      {
    	  // Log uUltrasonic Measurement Result: Obj1: 0=Distance(m), 1=Width, 2=Amplitude; Obj2: 3=Distance(m), 4=Width, 5=Amplitude; etc.;
    	  distance = pga460_printUltrasonicMeasResult(0+(i*3));
    	  width    = pga460_printUltrasonicMeasResult(1+(i*3));  // only available for UART, OWU, and SPI
    	  peak     = pga460_printUltrasonicMeasResult(2+(i*3));   // only available for UART, OWU, and SPI

    	  if (ReportResults) report_results("P1", i+1, distance, width, peak);
      }
#endif
    // -+-+-+-+-+-+-+-+-+-+-  PRESET 2 (LONG RANGE) MEASUREMENT   -+-+-+-+-+-+-+-+-+-+- //
      if(objectDetected == PGA460_FALSE || alwaysLong == PGA460_TRUE)                       // If no preset 1 (short distance) measurement result, switch to Preset 2 B+L command
      {
        // For safety, give the main capacitor time to charge - unclear if this is needed or if this is even sufficient
      	//ltc2943_charge_main_capacitor(ONE_TAU);  // takes 220 ms

    	pga460_registerWrite(64, 0);           // EVM sends this each time
        pga460_ultrasonicCmd(1,numOfObj);      // run preset 2 (long distance) burst+listen for 1 object
        pga460_pullUltrasonicMeasResult();     // Get Ultrasonic Measurement Result
        pga460_pullDiagnosticBytes();

  	    // print diagnostic info here

        // print echo data here

        for (uint8_t i=0; i<numOfObj; i++)
        {
          distance2 = pga460_printUltrasonicMeasResult(0+(i*3));   // Print Ultrasonic Measurement Result i.e. Obj1: 0=Distance(m), 1=Width, 2=Amplitude; Obj2: 3=Distance(m), 4=Width, 5=Amplitude;
          width    = pga460_printUltrasonicMeasResult(1+(i*3));    // only available for UART, OWU, and SPI
          peak     = pga460_printUltrasonicMeasResult(2+(i*3));     // only available for UART, OWU, and SPI

    	  if (ReportResults) report_results("P2", i+1, distance2, width, peak);

        }
      }

      //return distance;
      return distance2;
}


void pga460_perform_preset(int preset, PGA460_DATA_POINT *p, float speedSound_inches_per_us)   // 0 = preset1  1 = preset2
{
	// For safety, give the main capacitor time to charge - unclear if this is needed
	//ltc2943_charge_main_capacitor(ONE_TAU);  // takes 220 ms

    trace_SaveBuffer(); // save trace messages to SD card to ensure no write during preset

	numOfObj = PGA460_ECHOS;
	//pga460_registerWrite(64, 0);            // EVM sends this each time - no response expected, flushes in buffer, aborts cmd in progress
	pga460_ultrasonicCmd(preset,numOfObj);  // run preset for numOfObj objects - no response expected, flushes in buffer, aborts cmd in progress, serves new cmd
	pga460_pullUltrasonicMeasResult();      // Pull Ultrasonic Measurement Result - response expected, flushes in buffer, can rcv zero if prev no-response cmd in progress, or prev bytes and new cmd is queued
	pga460_pullDiagnosticBytes();           // Pull Diagnostic Measurement Result - response expected, flushes in buffer, can rcv zero if prev no-response cmd in progress, or prev bytes and new cmd is queued

	// Transfer raw data and results to output object
	pga460_update_data_point(p, speedSound_inches_per_us);

}






#if 0
static void read_all_registers(void)
{
	uint8_t i;
	uint8_t reg;
	char msg[80];

	tx_msg("\r\nreg_addr = reg_value\r\n");

	for (i=0; i < 0x40; i++)
	{

		reg = pga460_registerRead(i);

		  sprintf(msg,"%d (%x) = %x\r\n", i, i, reg);
	      tx_msg(msg);
	}
    tx_msg("\r\n done reporting registers...\r\n");
}
#endif


#if 0
static void read_diagnostic_registers(void)
{
	uint8_t i;
	uint8_t reg;
	char msg[80];

	tx_msg("\r\nreg_addr = reg_value\r\n");

	for (i=76; i < 78; i++)
	{

		reg = pga460_registerRead(i);

		  sprintf(msg,"%d (%x) = %x\r\n", i, i, reg);
	      tx_msg(msg);
	}
    tx_msg("\r\n done reporting registers...\r\n");

}
#endif

#if 0
static void eastech_configure_pga460(void)
{
#if 0
  pga460_initThresholds(3);  // At power on, must clear error by setting thresholds at least once

  pga460_defaultPGA460(xdcr);

  pga460_initTVG(agrTVG,fixedTVG);

#else
  configure_pga460();
#endif
}
#endif

#if 0
static void recorded_test_script_0(void)
{
	// Original test script from settings provided by Mark Watson
	pga460_registerRead(31);
	pga460_registerRead(31);
	pga460_registerRead(0);
	pga460_registerRead(1);
	pga460_registerRead(2);
	pga460_registerRead(3);
	pga460_registerRead(4);
	pga460_registerRead(5);
	pga460_registerRead(6);
	pga460_registerRead(7);
	pga460_registerRead(8);
	pga460_registerRead(9);
	pga460_registerRead(10);
	pga460_registerRead(11);
	pga460_registerRead(12);
	pga460_registerRead(13);
	pga460_registerRead(14);
	pga460_registerRead(15);
	pga460_registerRead(16);
	pga460_registerRead(17);
	pga460_registerRead(18);
	pga460_registerRead(19);
	pga460_registerRead(20);
	pga460_registerRead(21);
	pga460_registerRead(22);
	pga460_registerRead(23);
	pga460_registerRead(24);
	pga460_registerRead(25);
	pga460_registerRead(26);
	pga460_registerRead(27);
	pga460_registerRead(28);
	pga460_registerRead(29);
	pga460_registerRead(30);
	pga460_registerRead(31);
	pga460_registerRead(32);
	pga460_registerRead(33);
	pga460_registerRead(34);
	pga460_registerRead(35);
	pga460_registerRead(36);
	pga460_registerRead(37);
	pga460_registerRead(38);
	pga460_registerRead(39);
	pga460_registerRead(40);
	pga460_registerRead(41);
	pga460_registerRead(42);
	pga460_registerRead(43);
	pga460_registerRead(64);
	pga460_registerRead(65);
	pga460_registerRead(66);
	pga460_registerRead(67);
	pga460_registerRead(68);
	pga460_registerRead(69);
	pga460_registerRead(70);
	pga460_registerRead(71);
	pga460_registerRead(72);
	pga460_registerRead(73);
	pga460_registerRead(74);
	pga460_registerRead(75);
	pga460_registerRead(76);
	pga460_registerRead(77);
	pga460_registerRead(95);
	pga460_registerRead(96);
	pga460_registerRead(97);
	pga460_registerRead(98);
	pga460_registerRead(99);
	pga460_registerRead(100);
	pga460_registerRead(101);
	pga460_registerRead(102);
	pga460_registerRead(103);
	pga460_registerRead(104);
	pga460_registerRead(105);
	pga460_registerRead(106);
	pga460_registerRead(107);
	pga460_registerRead(108);
	pga460_registerRead(109);
	pga460_registerRead(110);
	pga460_registerRead(111);
	pga460_registerRead(112);
	pga460_registerRead(113);
	pga460_registerRead(114);
	pga460_registerRead(115);
	pga460_registerRead(116);
	pga460_registerRead(117);
	pga460_registerRead(118);
	pga460_registerRead(119);
	pga460_registerRead(120);
	pga460_registerRead(121);
	pga460_registerRead(122);
	pga460_registerRead(123);
	pga460_registerRead(124);
	pga460_registerRead(125);
	pga460_registerRead(126);
	pga460_registerRead(127);
	pga460_registerRead(76);
	pga460_registerRead(77);
	pga460_registerRead(31);
	pga460_registerRead(31);
	pga460_registerRead(76);
	pga460_registerRead(77);
	pga460_registerRead(31);
	pga460_registerWrite(110,0);
	pga460_registerWrite(109,128);
	pga460_registerWrite(108,128);
	pga460_registerWrite(107,128);
	pga460_registerWrite(106,128);
	pga460_registerWrite(105,16);
	pga460_registerWrite(104,66);
	pga460_registerWrite(103,8);
	pga460_registerWrite(102,33);
	pga460_registerWrite(101,132);
	pga460_registerWrite(100,136);
	pga460_registerWrite(99,136);
	pga460_registerWrite(98,136);
	pga460_registerWrite(97,136);
	pga460_registerWrite(96,136);
	pga460_registerWrite(95,136);
	pga460_registerRead(76);
	pga460_registerRead(77);
	pga460_registerRead(31);
	pga460_registerWrite(126,0);
	pga460_registerWrite(125,128);
	pga460_registerWrite(124,128);
	pga460_registerWrite(123,128);
	pga460_registerWrite(122,128);
	pga460_registerWrite(121,16);
	pga460_registerWrite(120,66);
	pga460_registerWrite(119,8);
	pga460_registerWrite(118,33);
	pga460_registerWrite(117,132);
	pga460_registerWrite(116,136);
	pga460_registerWrite(115,136);
	pga460_registerWrite(114,136);
	pga460_registerWrite(113,136);
	pga460_registerWrite(112,136);
	pga460_registerWrite(111,136);
	pga460_registerRead(76);
	pga460_registerRead(77);
	pga460_registerRead(31);
	pga460_registerWrite(64,128);
}
#endif

#if 0
static void recorded_test_script_1(void)
{
	// From GRID_USER_MEMSPACE-2023-02-15_201738.txt created at Eastech Flowlab by Mark LaPlante.
	pga460_registerWrite(0,0);
	pga460_registerWrite(1,0);
	pga460_registerWrite(2,0);
	pga460_registerWrite(3,0);
	pga460_registerWrite(4,0);
	pga460_registerWrite(5,0);
	pga460_registerWrite(6,0);
	pga460_registerWrite(7,0);
	pga460_registerWrite(8,0);
	pga460_registerWrite(9,0);
	pga460_registerWrite(10,0);
	pga460_registerWrite(11,0);
	pga460_registerWrite(12,0);
	pga460_registerWrite(13,0);
	pga460_registerWrite(14,0);
	pga460_registerWrite(15,0);
	pga460_registerWrite(16,0);
	pga460_registerWrite(17,0);
	pga460_registerWrite(18,0);
	pga460_registerWrite(19,0);
	pga460_registerWrite(20,175);
	pga460_registerWrite(21,255);
	pga460_registerWrite(22,255);
	pga460_registerWrite(23,45);
	pga460_registerWrite(24,104);
	pga460_registerWrite(25,51);
	pga460_registerWrite(26,252);
	pga460_registerWrite(27,128);
	pga460_registerWrite(28,221);
	pga460_registerWrite(29,208);
	pga460_registerWrite(30,15);
	pga460_registerWrite(31,18);
	pga460_registerWrite(32,127);
	pga460_registerWrite(33,127);
	pga460_registerWrite(34,172);
	pga460_registerWrite(35,19);
	pga460_registerWrite(36,175);
	pga460_registerWrite(37,140);
	pga460_registerWrite(38,87);
	pga460_registerWrite(39,104);
	pga460_registerWrite(40,0);
	pga460_registerWrite(41,0);
	pga460_registerWrite(42,0);
	pga460_registerWrite(43,211);

	pga460_registerWrite(65,143);
	pga460_registerWrite(66,193);
	pga460_registerWrite(67,246);
	pga460_registerWrite(68,135);
	pga460_registerWrite(69,4);
	pga460_registerWrite(70,189);
	pga460_registerWrite(71,126);
	pga460_registerWrite(72,103);
	pga460_registerWrite(73,0);
	pga460_registerWrite(74,205);
	pga460_registerWrite(75,0);

	pga460_registerWrite(95,155);
	pga460_registerWrite(96,188);
	pga460_registerWrite(97,204);
	pga460_registerWrite(98,204);
	pga460_registerWrite(99,204);
	pga460_registerWrite(100,204);
	pga460_registerWrite(101,33);
	pga460_registerWrite(102,8);
	pga460_registerWrite(103,66);
	pga460_registerWrite(104,16);
	pga460_registerWrite(105,133);
	pga460_registerWrite(106,50);
	pga460_registerWrite(107,75);
	pga460_registerWrite(108,150);
	pga460_registerWrite(109,255);
	pga460_registerWrite(110,0);
	pga460_registerWrite(111,155);
	pga460_registerWrite(112,188);
	pga460_registerWrite(113,204);
	pga460_registerWrite(114,204);
	pga460_registerWrite(115,204);
	pga460_registerWrite(116,204);
	pga460_registerWrite(117,33);
	pga460_registerWrite(118,8);
	pga460_registerWrite(119,66);
	pga460_registerWrite(120,16);
	pga460_registerWrite(121,133);
	pga460_registerWrite(122,50);
	pga460_registerWrite(123,75);
	pga460_registerWrite(124,150);
	pga460_registerWrite(125,255);
	pga460_registerWrite(126,0);
	pga460_registerWrite(127,178);

#if 0
	pga460_ultrasonicCmd(0,1);
	pga460_pullEchoDataDump(0);
	pga460_registerWrite(64,0);

	pga460_ultrasonicCmd(0,1);
	pga460_pullUltrasonicMeasResult();
	pga460_registerWrite(64,128);
#endif

}

#endif


#if 0

static void recorded_test_script(void)
{
	// From GRID_USER_MEMSPACE-2023-02-15_201738.txt created at Eastech Flowlab by Mark LaPlante.

#if 0
	pga460_registerRead(31);
	pga460_registerRead(31);
	pga460_registerRead(0);
	pga460_registerRead(1);
	pga460_registerRead(2);
	pga460_registerRead(3);
	pga460_registerRead(4);
	pga460_registerRead(5);
	pga460_registerRead(6);
	pga460_registerRead(7);
	pga460_registerRead(8);
	pga460_registerRead(9);
	pga460_registerRead(10);
	pga460_registerRead(11);
	pga460_registerRead(12);
	pga460_registerRead(13);
	pga460_registerRead(14);
	pga460_registerRead(15);
	pga460_registerRead(16);
	pga460_registerRead(17);
	pga460_registerRead(18);
	pga460_registerRead(19);
	pga460_registerRead(20);
	pga460_registerRead(21);
	pga460_registerRead(22);
	pga460_registerRead(23);
	pga460_registerRead(24);
	pga460_registerRead(25);
	pga460_registerRead(26);
	pga460_registerRead(27);
	pga460_registerRead(28);
	pga460_registerRead(29);
	pga460_registerRead(30);
	pga460_registerRead(31);
	pga460_registerRead(32);
	pga460_registerRead(33);
	pga460_registerRead(34);
	pga460_registerRead(35);
	pga460_registerRead(36);
	pga460_registerRead(37);
	pga460_registerRead(38);
	pga460_registerRead(39);
	pga460_registerRead(40);
	pga460_registerRead(41);
	pga460_registerRead(42);
	pga460_registerRead(43);
	pga460_registerRead(64);
	pga460_registerRead(65);
	pga460_registerRead(66);
	pga460_registerRead(67);
	pga460_registerRead(68);
	pga460_registerRead(69);
	pga460_registerRead(70);
	pga460_registerRead(71);
	pga460_registerRead(72);
	pga460_registerRead(73);
	pga460_registerRead(74);
	pga460_registerRead(75);
	pga460_registerRead(76);
	pga460_registerRead(77);
	pga460_registerRead(95);
	pga460_registerRead(96);
	pga460_registerRead(97);
	pga460_registerRead(98);
	pga460_registerRead(99);
	pga460_registerRead(100);
	pga460_registerRead(101);
	pga460_registerRead(102);
	pga460_registerRead(103);
	pga460_registerRead(104);
	pga460_registerRead(105);
	pga460_registerRead(106);
	pga460_registerRead(107);
	pga460_registerRead(108);
	pga460_registerRead(109);
	pga460_registerRead(110);
	pga460_registerRead(111);
	pga460_registerRead(112);
	pga460_registerRead(113);
	pga460_registerRead(114);
	pga460_registerRead(115);
	pga460_registerRead(116);
	pga460_registerRead(117);
	pga460_registerRead(118);
	pga460_registerRead(119);
	pga460_registerRead(120);
	pga460_registerRead(121);
	pga460_registerRead(122);
	pga460_registerRead(123);
	pga460_registerRead(124);
	pga460_registerRead(125);
	pga460_registerRead(126);
	pga460_registerRead(127);
	pga460_registerRead(27);
	pga460_registerWrite(27,192);
	pga460_registerRead(28);
	pga460_registerWrite(28,140);
	pga460_registerRead(29);
	pga460_registerWrite(29,0);
	pga460_registerRead(30);
	pga460_registerWrite(30,1);
	pga460_registerRead(32);
	pga460_registerWrite(32,71);
	pga460_registerRead(33);
	pga460_registerWrite(33,255);
	pga460_registerRead(34);
	pga460_registerWrite(34,28);
	pga460_registerRead(35);
	pga460_registerWrite(35,0);
	pga460_registerRead(35);
	pga460_registerWrite(35,0);
	pga460_registerRead(36);
	pga460_registerWrite(36,238);
	pga460_registerRead(36);
	pga460_registerWrite(36,238);
	pga460_registerRead(37);
	pga460_registerWrite(37,124);
	pga460_registerRead(37);
	pga460_registerWrite(37,124);
	pga460_registerRead(37);
	pga460_registerWrite(37,124);
	pga460_registerRead(38);
	pga460_registerWrite(38,10);
	pga460_registerRead(27);
	pga460_registerWrite(27,192);
	pga460_registerRead(38);
	pga460_registerWrite(38,10);
	pga460_registerRead(38);
	pga460_registerWrite(38,10);
	pga460_registerRead(38);
	pga460_registerWrite(38,10);
	pga460_registerRead(39);
	pga460_registerWrite(39,0);
	pga460_registerRead(76);
	pga460_registerRead(77);
	pga460_registerRead(31);
#endif

	pga460_registerWrite(0,0);
	pga460_registerWrite(1,0);
	pga460_registerWrite(2,0);
	pga460_registerWrite(3,0);
	pga460_registerWrite(4,0);
	pga460_registerWrite(5,0);
	pga460_registerWrite(6,0);
	pga460_registerWrite(7,0);
	pga460_registerWrite(8,0);
	pga460_registerWrite(9,0);
	pga460_registerWrite(10,0);
	pga460_registerWrite(11,0);
	pga460_registerWrite(12,0);
	pga460_registerWrite(13,0);
	pga460_registerWrite(14,0);
	pga460_registerWrite(15,0);
	pga460_registerWrite(16,0);
	pga460_registerWrite(17,0);
	pga460_registerWrite(18,0);
	pga460_registerWrite(19,0);
	pga460_registerWrite(20,175);
	pga460_registerWrite(21,255);
	pga460_registerWrite(22,255);
	pga460_registerWrite(23,45);
	pga460_registerWrite(24,104);
	pga460_registerWrite(25,51);
	pga460_registerWrite(26,252);
	pga460_registerWrite(27,128);
	pga460_registerWrite(28,221);
	pga460_registerWrite(29,208);
	pga460_registerWrite(30,15);
	pga460_registerWrite(31,18);
	pga460_registerWrite(32,127);
	pga460_registerWrite(33,127);
	pga460_registerWrite(34,172);
	pga460_registerWrite(35,19);
	pga460_registerWrite(36,175);
	pga460_registerWrite(37,140);
	pga460_registerWrite(38,87);
	pga460_registerWrite(39,104);
	pga460_registerWrite(40,0);
	pga460_registerWrite(41,0);
	pga460_registerWrite(42,0);
	pga460_registerWrite(43,211);
	pga460_registerWrite(65,143);
	pga460_registerWrite(66,193);
	pga460_registerWrite(67,246);
	pga460_registerWrite(68,135);
	pga460_registerWrite(69,4);
	pga460_registerWrite(70,189);
	pga460_registerWrite(71,126);
	pga460_registerWrite(72,103);
	pga460_registerWrite(73,0);
	pga460_registerWrite(74,205);
	pga460_registerWrite(75,0);
	pga460_registerWrite(95,155);
	pga460_registerWrite(96,188);
	pga460_registerWrite(97,204);
	pga460_registerWrite(98,204);
	pga460_registerWrite(99,204);
	pga460_registerWrite(100,204);
	pga460_registerWrite(101,33);
	pga460_registerWrite(102,8);
	pga460_registerWrite(103,66);
	pga460_registerWrite(104,16);
	pga460_registerWrite(105,133);
	pga460_registerWrite(106,50);
	pga460_registerWrite(107,75);
	pga460_registerWrite(108,150);
	pga460_registerWrite(109,255);
	pga460_registerWrite(110,0);
	pga460_registerWrite(111,155);
	pga460_registerWrite(112,188);
	pga460_registerWrite(113,204);
	pga460_registerWrite(114,204);
	pga460_registerWrite(115,204);
	pga460_registerWrite(116,204);
	pga460_registerWrite(117,33);
	pga460_registerWrite(118,8);
	pga460_registerWrite(119,66);
	pga460_registerWrite(120,16);
	pga460_registerWrite(121,133);
	pga460_registerWrite(122,50);
	pga460_registerWrite(123,75);
	pga460_registerWrite(124,150);
	pga460_registerWrite(125,255);
	pga460_registerWrite(126,0);
	pga460_registerWrite(127,178);

	pga460_registerRead(27);
	pga460_registerWrite(27,128);
	pga460_registerRead(28);
	pga460_registerWrite(28,221);
	pga460_registerRead(29);
	pga460_registerWrite(29,208);
	pga460_registerRead(30);
	pga460_registerWrite(30,15);
	pga460_registerRead(32);
	pga460_registerWrite(32,127);
	pga460_registerRead(33);
	pga460_registerWrite(33,127);
	pga460_registerRead(34);
	pga460_registerWrite(34,172);
	pga460_registerRead(35);
	pga460_registerWrite(35,19);
	pga460_registerRead(35);
	pga460_registerWrite(35,19);
	pga460_registerRead(36);
	pga460_registerWrite(36,175);
	pga460_registerRead(36);
	pga460_registerWrite(36,175);
	pga460_registerRead(37);
	pga460_registerWrite(37,140);
	pga460_registerRead(37);
	pga460_registerWrite(37,140);
	pga460_registerRead(37);
	pga460_registerWrite(37,140);
	pga460_registerRead(38);
	pga460_registerWrite(38,87);
	pga460_registerRead(27);
	pga460_registerWrite(27,128);
	pga460_registerRead(38);
	pga460_registerWrite(38,87);
	pga460_registerRead(38);
	pga460_registerWrite(38,87);
	pga460_registerRead(38);
	pga460_registerWrite(38,87);
	pga460_registerRead(39);
	pga460_registerWrite(39,104);
	pga460_registerRead(76);
	pga460_registerRead(77);
	pga460_registerRead(31);

#if 0
	pga460_registerWrite(64,128);   // Enable Data Dump
	pga460_ultrasonicCmd(0,1);
	pga460_pullEchoDataDump(0);

	pga460_registerWrite(64,0);     // Disable Data Dump
	pga460_ultrasonicCmd(0,1);
	pga460_pullUltrasonicMeasResult();

	//while (1) ;
#endif

}

#endif


#if 0

static void recorded_test_script(void)
{

	// 2023-03-09 captured from Zack's benchtop unit

	pga460_registerRead(0x1F);
	//[40,0x12,0xAD,0x
	pga460_registerRead(0x1F);
	//[40,0x12,0xAD,0x
	pga460_registerRead(0x00);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x01);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x02);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x03);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x04);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x05);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x06);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x07);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x08);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x09);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x0A);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x0B);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x0C);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x0D);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x0E);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x0F);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x10);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x11);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x12);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x13);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x14);
	//[40,0xAF,0x10,0x
	pga460_registerRead(0x15);
	//[40,0xFF,0xBF,0x
	pga460_registerRead(0x16);
	//[40,0xFF,0xBF,0x
	pga460_registerRead(0x17);
	//[40,0x2D,0x92,0x
	pga460_registerRead(0x18);
	//[40,0x68,0x57,0x
	pga460_registerRead(0x19);
	//[40,0x36,0x89,0x
	pga460_registerRead(0x1A);
	//[40,0xFC,0xC2,0x
	pga460_registerRead(0x1B);
	//[40,0xC0,0xFE,0x
	pga460_registerRead(0x1C);
	//[40,0x8C,0x33,0x
	pga460_registerRead(0x1D);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x1E);
	//[40,0x01,0xBE,0x
	pga460_registerRead(0x1F);
	//[40,0x12,0xAD,0x
	pga460_registerRead(0x20);
	//[40,0x47,0x78,0x
	pga460_registerRead(0x21);
	//[40,0xFF,0xBF,0x
	pga460_registerRead(0x22);
	//[40,0x1C,0xA3,0x
	pga460_registerRead(0x23);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x24);
	//[40,0xEE,0xD0,0x
	pga460_registerRead(0x25);
	//[40,0x7C,0x43,0x
	pga460_registerRead(0x26);
	//[40,0x0A,0xB5,0x
	pga460_registerRead(0x27);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x28);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x29);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x2A);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x2B);
	//[40,0x9D,0x22,0x
	pga460_registerRead(0x40);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x41);
	//[40,0x8B,0x34,0x
	pga460_registerRead(0x42);
	//[40,0x4D,0x72,0x
	pga460_registerRead(0x43);
	//[40,0xF3,0xCB,0x
	pga460_registerRead(0x44);
	//[40,0x72,0x4D,0x
	pga460_registerRead(0x45);
	//[40,0x06,0xB9,0x
	pga460_registerRead(0x46);
	//[40,0x47,0x78,0x
	pga460_registerRead(0x47);
	//[40,0x7C,0x43,0x
	pga460_registerRead(0x48);
	//[40,0xD3,0xEB,0x
	pga460_registerRead(0x49);
	//[40,0x01,0xBE,0x
	pga460_registerRead(0x4A);
	//[40,0x97,0x28,0x
	pga460_registerRead(0x4B);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x4C);
	//[40,0x84,0x3B,0x
	pga460_registerRead(0x4D);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x5F);
	//[40,0xD4,0xEA,0x
	pga460_registerRead(0x60);
	//[40,0x96,0x29,0x
	pga460_registerRead(0x61);
	//[40,0x24,0x9B,0x
	pga460_registerRead(0x62);
	//[40,0xB6,0x09,0x
	pga460_registerRead(0x63);
	//[40,0x6F,0x50,0x
	pga460_registerRead(0x64);
	//[40,0x28,0x97,0x
	pga460_registerRead(0x65);
	//[40,0x4A,0x75,0x
	pga460_registerRead(0x66);
	//[40,0xC8,0xF6,0x
	pga460_registerRead(0x67);
	//[40,0x50,0x6F,0x
	pga460_registerRead(0x68);
	//[40,0x19,0xA6,0x
	pga460_registerRead(0x69);
	//[40,0x1A,0xA5,0x
	pga460_registerRead(0x6A);
	//[40,0xA3,0x1C,0x
	pga460_registerRead(0x6B);
	//[40,0xA9,0x16,0x
	pga460_registerRead(0x6C);
	//[40,0x44,0x7B,0x
	pga460_registerRead(0x6D);
	//[40,0x71,0x4E,0x
	pga460_registerRead(0x6E);
	//[40,0x63,0x5C,0x
	pga460_registerRead(0x6F);
	//[40,0xB1,0x0E,0x
	pga460_registerRead(0x70);
	//[40,0x4E,0x71,0x
	pga460_registerRead(0x71);
	//[40,0xA1,0x1E,0x
	pga460_registerRead(0x72);
	//[40,0xAA,0x15,0x
	pga460_registerRead(0x73);
	//[40,0x70,0x4F,0x
	pga460_registerRead(0x74);
	//[40,0x08,0xB7,0x
	pga460_registerRead(0x75);
	//[40,0x84,0x3B,0x
	pga460_registerRead(0x76);
	//[40,0xD0,0xEE,0x
	pga460_registerRead(0x77);
	//[40,0x5A,0x65,0x
	pga460_registerRead(0x78);
	//[40,0x88,0x37,0x
	pga460_registerRead(0x79);
	//[40,0x1C,0xA3,0x
	pga460_registerRead(0x7A);
	//[40,0xEE,0xD0,0x
	pga460_registerRead(0x7B);
	//[40,0xFE,0xC0,0x
	pga460_registerRead(0x7C);
	//[40,0x42,0x7D,0x
	pga460_registerRead(0x7D);
	//[40,0x10,0xAF,0x
	pga460_registerRead(0x7E);
	//[40,0x49,0x76,0x
	pga460_registerRead(0x7F);
	//[40,0x34,0x8B,0x
	pga460_registerRead(0x1B);
	//[40,0xC0,0xFE,0x
	pga460_registerWrite(0x1B,0xC0);
	pga460_registerRead(0x1C);
	//[40,0x8C,0x33,0x
	pga460_registerWrite(0x1C,0x8C);
	pga460_registerRead(0x1D);
	//[40,0x00,0xBF,0x
	pga460_registerWrite(0x1D,0x00);
	pga460_registerRead(0x1E);
	//[40,0x01,0xBE,0x
	pga460_registerWrite(0x1E,0x01);
	pga460_registerRead(0x20);
	//[40,0x47,0x78,0x
	pga460_registerWrite(0x20,0x47);
	pga460_registerRead(0x21);
	//[40,0xFF,0xBF,0x
	pga460_registerWrite(0x21,0xFF);
	pga460_registerRead(0x22);
	//[40,0x1C,0xA3,0x
	pga460_registerWrite(0x22,0x1C);
	pga460_registerRead(0x23);
	//[40,0x00,0xBF,0x
	pga460_registerWrite(0x23,0x00);
	pga460_registerRead(0x23);
	//[40,0x00,0xBF,0x
	pga460_registerWrite(0x23,0x00);
	pga460_registerRead(0x24);
	//[40,0xEE,0xD0,0x
	pga460_registerWrite(0x24,0xEE);
	pga460_registerRead(0x24);
	//[40,0xEE,0xD0,0x
	pga460_registerWrite(0x24,0xEE);
	pga460_registerRead(0x25);
	//[40,0x7C,0x43,0x
	pga460_registerWrite(0x25,0x7C);
	pga460_registerRead(0x25);
	//[40,0x7C,0x43,0x
	pga460_registerWrite(0x25,0x7C);
	pga460_registerRead(0x25);
	//[40,0x7C,0x43,0x
	pga460_registerWrite(0x25,0x7C);
	pga460_registerRead(0x26);
	//[40,0x0A,0xB5,0x
	pga460_registerWrite(0x26,0x0A);
	pga460_registerRead(0x1B);
	//[40,0xC0,0xFE,0x
	pga460_registerWrite(0x1B,0xC0);
	pga460_registerRead(0x26);
	//[40,0x0A,0xB5,0x
	pga460_registerWrite(0x26,0x0A);
	pga460_registerRead(0x26);
	//[40,0x0A,0xB5,0x
	pga460_registerWrite(0x26,0x0A);
	pga460_registerRead(0x26);
	//[40,0x0A,0xB5,0x
	pga460_registerWrite(0x26,0x0A);
	pga460_registerRead(0x27);
	//[40,0x00,0xBF,0x
	pga460_registerWrite(0x27,0x00);
	pga460_registerRead(0x4C);
	//[40,0x84,0x3B,0x
	pga460_registerRead(0x4D);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x1F);
	//[40,0x12,0xAD,0x


	pga460_registerWrite(0x00,0x00);
	pga460_registerWrite(0x01,0x00);
	pga460_registerWrite(0x02,0x00);
	pga460_registerWrite(0x03,0x00);
	pga460_registerWrite(0x04,0x00);
	pga460_registerWrite(0x05,0x00);
	pga460_registerWrite(0x06,0x00);
	pga460_registerWrite(0x07,0x00);
	pga460_registerWrite(0x08,0x00);
	pga460_registerWrite(0x09,0x00);
	pga460_registerWrite(0x0A,0x00);
	pga460_registerWrite(0x0B,0x00);
	pga460_registerWrite(0x0C,0x00);
	pga460_registerWrite(0x0D,0x00);
	pga460_registerWrite(0x0E,0x00);
	pga460_registerWrite(0x0F,0x00);
	pga460_registerWrite(0x10,0x00);
	pga460_registerWrite(0x11,0x00);
	pga460_registerWrite(0x12,0x00);
	pga460_registerWrite(0x13,0x00);
	pga460_registerWrite(0x14,0xAF);
	pga460_registerWrite(0x15,0xFF);
	pga460_registerWrite(0x16,0xFF);
	pga460_registerWrite(0x17,0x2D);
	pga460_registerWrite(0x18,0x68);
	pga460_registerWrite(0x19,0x33);
	pga460_registerWrite(0x1A,0xFC);
	pga460_registerWrite(0x1B,0x80);
	pga460_registerWrite(0x1C,0xDD);
	pga460_registerWrite(0x1D,0xD0);
	pga460_registerWrite(0x1E,0x0F);
	pga460_registerWrite(0x1F,0x12);
	pga460_registerWrite(0x20,0x7F);
	pga460_registerWrite(0x21,0x7F);
	pga460_registerWrite(0x22,0xAC);
	pga460_registerWrite(0x23,0x13);
	pga460_registerWrite(0x24,0xAF);
	pga460_registerWrite(0x25,0x8C);
	pga460_registerWrite(0x26,0x57);
	pga460_registerWrite(0x27,0x68);
	pga460_registerWrite(0x28,0x00);
	pga460_registerWrite(0x29,0x00);
	pga460_registerWrite(0x2A,0x00);
	pga460_registerWrite(0x2B,0xD3);
	pga460_registerWrite(0x41,0x8F);
	pga460_registerWrite(0x42,0xC1);
	pga460_registerWrite(0x43,0xF6);
	pga460_registerWrite(0x44,0x87);
	pga460_registerWrite(0x45,0x04);
	pga460_registerWrite(0x46,0xBD);
	pga460_registerWrite(0x47,0x7E);
	pga460_registerWrite(0x48,0x67);
	pga460_registerWrite(0x49,0x00);
	pga460_registerWrite(0x4A,0xCD);
	pga460_registerWrite(0x4B,0x00);
	pga460_registerWrite(0x5F,0x9B);
	pga460_registerWrite(0x60,0xBC);
	pga460_registerWrite(0x61,0xCC);
	pga460_registerWrite(0x62,0xCC);
	pga460_registerWrite(0x63,0xCC);
	pga460_registerWrite(0x64,0xCC);
	pga460_registerWrite(0x65,0x21);
	pga460_registerWrite(0x66,0x08);
	pga460_registerWrite(0x67,0x42);
	pga460_registerWrite(0x68,0x10);
	pga460_registerWrite(0x69,0x85);
	pga460_registerWrite(0x6A,0x32);
	pga460_registerWrite(0x6B,0x4B);
	pga460_registerWrite(0x6C,0x96);
	pga460_registerWrite(0x6D,0xFF);
	pga460_registerWrite(0x6E,0x00);
	pga460_registerWrite(0x6F,0x9B);
	pga460_registerWrite(0x70,0xBC);
	pga460_registerWrite(0x71,0xCC);
	pga460_registerWrite(0x72,0xCC);
	pga460_registerWrite(0x73,0xCC);
	pga460_registerWrite(0x74,0xCC);
	pga460_registerWrite(0x75,0x21);
	pga460_registerWrite(0x76,0x08);
	pga460_registerWrite(0x77,0x42);
	pga460_registerWrite(0x78,0x10);
	pga460_registerWrite(0x79,0x85);
	pga460_registerWrite(0x7A,0x32);
	pga460_registerWrite(0x7B,0x4B);
	pga460_registerWrite(0x7C,0x96);
	pga460_registerWrite(0x7D,0xFF);
	pga460_registerWrite(0x7E,0x00);
	pga460_registerWrite(0x7F,0xB2);


	pga460_registerRead(0x1B);
	//[40,0x80,0x3F,0x
	pga460_registerWrite(0x1B,0x80);
	pga460_registerRead(0x1C);
	//[40,0xDD,0xE1,0x
	pga460_registerWrite(0x1C,0xDD);
	pga460_registerRead(0x1D);
	//[40,0xD0,0xEE,0x
	pga460_registerWrite(0x1D,0xD0);
	pga460_registerRead(0x1E);
	//[40,0x0F,0xB0,0x
	pga460_registerWrite(0x1E,0x0F);
	pga460_registerRead(0x20);
	//[40,0x7F,0x
	pga460_registerWrite(0x20,0x7F);
	pga460_registerRead(0x21);
	//[40,0x7F,0x
	pga460_registerWrite(0x21,0x7F);
	pga460_registerRead(0x22);
	//[40,0xAC,0x13,0x
	pga460_registerWrite(0x22,0xAC);
	pga460_registerRead(0x23);
	//[40,0x13,0xAC,0x
	pga460_registerWrite(0x23,0x13);
	pga460_registerRead(0x23);
	//[40,0x13,0xAC,0x
	pga460_registerWrite(0x23,0x13);
	pga460_registerRead(0x24);
	//[40,0xAF,0x10,0x
	pga460_registerWrite(0x24,0xAF);
	pga460_registerRead(0x24);
	//[40,0xAF,0x10,0x
	pga460_registerWrite(0x24,0xAF);
	pga460_registerRead(0x25);
	//[40,0x8C,0x33,0x
	pga460_registerWrite(0x25,0x8C);
	pga460_registerRead(0x25);
	//[40,0x8C,0x33,0x
	pga460_registerWrite(0x25,0x8C);
	pga460_registerRead(0x25);
	//[40,0x8C,0x33,0x
	pga460_registerWrite(0x25,0x8C);
	pga460_registerRead(0x26);
	//[40,0x57,0x68,0x
	pga460_registerWrite(0x26,0x57);
	pga460_registerRead(0x1B);
	//[40,0x80,0x3F,0x
	pga460_registerWrite(0x1B,0x80);
	pga460_registerRead(0x26);
	//[40,0x57,0x68,0x
	pga460_registerWrite(0x26,0x57);
	pga460_registerRead(0x26);
	//[40,0x57,0x68,0x
	pga460_registerWrite(0x26,0x57);
	pga460_registerRead(0x26);
	//[40,0x57,0x68,0x
	pga460_registerWrite(0x26,0x57);
	pga460_registerRead(0x27);
	//[40,0x68,0x57,0x
	pga460_registerWrite(0x27,0x68);
	pga460_registerRead(0x4C);
	//[40,0x80,0x3F,0x
	pga460_registerRead(0x4D);
	//[40,0x00,0xBF,0x
	pga460_registerRead(0x1F);
	//[40,0x12,0xAD,0x

	pga460_registerWrite(0x40,0x80);
	pga460_ultrasonicCmd(0x0,0x01);
	pga460_pullEchoDataDump(0x0);
	//[40,0xFF,0xFF,0xFF,0x7F,0x05,0x03,0x05,0x03,0x17,0x12,0x03,0x02,0x01,0xB3,0xDF,0x37,0x05,0x ...

	pga460_registerWrite(0x40,0x00);
	pga460_ultrasonicCmd(0x0,0x01);
	pga460_pullUltrasonicMeasResult();
	//[40,0x12,0x47,0x8C,0xDF,0xF9,0x


}

#endif

#if 0
static void recorded_test_script(void)
{
	// Recorded test script on Zack's bench 2023-03-09  0.867 m = 34.134 in
	//Power on. Running 5.0.135 from Bank 1, MCUID = 1121, Gen5, BoardId = 3


	pga460_registerRead( 77);

	pga460_registerWrite( 27,192);
	pga460_registerWrite( 28,140);
	pga460_registerWrite( 29,  0);
	pga460_registerWrite( 30,  1);
	pga460_registerWrite( 32, 71);
	pga460_registerWrite( 33,192);
	pga460_registerWrite( 34, 28);
	pga460_registerWrite( 35,  0);
	pga460_registerWrite( 36,238);
	pga460_registerWrite( 37,124);
	pga460_registerWrite( 38, 10);
	pga460_registerWrite( 39,  0);

	pga460_registerRead( 77);

	pga460_registerWrite(  0,  0);
	pga460_registerWrite(  1,  0);
	pga460_registerWrite(  2,  0);
	pga460_registerWrite(  3,  0);
	pga460_registerWrite(  4,  0);
	pga460_registerWrite(  5,  0);
	pga460_registerWrite(  6,  0);
	pga460_registerWrite(  7,  0);
	pga460_registerWrite(  8,  0);
	pga460_registerWrite(  9,  0);
	pga460_registerWrite( 10,  0);
	pga460_registerWrite( 11,  0);
	pga460_registerWrite( 12,  0);
	pga460_registerWrite( 13,  0);
	pga460_registerWrite( 14,  0);
	pga460_registerWrite( 15,  0);
	pga460_registerWrite( 16,  0);
	pga460_registerWrite( 17,  0);
	pga460_registerWrite( 18,  0);
	pga460_registerWrite( 19,  0);
	pga460_registerWrite( 20,175);
	pga460_registerWrite( 21,255);
	pga460_registerWrite( 22,255);
	pga460_registerWrite( 23, 45);
	pga460_registerWrite( 24,104);
	pga460_registerWrite( 25, 51);
	pga460_registerWrite( 26,252);
	pga460_registerWrite( 27,128);
	pga460_registerWrite( 28,221);
	pga460_registerWrite( 29,208);
	pga460_registerWrite( 30, 15);
	pga460_registerWrite( 31, 18);
	pga460_registerWrite( 32,127);
	pga460_registerWrite( 33,127);
	pga460_registerWrite( 34,172);
	pga460_registerWrite( 35, 19);
	pga460_registerWrite( 36,175);
	pga460_registerWrite( 37,140);
	pga460_registerWrite( 38, 87);
	pga460_registerWrite( 39,104);
	pga460_registerWrite( 40,  0);
	pga460_registerWrite( 41,  0);
	pga460_registerWrite( 42,  0);
	pga460_registerWrite( 43,211);
	pga460_registerWrite( 65,143);
	pga460_registerWrite( 66,193);
	pga460_registerWrite( 67,246);
	pga460_registerWrite( 68,135);
	pga460_registerWrite( 69,  4);
	pga460_registerWrite( 70,189);
	pga460_registerWrite( 71,126);
	pga460_registerWrite( 72,103);
	pga460_registerWrite( 73,  0);
	pga460_registerWrite( 74,205);
	pga460_registerWrite( 75,  0);
	pga460_registerWrite( 95,155);
	pga460_registerWrite( 96,188);
	pga460_registerWrite( 97,204);
	pga460_registerWrite( 98,204);
	pga460_registerWrite( 99,204);
	pga460_registerWrite(100,204);
	pga460_registerWrite(101, 33);
	pga460_registerWrite(102,  8);
	pga460_registerWrite(103, 66);
	pga460_registerWrite(104, 16);
	pga460_registerWrite(105,133);
	pga460_registerWrite(106, 50);
	pga460_registerWrite(107, 75);
	pga460_registerWrite(108,150);
	pga460_registerWrite(109,255);
	pga460_registerWrite(110,  0);
	pga460_registerWrite(111,155);
	pga460_registerWrite(112,188);
	pga460_registerWrite(113,204);
	pga460_registerWrite(114,204);
	pga460_registerWrite(115,204);
	pga460_registerWrite(116,204);
	pga460_registerWrite(117, 33);
	pga460_registerWrite(118,  8);
	pga460_registerWrite(119, 66);
	pga460_registerWrite(120, 16);
	pga460_registerWrite(121,133);
	pga460_registerWrite(122, 50);
	pga460_registerWrite(123, 75);
	pga460_registerWrite(124,150);
	pga460_registerWrite(125,255);
	pga460_registerWrite(126,  0);
	pga460_registerWrite(127,178);



	pga460_registerRead( 77);


	#if 1
	//pga460_registerWrite( 64,128);
	//pga460_ultrasonicCmd(0,1);
	//pga460_pullEchoDataDump(0);
	//[ 64,255,255,255,131,  5,  1,  1,  1,  2,  3,  6,  7,  5,  8,255,255, 59, 17, 25, 11,  7, 11,  9, 17, 16, 14, 14, 18, 66, 52,  8, 11, 14, 14,  8,  7,  6, 10, 29,233,183, 11,  9,  5,  4,  6,  9,  6,  7,  8, 15, 11, 22, 37, 12,  4,  5,  4,  5,  6,  5,  5,  6, 17, 68, 41,  6,  5,  5,  5,  4,  4,  3,  3,  2,  3,  2,  8, 14,  5,  3,  5,  5,  4,  5,  8,  9,  8,  9, 16, 15,  8,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  2, 22, 36,  1,  8,  5, 11,  5,  2,  0,  0,  0,  0,  0,  0,  0,100,


	pga460_registerWrite( 64,  0);
	pga460_ultrasonicCmd(0,1);
	pga460_pullUltrasonicMeasResult();
	//[ 64, 18,247,188,255,248,


	//pga460_registerWrite( 64,128);
	//pga460_ultrasonicCmd(0,1);
	//pga460_pullEchoDataDump(0);
	//[ 64,255,255,255,131,  5,  1,  1,  1,  1,  3,  6,  7,  5,  7,255,255, 60, 17, 26, 11,  7, 12,  9, 17, 16, 13, 14, 18, 70, 54,  9, 12, 14, 14,  8,  7,  6,  9, 27,234,187, 11,  8,  4,  4,  6,  9,  4,  7,  8, 15, 12, 20, 37, 14,  5,  3,  5,  4,  5,  3,  4,  4, 16, 62, 37,  7,  7,  4,  3,  4,  5,  5,  4,  4,  4,  4, 10, 15,  5,  4,  3,  3,  3,  4,  4,  6,  3,  7, 13,  8,  6,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  2,  0,  5,  8, 27, 31, 27, 27, 18, 11,  8, 14,  5,  0, 37,


	pga460_registerWrite( 64,  0);
	pga460_ultrasonicCmd(0,1);
	pga460_pullUltrasonicMeasResult();
	//[ 64, 18,246,188,255,249,


	//pga460_registerWrite( 64,128);
	//pga460_ultrasonicCmd(0,1);
	//pga460_pullEchoDataDump(0);
	//[ 64,255,255,255,131,  5,  1,  1,  1,  1,  3,  6,  7,  5,  8,255,255, 60, 17, 26, 10,  7, 12,  9, 17, 17, 13, 14, 19, 71, 56,  8, 11, 14, 14,  8,  7,  5, 10, 29,237,191, 12,  7,  5,  4,  6,  9,  6,  6,  7, 15, 11, 20, 37, 12,  4,  3,  5,  5,  6,  4,  4,  4, 17, 68, 43,  6,  6,  4,  4,  4,  4,  4,  2,  3,  4,  3,  9, 13,  6,  5,  3,  4,  4,  6,  5,  7,  6,  9,  9,  9,  7,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  0, 18,  5,  1,  0,  0,  0,  0,  0,  0,  1,  5, 14, 14, 31,103,


	pga460_registerWrite( 64,  0);
	pga460_ultrasonicCmd(0,1);
	pga460_pullUltrasonicMeasResult();
	//[ 64, 18,246,189,255,248,
	#endif

}
#endif

#if 1
static void recorded_test_script(void)
{
	// From Flowlab EMV 2023-03-10.  It had been running for 2,094,261 loops.
	// Power on. Running 5.0.135 from Bank 1, MCUID = 1121, Gen5, BoardId = 3
	//
#if 1
	for (int i = 0; i < 44; i++)
	{
		pga460_registerWrite(i, SensorConfig.grid[i]);
	}
	for (int i = 65; i < 76; i++)
	{
		pga460_registerWrite(i, SensorConfig.grid[i]);
	}
	for (int i = 95; i < 128; i++)
	{
		pga460_registerWrite(i, SensorConfig.grid[i]);
	}

#else
	pga460_registerWrite(0,0);
	pga460_registerWrite(1,0);
	pga460_registerWrite(2,0);
	pga460_registerWrite(3,0);
	pga460_registerWrite(4,0);
	pga460_registerWrite(5,0);
	pga460_registerWrite(6,0);
	pga460_registerWrite(7,0);
	pga460_registerWrite(8,0);
	pga460_registerWrite(9,0);
	pga460_registerWrite(10,0);
	pga460_registerWrite(11,0);
	pga460_registerWrite(12,0);
	pga460_registerWrite(13,0);
	pga460_registerWrite(14,0);
	pga460_registerWrite(15,0);
	pga460_registerWrite(16,0);
	pga460_registerWrite(17,0);
	pga460_registerWrite(18,0);
	pga460_registerWrite(19,0);
	pga460_registerWrite(20,175);
	pga460_registerWrite(21,255);
	pga460_registerWrite(22,255);
	pga460_registerWrite(23,45);
	pga460_registerWrite(24,101);
	pga460_registerWrite(25,150);
	pga460_registerWrite(26,88);
	pga460_registerWrite(27,64);
	pga460_registerWrite(28,222);
	pga460_registerWrite(29,208);
	pga460_registerWrite(30,26);
	pga460_registerWrite(31,18);
	pga460_registerWrite(32,91);
	pga460_registerWrite(33,255);
	pga460_registerWrite(34,172);
	pga460_registerWrite(35,19);
	pga460_registerWrite(36,175);
	pga460_registerWrite(37,140);
	pga460_registerWrite(38,91);
	pga460_registerWrite(39,0);
	pga460_registerWrite(40,0);
	pga460_registerWrite(41,0);
	pga460_registerWrite(42,0);
	pga460_registerWrite(43,46);

	pga460_registerWrite(65,143);
	pga460_registerWrite(66,36);
	pga460_registerWrite(67,249);
	pga460_registerWrite(68,165);
	pga460_registerWrite(69,3);
	pga460_registerWrite(70,45);
	pga460_registerWrite(71,124);
	pga460_registerWrite(72,211);
	pga460_registerWrite(73,1);
	pga460_registerWrite(74,151);
	pga460_registerWrite(75,0);

	pga460_registerWrite(95,155);
	pga460_registerWrite(96,188);
	pga460_registerWrite(97,188);
	pga460_registerWrite(98,204);
	pga460_registerWrite(99,204);
	pga460_registerWrite(100,204);
	pga460_registerWrite(101,16);
	pga460_registerWrite(102,132);
	pga460_registerWrite(103,33);
	pga460_registerWrite(104,8);
	pga460_registerWrite(105,66);
	pga460_registerWrite(106,16);
	pga460_registerWrite(107,26);
	pga460_registerWrite(108,26);
	pga460_registerWrite(109,26);
	pga460_registerWrite(110,0);
	pga460_registerWrite(111,155);
	pga460_registerWrite(112,188);
	pga460_registerWrite(113,188);
	pga460_registerWrite(114,204);
	pga460_registerWrite(115,204);
	pga460_registerWrite(116,204);
	pga460_registerWrite(117,33);
	pga460_registerWrite(118,8);
	pga460_registerWrite(119,66);
	pga460_registerWrite(120,16);
	pga460_registerWrite(121,133);
	pga460_registerWrite(122,50);
	pga460_registerWrite(123,68);
	pga460_registerWrite(124,110);
	pga460_registerWrite(125,200);
	pga460_registerWrite(126,0);
	pga460_registerWrite(127,113);
#endif

	pga460_registerWrite(32,27);
	pga460_registerWrite(38,11);

	pga460_registerRead(77);  // read to clear all status bits

	#if 0
	pga460_registerWrite(64,128);
	pga460_ultrasonicCmd(0,1);
	pga460_pullEchoDataDump(0);
	[64,255,255,255,255,15,1,1,1,1,4,9,10,7,3,255,255,168,14,39,27,6,16,14,19,23,22,26,14,74,82,24,17,28,29,20,8,8,9,19,255,255,46,8,9,4,5,12,9,12,12,15,14,13,51,45,11,5,4,4,4,5,7,7,6,24,24,4,4,3,2,3,4,3,4,2,2,4,4,3,4,4,3,2,3,2,3,3,3,2,41,46,5,2,2,2,2,2,2,5,5,5,2,5,5,2,2,2,2,2,2,2,1,2,1,2,5,1,2,2,2,2,2,2,5,2,2,2,2,99,
	pga460_registerWrite(64,0);
	pga460_ultrasonicCmd(0,1);
	pga460_pullUltrasonicMeasResult();
	[64,18,94,243,255,91,
	#endif
}
#endif

#if 0

static void sideways_echo_dump(void)
{

	for (int i=0; i < 10; i++)
	{

	  char msg[80];
	  sprintf(msg,"\nNew Echo: %d\n", i);
	  tx_msg(msg);


	  pga460_ultrasonicCmd(0,1);
	  for (int j=0; j < 128; j++)
	  {
		uint8_t edd;

	    edd = pga460_pullEchoDataDump(j);  // Note: when j is 0 all 128 bytes are read from the PGA460.  For j > 0, an array value is returned.

	    // create a sideways graph  0 to 255 with 50 chars = 255
	    int star_count;
	    star_count = edd/50;
	    for (int i=0; i < sizeof(msg); i++) msg[i] = 0;
	    for (int i=0; i < star_count;i++) msg[i] = '*';

		tx_msg(msg);
		tx_msg("\n");
	  }

	}


}

#endif

void pga460_turn_on(void)
{
    // For safety, give the main capacitor time to charge - unclear if this is needed
	//ltc2943_charge_main_capacitor(ONE_TAU);  // takes 220 ms
	
    if (BoardId == BOARD_ID_SEWERWATCH_1)
    {
    	HAL_GPIO_WritePin(GEN45_VH_PWR_EN_Port,GEN45_VH_PWR_EN_Pin, GPIO_PIN_SET);

		HAL_led_delay(10);  // give it time to turn on

		//TODO monitor Power Good pin here !!!  (TPS63070RNMR)

    	MX_LPUART1_UART_Init(38400, UART_STOPBITS_2);  // This initializes lpuart1 for the initial talking to the embedded Level Board
		HAL_led_delay(5);  // give the uart a little time

    	lpuart1_flush_inbuffer();
    }
    else
    {
    	// Turn on power to the Level Board
    	// SKYWIRE GEN4: PA7 (VH_PWR_EN_Pin) is used to supply 7.5v to the Level Board.
    	// HAL_GPIO_WritePin(GEN45_VH_PWR_EN_Port,GEN45_VH_PWR_EN_Pin, GPIO_PIN_SET);

    	HAL_GPIO_WritePin(GEN5_RS485_PWR_EN_Port,GEN5_RS485_PWR_EN_Pin, GPIO_PIN_SET);

		HAL_led_delay(10);  // give it time to turn on

    	MX_USART3_UART_Init(115200, UART_STOPBITS_2);  // This initializes uart3 (RS485) for the initial talking to the Level Board
		HAL_led_delay(5);  // give the uart a little time

    	uart3_flush_inbuffer();
    }

    // For safety, give the main capacitor time to charge - unclear if this is needed
	//ltc2943_charge_main_capacitor(ONE_TAU);  // takes 220 ms
}



void main3(void)
{
	pga460_turn_on();

	recorded_test_script();

	//sideways_echo_dump();

	//tx_msg("\nTest script complete.\n");

	while (1)
	{
		pga460_get_distance(1);  // report results to console (no longer working)
		HAL_Delay(1000);
	}
}

void pga460_turn_off(void)
{
    if (BoardId == BOARD_ID_SEWERWATCH_1)
    {
    	HAL_GPIO_WritePin(GEN45_VH_PWR_EN_Port,GEN45_VH_PWR_EN_Pin, GPIO_PIN_RESET);
		HAL_UART_MspDeInit(&hlpuart1);
    }
    else
    {
    	HAL_GPIO_WritePin(GEN5_RS485_PWR_EN_Port,GEN5_RS485_PWR_EN_Pin, GPIO_PIN_RESET);
		HAL_UART_MspDeInit(&huart3);
    }
}

#if 0
static float get_temperature_from_pga460(void)
{
	float tempC;

	tempC = pga460_runDiagnostics(1, 2); // Perform a DIE temperature measurement...This tends to be higher than ambient

	LastGoodDegF = (tempC * 9.0 / 5.0) + 32.0;   // NOTE:  THIS IS A GLOBAL USED BY pga460_get_distance(0)

			// For testing
			{
				char msg[80];
				int dec1, dec2, dec3, dec4;
				float deg_C;
                deg_C = (LastGoodDegF - 32.0) * 5.0 / 9.0;  // Note: USES GLOBAL VARIABLE
				convfloatstr_4dec(LastGoodDegF, &dec1, &dec2);
				convfloatstr_2dec(deg_C, &dec3, &dec4);
				sprintf(msg, "PGA460 DegF&C, %d.%04d, %d.%02d\n", dec1, dec2, dec3, dec4);
				trace_AppendMsg(msg);
			}

  return tempC;
}
#endif

void pga460_print_data_point(char *label, PGA460_DATA_POINT *p)
{
	char msg[80];
	//int dec1, dec2;

	static int just_once = 1;

	if (just_once)
	{
		// Column headers
		sprintf(msg,"\nPreset, EDiag, Diag, Freq, Decay, ");
		trace_AppendMsg(msg);
		for (int i=0; i<PGA460_ECHOS; i++)
		{
		  //sprintf(msg, "Echo%d, Dist, Width, Peak, \n", i);
		  sprintf(msg, "Dist, Width, Peak, \n");
		  trace_AppendMsg(msg);
		}
		just_once = 0;
	}

	// print burst diagnostics (freq, decay)
	//convfloatstr_3dec(p->diag_freq, &dec1, &dec2);
	//sprintf(msg, "%s, %3d.%03d, %4d", label, dec1, dec2, p->diag_decay);

	sprintf(msg, "%s, %02X, %02X, %02X, %02X", label,  p->raw_echo_uart, p->raw_diag_uart, p->raw_diag_freq, p->raw_diag_decay);
	trace_AppendMsg(msg);

	// print echo data
	for (int i=0; i<PGA460_ECHOS; i++)
	{
		//sprintf(msg, ", E%d", i);
		//trace_AppendMsg(msg);
#if 0
		convfloatstr_2dec(p->echo[i].distance, &dec1, &dec2);
		sprintf(msg, ", %3d.%02d, %4u, %3u", dec1, dec2, p->echo[i].width, p->echo[i].raw_peak);
#else
		sprintf(msg, ", %04X, %02X, %02X", p->echo[i].raw_distance, p->echo[i].raw_width, p->echo[i].raw_peak);
#endif
		trace_AppendMsg(msg);
	}
}

#if 1 // sorted array approach
#define PHASE_1_SORTED_DATA_POINTS  5
#define PHASE_1_MEDIAN_INDEX        2
#define PHASE_2_SORTED_DATA_POINTS 11
#define PHASE_2_MEDIAN_INDEX        5
#define ALLOWED_VARIANCE_IN_INCHES  1
typedef struct {
	float distance;
} SORTED_DATA_POINT;

static SORTED_DATA_POINT data_point_array[PHASE_2_SORTED_DATA_POINTS];  // room for both phases in one array
static int data_point_count;

static int variable_delay_ms[PHASE_2_SORTED_DATA_POINTS] =
{  // delay      elapsed time
		  0,     //    0
		200,     //  200
		300,     //  500
		400,     //  900
		300,     // 1200   - end of phase 1 (1.2 seconds)
		200,     // 1400
		300,     // 1700
		200,     // 1900
		300,     // 2200
		400,     // 2600
		300      // 2900   - end of phase 2 (2.9 seconds)
};

static void print_sorted_array(void)
{
	int i;
	trace_AppendMsg("  sort,");
	for (i=0; i < PHASE_2_SORTED_DATA_POINTS; i++)
	{

		char msg[80];
		int dec1, dec2;

		convfloatstr_2dec(data_point_array[i].distance, &dec1, &dec2);
		sprintf(msg, " %3d.%02d,", dec1, dec2);
		trace_AppendMsg(msg);
	}
	//trace_AppendMsg("\n");
}


void start_sort(void)
{
	int i;

	data_point_count = 0;

	for (i=0; i < PHASE_2_SORTED_DATA_POINTS; i++)
	{
		data_point_array[i].distance = 0;
		//data_point_array[i].distance = i;  // for debug, a known sequence to verify insertion sort works
	}
	//print_sorted_array(); // for debug
}

void insert_new_data_point(PGA460_DATA_POINT * p)
{
	int i,k;


	// check if room for this data point
	if (data_point_count >= PHASE_2_SORTED_DATA_POINTS) return;  // no room for data point

	// Special case for first data point inserted into empty array
	if (data_point_count == 0)
	{
		// insert first point
		data_point_array[0].distance = p->echo[0].distance;
		data_point_count++;
		//print_sorted_array();  // for debug
		return;
	}

	// find insertion point
	for (i=0; i < data_point_count; i++)
	{
		if (p->echo[0].distance > data_point_array[i].distance) break;
	}
	// assert: i is the insertion point

	// expand sorted array
	data_point_count++;

	//k = data_point_count;
	k = PHASE_2_SORTED_DATA_POINTS - 1;

	// shift existing items down by walking backwards up the array
	while (k > i)
	{
		data_point_array[k].distance = data_point_array[k-1].distance;
		k--;
	}
	// assert: k = i

	// insert new point

	data_point_array[i].distance = p->echo[0].distance;

	//print_sorted_array();  // for debug

}

static int do_middle_points_agree(int m)
{
	float diff;

	//TODO the variance may depend on distance, e.g. farther targets may inherently produce greater variation.  Need more testing.

	diff = data_point_array[m-1].distance - data_point_array[m].distance;
	if (diff < ALLOWED_VARIANCE_IN_INCHES) return 1;

	diff = data_point_array[m].distance - data_point_array[m+1].distance;
	if (diff < ALLOWED_VARIANCE_IN_INCHES) return 1;

	return 0;
}


static int phase_1(float speedSound_inches_per_us, int trace)
{
	int i;

	// gather phase 1 data points, insert into sorted array
	for (i=0; i < PHASE_1_SORTED_DATA_POINTS; i++)
	{
		PGA460_DATA_POINT P2;

		// wait for water surface to change; echos to subside; other environmental effects to dissipate
		HAL_led_delay(variable_delay_ms[data_point_count]);

		pga460_perform_preset(PGA460_PRESET_2, &P2, speedSound_inches_per_us);

		if (trace)
		{
		  pga460_print_data_point("P2", &P2);
		  trace_AppendMsg(",   ");
		}

		insert_new_data_point(&P2);

	}

	// Check if 3 middle points "agree" with each other
	return do_middle_points_agree(PHASE_1_MEDIAN_INDEX);
	//return 0;  // for testing, force failure
}



static int phase_2(float speedSound_inches_per_us, int trace)
{
	int i;

	if (trace) trace_AppendMsg("phase_2,");

	// gather phase 2 data points, insert into sorted array
	for (i=PHASE_1_SORTED_DATA_POINTS; i < PHASE_2_SORTED_DATA_POINTS; i++)
	{
		PGA460_DATA_POINT P2;

		// wait for water surface to change; echos to subside; other environmental effects to dissipate
		HAL_led_delay(variable_delay_ms[data_point_count]);

		pga460_perform_preset(PGA460_PRESET_2, &P2, speedSound_inches_per_us);

		if (trace)
		{
		  pga460_print_data_point("P2", &P2);
		  trace_AppendMsg(",   ");
		}

		insert_new_data_point(&P2);

	}

	//if (trace) print_sorted_array();

	// Check if 3 middle points "agree" with each other
	return do_middle_points_agree(PHASE_2_MEDIAN_INDEX);
}

float pga460_take_measurement(float deg_C)
{
	float reported_distance;
	float speedSound_inches_per_us;
	int index_of_value_to_report;
	int try_count = 0;

	int trace = 1 ;

	pga460_turn_on();

	recorded_test_script();

	//deg_C = get_temperature_from_pga460();  // SIDE EFFECTS:  sets global LastGoodDegF

    extern float get_speed_of_sound_inches_per_us(float tempC);

	speedSound_inches_per_us = get_speed_of_sound_inches_per_us(deg_C);

	// The following algorithm uses 5 and 11 points.
	// An alternative is to use 5, 7, 9, 11 whichever passes first wins.
	// TODO need to determine actual physical limits of sensor and preset.
	// For example, any measurement less than "low_limit" or higher than "hi_limit"
	// indicates possible sensor fouling and/or submersion.
	// Need to capture some data where there is no echo (target is too far away).

TryAgain:

	start_sort();

	index_of_value_to_report = PHASE_1_MEDIAN_INDEX;

	if (!phase_1(speedSound_inches_per_us, trace))
	{
		index_of_value_to_report = PHASE_2_MEDIAN_INDEX;
		if (!phase_2(speedSound_inches_per_us, trace))
		{
			try_count++;
			if (try_count < 3)
			{
				if (trace)
			    {
				  print_sorted_array();
				  trace_AppendMsg("   retry\n");
			    }
				//TODO add random delay here?
				goto TryAgain;
			}
		}
	}

	reported_distance = data_point_array[index_of_value_to_report].distance;

#if 1
	// Only report distances under 24 feet, otherwise report zero to indicate overrange
	if (reported_distance >= 288)
	{
	  reported_distance = 0;
	}
#else
	reported_distance = 0;

	// Apply Criteria
	if (freq_not_ok)  reported_distance += 1;
	if (decay_not_ok) reported_distance += 2;
	//if (width_not_ok) reported_distance += 4;

	if (reported_distance == 0)
	{
		// No indications of a bad data point, so report average distance.
		// Only report distances under 24 feet, otherwise report zero to indicate overrange
		if (p2_ave.distance < 288)
		{
		  reported_distance = p2_ave.distance;
		}
	}
#endif


	// Trace reported distance
	if (trace)
	{
		char msg[80];
		int dec1, dec2;

		print_sorted_array();

		convfloatstr_2dec(reported_distance, &dec1, &dec2);
		sprintf(msg, "   rep, %3d.%02d,", dec1, dec2);
		trace_AppendMsg(msg);
	}


	if (trace) trace_AppendMsg("\n");

	pga460_turn_off();

    trace_SaveBuffer(); // save trace messages to SD card

	return reported_distance;  // distance is reported in inches


}



#else

// Original measurement

float pga460_take_measurement(float deg_C)
{
	float reported_distance;
	float speedSound_inches_per_us;
	PGA460_AVE_DATA p2_ave;
	int freq_not_ok = 0;
	int decay_not_ok = 0;
	//int width_not_ok = 0;


	pga460_turn_on();

	recorded_test_script();

	//deg_C = get_temperature_from_pga460();  // SIDE EFFECTS:  sets global LastGoodDegF

    extern float get_speed_of_sound_inches_per_us(float tempC);

	speedSound_inches_per_us = get_speed_of_sound_inches_per_us(deg_C);

	//trace_AppendMsg("distance");

#if 0
	for (int i=0; i<3; i++)
	{
		PGA460_DATA_POINT P1;
		pga460_perform_preset(PGA460_PRESET_1, &P1, speedSound_inches_per_us);
		pga460_print_data_point("P1", &P1);
		HAL_led_delay(200); // wait for water surface to change; echos to subside
		trace_AppendMsg(",   ");
	}
#endif
	//p2_ave.raw_diag_freq = 0;
	//p2_ave.raw_diag_decay= 0;
	//p2_ave.diag_freq = 0.0;
	//p2_ave.diag_decay= 0.0;

	p2_ave.distance  = 0.0;
	p2_ave.width     = 0.0;
	p2_ave.raw_peak  = 0.0;

#define NUM_OF_RAW_DATA_POINTS  3

	for (int i=0; i<NUM_OF_RAW_DATA_POINTS; i++)
	{
		PGA460_DATA_POINT P2;
		pga460_perform_preset(PGA460_PRESET_2, &P2, speedSound_inches_per_us);
		pga460_print_data_point("P2", &P2);
		HAL_led_delay(200); // wait for water surface to change; echos to subside
		trace_AppendMsg(",   ");

		// Apply criteria to each individual data point
		// Removed in 5.0.229
		//if ((P2.raw_diag_freq  < 27) || (P2.raw_diag_freq  > 29))  freq_not_ok = 1;
		//if ((P2.raw_diag_decay < 61) || (P2.raw_diag_decay > 96))  decay_not_ok = 1;
		//if (P2.echo[0].width < 2000)  width_not_ok = 1;
		//if (P2.echo[0].width > 4000)  width_not_ok = 1;

		// sum up individual trial data

		//p2_ave.raw_diag_freq  += P2.raw_diag_freq;
		//p2_ave.raw_diag_decay += P2.raw_diag_decay;
		//p2_ave.diag_freq  += P2.diag_freq;
		//p2_ave.diag_decay += P2.diag_decay;
		p2_ave.distance   += P2.echo[0].distance;
		p2_ave.width      += P2.echo[0].width;
		p2_ave.raw_peak   += P2.echo[0].raw_peak;

	}

	// Calculate averages

	p2_ave.distance   /= NUM_OF_RAW_DATA_POINTS;
	p2_ave.width      /= NUM_OF_RAW_DATA_POINTS;
	p2_ave.raw_peak   /= NUM_OF_RAW_DATA_POINTS;



#if 1

	reported_distance = 0;

	// Apply Criteria
	if (freq_not_ok)  reported_distance += 1;
	if (decay_not_ok) reported_distance += 2;
	//if (width_not_ok) reported_distance += 4;

	if (reported_distance == 0)
	{
		// No indications of a bad data point, so report average distance.
		// Only report distances under 24 feet, otherwise report zero to indicate overrange
		if (p2_ave.distance < 288)
		{
		  reported_distance = p2_ave.distance;
		}
	}


#else
	reported_distance = p2_ave.distance;
#endif



#if 1
	// Trace reported distance
	{
		char msg[80];
		int dec1, dec2;

		convfloatstr_2dec(reported_distance, &dec1, &dec2);
		sprintf(msg, "   rep, %3d.%02d,", dec1, dec2);
		trace_AppendMsg(msg);
	}
#endif

	trace_AppendMsg("\n");

	pga460_turn_off();

    trace_SaveBuffer(); // save trace messages to SD card

	return reported_distance;  // distance is reported in inches
}

#endif // alternate measurement function


#endif  // ENABLE_PGA460
