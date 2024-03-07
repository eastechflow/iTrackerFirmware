/*
 * esp32.h
 *
 *  Created on: Mar 23, 2022
 *      Author: Zachary
 */

#ifndef INC_ESP32_H_
#define INC_ESP32_H_

void esp32_turn_on(void);
void esp32_wakeup(void);
void esp32_sleep(void);
void esp32_pull_boot_pin_low(void);
void esp32_release_boot_pin(void);


#endif /* INC_ESP32_H_ */
