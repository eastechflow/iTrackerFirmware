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

#ifndef VERSION_H_
#define VERSION_H_

// zg Feb 18, 2020 During beta, f/w was using 5.0.99xx.  Code freeze on 5.0.9930
// zg Feb 18, 2020 released as 4.1.3.

// 3005 added DEBUG cmd
// 3008 addped trace functionality to Debug Cell modem stuff
// 3009 fixed PING to match Slim's 4.0.17 version
// 3013
// 3014 has factory autoconfig serial number, not of much interest given the way mfg occurs
// 3015 attempted fix for CellLog; found file system reinitialization error
// 3016 fixed fat fs reinitialization error.  Gave to Max
// 4022 turning on cell phone via SD card will reconfigure WiFi to use internal antenna
// 4023 For Max to test on his cell unit
// 4025 Sent back with Zack's first cell unit to try and identify where problem lies.
// 4048 First release of new cell protocol
// 4055 Switch to debug binaries.  Enable SD card writes from Bank 2
// 4056 Switch to sensor trace code
// 4057 First side-by-side Trace code
// 4058 Added binary search 2 and average of 5 readings
// 4063 ISR changes; save 100 points of raw data; print out in Trace
// 4064 Binary search using voltage
// 4065 Binary search but ISRs not turned off at end of measurement cycle
// 4066 Bin Search Voltage using new ISR's
// 4067 Subtle fix to Bin Search; allow cell calls and sensor trace output for dual testing
// 4068 Subtle fix to Bin Search; allow cell calls and sensor trace output for dual testing 20 ms delay
// 4069 removed delays in sensor_single, need to carefully understand turbulence a bit more.
// 4070 removed old way of measuring; added 20 ms delay in-between readings; allowed retries at each gain setting;
//      added distance2 as ave of all echos; put in fail-safe code.
// 4071 prep for release, code diff, etc.
// 4.2.0000 Added 1 sec delay in-between retries.
// 4.2.2 Stress test support "Enable_StressTest.txt"
// 4.2.3 Add delay of 185 ms to allow caps to charge
// 4.2.4
// 4.2.5 Add average gain logic; increase timeout in SewerWatch
// 4.2.6 Remove delay for caps charging, except for when averaging time-spaced data
// 4.2.9 For manufacturing, supports FactoryTest.txt
// 4.2.10 To support old modems (without FWSWITCH cmd) being re-configured to use hologram
// 4.3.0  Default IP to use first AWS server
// 4.3.1  Default IP to use load-balanced AWS server
// 4.3.2  Extended socket dial timeout from 20s to 90s
// 4.3.3  Corrected defect when switching firmware images; changed factory default to ATT (context 1) to match modem default (FWSWITCH 0). Fixed RTC sync errors when Cell UTC not obtained.
// 4.3.4  Fixed the SLEEP commandA
// 4.3.5  Added Zentri Factory Reset
// 4.3.6  Added support for "S000000" responses from Zentri
// 4.3.7  Added MAC address to file for FactoryResetZentri; Added FactoryEraseZentri feature; Added Manifest 9 9 9 support
// 4.3.8  Removed Cell retries, added 90s timeout for ACK, cleaned up cell shutdown code, added FirstResponse support, improved SewerWatch web app upgrade process on Zentri
// 4.3.9  Ignored errors for ATE0 command --- it seems to report an error if the echo is already off
// 4.3.91 Increased time for CEREG to connect from 1 second to 2 seconds between retries (timeout total of 9s to 18s)
// 4.3.92  Changed default values for ctx and apn for LVW2 and LVW3 modems based on tests with hologram cards by Matt Williams; Added SewerWatch files for FirstResponseIQ;Fix +++ sequence
// 4.3.93  Test for Zack and Matt - longer power on time to see if cell modems would talk (they did not and must have been zapped by static)
// 4.3.94  Fix for network registration and activation
// 4.3.95  Change 1s timeouts to 5s
// 4.3.96  Change timeouts to 10s
// 4.3.97  Add retries to SCFG and SGACT
// 4.3.98  Fix timeouts for SGACT, SD, SH to support older modem LE910-SV1
// 4.3.107  Fix to only power down modem when going to sleep; added state_CEREG and state_OPEN_SOCKET to avoid repeated command cycles
// 4.4.0  Official release for cell phone fixes and 1st Response IQ
// 4.4.1  Added SleepN command where N is 1..9; attempted fix for when Alert1 > Alert2; added dev code for JSON settings from the Cloud
// 4.4.2  More fixes for when Alert1 > Alert2
// 4.4.3  Add Xbee support.  Default is now Xbee3 unless from WiFi xbee3_config is set to 0
// 4.4.4  Add alert "retry" when Cloud does not ACK the alert.  Temporarily changes log interval to 5 minutes until alert is acknowledged.
// 4.4.5  Fixed FactoryEraseZentri; Added Xbee3 support; Added latching Alerts; fixed corrupt WebApp files
// 4.4.6  Fixed possible null-terminator issue in WiFi; enforced criteria for changing serial number to prevent "null" serial numbers; looked at gain stuck at 241 issue
// 4.4.6  Fixed temp in C for Live readings;
// Need version for Flowscope.  Was using 3 digits on zip files.  Last zip: 008  This codebase: 009
// 0.0.9 Tests LPUART1 without requiring an SD card
// 0.0.10 Repeatedly tests iTracker ultrasonic sensor for Mark Watson evaluation
// 0.0.11 Added control over gain
// 0.0.12 Added output via PE6
// 0.0.13 Added output via PE6
// 0.0.14 Added output via PE8 (PGOOD) MUST DISCONNECT at R20  see tim.c line 51 to disable complementary output signal
// 0.0.17 Complementary output with pulldown at end of signal
// 0.0.18 Code clean up prior to attempting to separate setup from trigger
// 0.0.19 Allow gain of zero for testing
// 0.0.20 2nd Drop from Mark Watson
// 0.0.21 Merge in progress  muxing uarts to either wifi or cell and to support ESP32 instead of Zentri
// 0.0.22 Drop to Mark Watson for testing PGA460 using iTracker rev 5.08 boards (skywired)
// 0.0.23 Drop to Mark Watson for testing rev 5.09 analog circuit
// 0.0.22 Drop to Mark Watson for testing PGA460 using iTracker rev 5.08 boards (skywired)
// 0.0.24 Drop to Mark Watson for tracing echos on SD card
// 0.0.25 Drop to Mark Watson for testing PGA460
// 5.0.25 Drop to Matt and Max for BT app development
// 5.0.26 Drop to Matt and Max for BT app development; Changes for forcing bootload; UART4 reports all traffic on UART2 (WIFI)
// 5.0.27 Drop to Matt and Max for BT app development
// 5.0.28 Fix BT going to sleep --- extend timeout to 10 minutes of inactivity before sleeping
// 5.0.29 Allow external WiFi and external Cell antenna configuration.  Add SD card support for antenna config.
// 5.0.30 Fix issues related to CELL_RTS and CELL_CTS.
// 5.0.31 Copy to Max for Laird testing; More RTS CTS issues; added trace functions; cleaned up esp32 boot time
// 5.0.32 Sent to Watson before vacation
// 5.0.33 Fixed issue when Laird is not physically present; set default Carrier and Bands in laird_reset
// 5.0.34 Folded in code from Mark Watson for OneWire and AD5270 digital pot; sent to Mark Watson
// 5.0.35 Used to test new iTracker5 boards
// 5.0.36 Revised on-site remotely
// 5.0.37 Revised in Sheridan, copied to Tulsa
// 5.0.38 Delivered to Tulsa
// 5.0.39 Delivered to Watson
// 5.0.40 Delivered to Watson, uses Vel2 UART for sensor testing
// 5.0.41 Delivered to Watson, prints out auto search results in real time on Vel2 (lpuart1)
// 5.0.42 Fixed DegC/DegF error with MAX31820; Delivered to Tulsa and to Watson
// 5.0.43 Added manual and auto loop selection via keystrokes
// 5.0.44 Updated gain table for new circuit with original R19 of 6.65K
// 5.0.45 Fix initialization error in gain table
// 5.0.46 Fix binary search.  First version that worked well
// 5.0.47 Power testing
// 5.0.48 More Power testing
// 5.0.49 To Matt for testing
// 5.0.50 To Watson and Matt for testing
// 5.0.51 Fix spy issues and init of temp sensor; working on init of Laird
// 5.0.52 Test code for Watson
// 5.0.53 Release candidate - had bug
// 5.0.54 Release candidate - still has 400 uA during sleep - why ?
// 5.0.55 Test control of CTS RTS for CELL at Sleep
// 5.0.56 Release candidate to Matt
// 5.0.57 Fix reset error on bit flags used in MainConfig; several bitfields were mixed up with a general "user" reset.
// 5.0.58 Fixes from Mark Watson on GPIO pin initialization
// 5.0.59 Added support for [] passthru from WiFi to Cell modem; removed WebApp.c and WebApp.h
// 5.0.60 Fixed bug with main loop going to sleep when RTC was sync'd.  Removed laird_query_config
// 5.0.61 Updated buffer size reporting. Turned on vbat code in main.  Some changes to uart2 buffer defines
// 5.0.62 BUGHUNT
// 5.0.63 BUGHUNT and esp32 power on brownout
// 5.0.64 Added code to slowly turn on the ESP32 to avoid brownouts.  Cleaned up code.  Release candidate.
// 5.0.65 Turned off charging resistor
// 5.0.66 Fixed pin states to lower power draw during sleep
// 5.0.67 Add cmds to control Recharge; More fixes to lower power draw during sleep; read DegF while waiting for cap to charge (saves on time)
// 5.0.68 Revert read DegF and remove pacing to ESP32 on tx (fix needs to occur in ESP32 code); shift demo data to start at midnight
// 5.0.69 Fix issue with enabling RTC battery charging bit at power on; add ESP32 bootload via UART4
// 5.0.70 Adjust pacing of uart2 to esp32 to better match BLE transmission results
// 5.0.71 Implement XON XOFF with ESP32
// 5.0.72 Sync empty log files with RTC when set by the App; report RTC batteries depleted message in log file for units with bad batteries
// 5.0.73 Removed automatic reset when upgrading from SD Card.  Must manually power-cycle as in Gen4.   Added duplicate log file to indicate hiccup in RTC battery.
// 5.0.74 Adjusted uart buffer sizes; add code to prevent buffer overruns, especially when SPY is enabled
// 5.0.75 Remove unused code
// 5.0.76 Carefully clean up initialization sequence, RTC functions, and logging of messages.
// 5.0.77 Remove SD Card message from Temp Sensor Init; remove RTC read/write of backup register
// 5.0.78 Added delay at power-on as a reed switch debounce/power supply stablization.  Remove report of power on for 1st log record.
// 5.0.79 Fixed initialization of time anchor TimeToTakeReading before main loop starts
// 5.0.80 Skip making cell call if no actual cell modem found, or if confirmation process is still active
// 5.0.81 Fix parasitic power issue (or conversion time issue) with Temp sensor; add LED2 control
// 5.0.82 Added battery accumulator to log record and log output
// 5.0.83 Fixed issue with LED possibly pulsing on/off during cell phone calls even after initial power-on has expired
// 5.0.84 Add cloud packet support; fix LED on issue; fix LTC2943 issues; add on/off flag to report battery accumulator
// 5.0.85 Carefully initialized all pins
// 5.0.86 Fixed LED to only be enabled when WiFi is active; even after a data point is taken; and if unit wakes up in WiFi mode. Reduced debounce time to 3 seconds. Fixed USB power issues with LTC2943.
// 5.0.87 Add correction for sleep current to LTC2943.  The resistor used on all iTrackers does not allow sleep current to be measured.
// 5.0.88 Add fix for Cloud packets timestamp.  Previous versions reported the timestamp from the last log record of the series, instead of the first.
// 5.0.89 Add code to support PGA460 testing from PC using VEL1 (UART4) port
// 5.0.90 Add code to support testing wake up slow circuits using keystrokes on UART1.  Can toggle power to esp32 and PGA460 individually
// 5.0.91 Clarify PA15 power-on state and change all pin init code to first set the state of the output, then initialize the pin as output.  This more correctly illustrates what is happening on the pin.
// 5.0.92 Support for board id
// 5.0.93 Support for tracing PGA460 using UART4 and UART3
// 5.0.94 Tracking down problems with batteries and ESP32 brownouts.  Moved Cell power on until AFTER ESP32 power up
// 5.0.95 Remove infinite loops, report diagnostics, and turn off LED, when powering-on the ESP32.
// 5.0.96 Power off ESP32 if it does not strictly follow protocol.  Add SD file flag "Disable_ESP32.txt" to defeat powering on ESP32 entirely.
// 5.0.97 Allow for ESP32 to send two extra lines at the end of the protocol sequence.
// 5.0.98 Allow more time for ESP32 to echo the MAC address back.  It took over 2 seconds on Unit 4 and Unit 7 (of 7) which caused the ESP32 to be igonored by the firmware for those two units.
// 5.0.99 Prevent long BLE names;
// 5.0.100 Allow longer time for cell registration on the network (time for CEREG to complete before timing out).  Allow up to 7 minutes, essentially once per day.  Bails out early if registered.
// 5.0.101 Make cell call after 48 data points rather than 96; Allow CEREG 7 minutes of cell modem ON time before giving up
// 5.0.102 Correct implementation errors in CEREG process
// 5.0.103 For Turnipseed engineering.  Make cell phone call every 4 data points for first 24 hours, then switch frequency to every 48 points.
// 5.0.104 Turn on RTC battery charging bit at power on based on config setting.  May switch to using BoardId ?
// 5.0.105 Turn on RTC battery charging bit at power on based on config setting using BoardId
// 5.0.106 Turn on RTC battery charging bit at power on based on config setting; config setting must be set by production.  Installed battery must be visually identified and {"comm":24} executed.
// 5.0.107 Increased ACK timeout from 90 seconds to 180 seconds
// 5.0.108 Added ESP32 power down/up retries; Moved flash writes to main loop to avoid possible issues with magnetic wake up
// 5.0.109 Extend laird power on delay to 15 seconds
// 5.0.110 New main loop
// 5.0.111 kludge for testing potted units; had deep sleep turned off, units will not show zero power draw when sleeping
// 5.0.112 Fixed errors with main loop.  Fixed laird_reset() functions, tried to add laird reconfig retreis...
// 5.0.113 Fixed timing errors related to wifi sleep/wake and extend
// 5.0.114 Fixed error related to powering up esp32; fixed "new main loop" collisions with esp32 and cell
// 5.0.115 Before revert of main loop back to something similar to original
// 5.0.116 Revert main loop back to something similar to original
// 5.0.117 Schedule next reading after each reading is taken
// 5.0.118 Fix low level issue when esp32 power is shut off and the brownout message is sent to the iTracker
// 5.0.119 Fix issue with computing WiFiWakeUp
// 5.0.120 Disable all interrupts when turning on the esp32
// 5.0.121 Enable rapid WiFi wakeups (every minute) for stress testing
// 5.0.122 Production code --- wifi wakeup every 5 minutes
// 5.0.123 Production code enable cell channels 2,4,12,13,17 for USA ; add testing code for esp32 wakeup
// 5.0.124 Experimental testing code for esp32 wakeup and power issues
// 5.0.125 Remove "GotHere" trace messages
// 5.0.126 Extend timeout for echo of BT name and MAC address
// 5.0.127 Remove laird_reset from serial number command
// 5.0.128 Support rev 3 comm boards with AUTORUN problem (M2.41 not grounded, must send AT! at power on) and other bug fixes (CEREG, ACK_TIMEOUT)
// 5.0.129 Release to field.  Add wakeup from micro SD card insert/remove.
// 5.0.130 Fix error on extending wifi connections at or near the "hard edge"
// 5.0.131 Enable WakeUpFrom_EXTI (using SD card insert/removal to wake up unit).  Unit will stay on for the initial "hard" 10 minutes.
// 0.0.132 Test of 3 min automatic wake ups logging to data file
// 1.0.132 Test of 3 min automatic wake ups logging to data file
// 5.0.132 Copy before changing deep_sleep() to monitor sleep; using deep_sleep() from light_sleep()
// 5.0.133 Add CMD_STAY_AWAKE (7) to allow for longer ON times when setting up cellular - give registration a long time to occur
// 5.0.134 Tweak light_sleep() code and test function
// 5.0.135 Merge Sewer Watch support
// 5.0.136 Revert wifi_delay_timer_ms to 5000
// 5.0.137 Support nnn.n values in s2c
// 5.0.138 Battery percentage issues, report preset 1 and 2 distances to SD card, temperature compensation for SewerWatch
// 5.0.139 Report preset 2 distances instead of preset 1
// 5.0.140 Correctly report preset 2 distances instead of preset 1; skip unneeded temperature measurement
// 5.0.141 Add code to measure and report voltages during measurement cycle
// 5.0.142 Add fully_charge_capacitor() calls to esp32, cell, and pga460
// 5.0.143 Add low-level VCA measurement


