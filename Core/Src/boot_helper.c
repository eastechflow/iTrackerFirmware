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

// https://www.st.com/content/ccc/resource/technical/document/application_note/group0/6b/6d/66/98/7d/3e/46/cc/DM00282546/files/DM00282546.pdf/jcr:content/translations/en.DM00282546.pdf
// https://www.instructables.com/id/The-Incredible-STM32-L4/
// https://scienceprog.com/flashing-programs-to-stm32-embedded-bootloader/
// https://stackoverflow.com/questions/28288453/how-do-you-jump-to-the-bootloader-dfu-mode-in-software-on-the-stm32-f072

// BOOT
#include "boot_helper.h"
#include "debug_helper.h"
#include <stdint.h>
#include "stm32l4xx_hal.h"

#define SYSMEM_RESET_VECTOR            0x1fffC804
#define RESET_TO_BOOTLOADER_MAGIC_CODE 0xDEADBEEF
#define BOOTLOADER_STACK_POINTER       0x20002250

uint32_t dfu_reset_to_bootloader_magic;

void __initialize_hardware_early(void) {
    if (dfu_reset_to_bootloader_magic == RESET_TO_BOOTLOADER_MAGIC_CODE) {
        void (*bootloader)(void) = (void (*)(void)) (*((uint32_t *) SYSMEM_RESET_VECTOR));
        dfu_reset_to_bootloader_magic = 0;
        __set_MSP(BOOTLOADER_STACK_POINTER);
        bootloader();
        while (42);
    } else {
        SystemInit();
    }
}

void dfu_run_bootloader() {
    dfu_reset_to_bootloader_magic = RESET_TO_BOOTLOADER_MAGIC_CODE;
	//MX_USB_DEVICE_Init();
	while(!USB_Received) {}
	__initialize_hardware_early();
	while(1){}
    //NVIC_SystemReset();
}
