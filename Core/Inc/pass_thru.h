/*
 * pass_thru.h
 *
 *  Created on: Jul 16, 2021
 */

#ifndef INC_PASS_THRU_H_
#define INC_PASS_THRU_H_

void PassThruMainLoop(void);

void esp32_enter_bootload_mode(int ControlBootPin);  // 1 to put in bootload mode; 0 for simple uart pass-thru

#endif /* INC_PASS_THRU_H_ */
