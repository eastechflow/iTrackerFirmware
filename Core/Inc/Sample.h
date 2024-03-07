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

#ifndef SOURCES_FUNCTIONS_SAMPLE_H_
#define SOURCES_FUNCTIONS_SAMPLE_H_

#include "stdint.h"

#define SAMPLE_DATA_ONE_MONTH     1
#define SAMPLE_DATA_FILL_MEMORY 159


// For SmartFlume testing
#define SAMPLE_DATA_TYPE_PIPE_8    0  // original demo data
#define SAMPLE_DATA_TYPE_FLUME_4   1  // 4 inch flume
#define SAMPLE_DATA_TYPE_FLUME_6   2  // 6 inch flume
#define SAMPLE_DATA_TYPE_FLUME_8   3  // 8 inch flume
#define SAMPLE_DATA_TYPE_FLUME_10  4  // 10 inch flume
#define SAMPLE_DATA_TYPE_FLUME_12  5  // 12 inch flume
#define SAMPLE_DATA_TYPE_MAX       6

void loadSampleData(uint8_t HowManyCycles, int DataType);  // One cycle is 30 days worth of data

void viewData(void);

uint8_t Import_CSV_File(const TCHAR * Filename);  // "Site001.csv"

void PerformFactoryConfig(void);

#endif
