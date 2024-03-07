/*
 * dual_boot.h
 *
 *  Created on: Jan 2, 2020
 *      Author: zg
 */

#ifndef INC_DUAL_BOOT_H_
#define INC_DUAL_BOOT_H_

#include "stdint.h"

// From RM0351 Reference Manual, page 91:
// Note: When booting from bank 2, the boot loader will swap the Flash memory banks.
// Consequently, in the application initialization code, you have to relocate the vector table to
// bank 2 swapped base address (0x0800 0000) using the NVIC exception table and offset
// register.
//

//#define BANK2_ADDRESS  (uint32_t)0x08080000
#define BANK2_SWAPPED_ADDRESS  (uint32_t)0x08000000


uint32_t GetBFB2(void); // returns OB_BFB2_DISABLE or OB_BFB2_ENABLE

void Bootload(void);



#endif /* INC_DUAL_BOOT_H_ */