// 0.0.200 New state machine for performing cellular activities
// 0.0.201 New state machine for performing cellular activities
// 0.0.202 Merge code streams
// 0.0.203 Code drop to Mark Watson for testing SewerWatch temperature sensor
// 0.0.204 Add status messages to cell test so App can inform user of progress
// 0.0.205 Revise cell state machine to provide more feedback to App. Simply messages.
// 5.0.206 Correct typo in main.c relating to GetBFB2 that was is 203 and 204
// 5.0.207 Fix TCP config and TCP start code - BLE name was "SewerWatch"
// 5.0.208 Change BLE name back to iTracker
// 5.0.209 Fixed error with Block Comm Bds and the AT! being sent just after CONNECT.  Fixed error when log interval was excessive (buffer overwrite)
// 5.0.210 Changed tower holdoff to 8 hrs on failed 30 min registrations; added status messages; added iSIM and eSIM cmds; added cell_band command
// 5.0.211 Minor fixes for bringing up brand new boards; added loop to test repeated  pga460 measurements
// 5.0.212 Add pga460 diagnostic(freq, decay); fix error in converting floats to strings (rounding code did not work)
// 5.0.213 Add grid import/export.  Add P1, P2 reporting to SD card.
// 5.0.214 Focus on Preset 2 Echo 0 data only.  Add criteria to filter data and report zero distance.
// 5.0.215 Revise and enable criteria based on deadband crossover testing
// 5.0.216 Fix bug with Peak Amplitude; revise Decay criteria (add width under 2000, reported distance must be under 24 feet)
// 5.0.217 Trace reported distance
// 5.0.218 Remove width criteria
// 5.0.219 Revise freq and decay criteria based on data from Mark LaPlante (for demo to Duke)
// 5.0.220 Add support for AU default bands 1,3,5,7,28,40  via cmd:  38,9061; allow numeric cmds ; fix bug in cell state machine for try band
// 5.0.221 Remove band 40 from AU (band 40 is not supported by Laird modem)
// 5.0.222 Remove decay criteria entirely
// 5.0.223 Turn on Preset 1 raw data
// 5.0.224 Fix TCPCFG to use ctx
// 5.0.225 Add FactoryExtract0001.txt file creation
// 5.0.226 Turn off Yellow LED when done with FactoryExtract
// 5.0.227 Add Hail Mary code
// 5.0.228 Add socket close; revise Hail Mary code; add code for hardstop and softstop for battery voltage and capacity; revise led visual indicators; enable cell and wifi logging by default
// 5.0.229 Remove criteria from pga460
// 5.0.230 Fix cell bands 25,26,27,28.  Fix cell_apn logic to ignore null strings and report cfg values. Make BLE name configurable via {"Command":"b1"}
// 5.0.231 Fix PGA460 print stmt; changed default apn from hologram to flolive.net
// 5.0.232 added Disable_Cell.txt and Force_iTracker.txt to SD Card commands
// 5.0.233 fix error in Disable_Cell.txt and Force_iTracker.txt to SD Card commands
// 5.0.234 try new pga460 measurement algorithm
// 5.0.235 Add alert verification loop; reduce cell phone on time for retries to avoid battery drain when cell antenna is defective or missing
// 5.0.236 Remove alert latching
// 5.0.237 Special factory test code for pga460 - gathers lots of raw data  - do not ship to customer
// 5.0.238 Ok for customer ship
// 5.0.239
// 5.0.240 Changed low-level PGA460 timing to wait long enough for BURST+LISTEN command to finish
// 5.0.241 Added uart diagnostic byte to preset; added support for configuring SmartFlume and other product types

#if 0
#define VERSION_MAJOR	4
#define VERSION_MINOR	4
#define VERSION_BUILD	6
#else
//iTracker5
#define VERSION_MAJOR	5   // use 2 for experimental
#define VERSION_MINOR	0
#define VERSION_BUILD	241
#endif
#endif
