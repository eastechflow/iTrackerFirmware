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

#include "i2c.h"
#include "Gen4_max5418.h"

uint16_t max_i2c_addr = MAX5418_I2C_ADDRESS;

void Gen4_max5418_setR(uint8_t val)
{
	uint16_t tx_buff;

	tx_buff = (max_i2c_addr << 1);

	HAL_I2C_Mem_Write(&hi2c2, tx_buff, (uint16_t) MAX5418_VREG, 1, &val, 1, 10000);
}
