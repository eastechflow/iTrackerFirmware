/******************************************************************************
*
* Revision: A
* Author:   TWebber
* Created:  3/7/2017
*
* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*  FILE: AD5272.h
* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* DESCRIPTION: Header file to define functions for Backlight controller.
*
* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

#ifndef _AD5270_H_
#define _AD5270_H_

#include "stdint.h"
#include <stdbool.h>

/* These are all shifted left 2 bits so they can be ORed with D8 and D9 */
#define AD5270_NOP				(0x00)
#define AD5270_WRITE_WIPER      (0x01)
#define AD5270_READ_WIPER       (0x02)
#define AD5270_STORE_WIPER      (0x03)
#define AD5270_SW_RESET         (0x04)
#define AD5270_50TP_RD          (0x05)
#define AD5270_50TP_ADDR_RD     (0x06)
#define AD5270_CONTROL_WR       (0x07)
#define AD5270_CONTROL_RD       (0x08)
#define AD5270_SW_SHUTDOWN      (0x09)


bool     Gen5_AD5270_Set(unsigned char reg_address, uint16_t reg_value);
uint16_t Gen5_AD5270_Get(unsigned char reg_address);
void     Gen5_AD5270_Init(void);
void     Gen5_AD5270_SetWiper(uint16_t WiperPos);  // Must call Init once before using this function.  After thatn, can use as often as needed.

#endif

/******************************************************************************
* Done.
*/
