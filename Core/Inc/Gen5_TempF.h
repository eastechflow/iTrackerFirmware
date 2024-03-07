/*
 * Gen5_TempF.h
 *
 *  Created on: Apr 28, 2022
 *      Author: Zachary
 */

#ifndef INC_GEN5_TEMPF_H_
#define INC_GEN5_TEMPF_H_

void Gen5_MAX31820_init(void);

void get_tempF(void);  // selects between Gen4 and Gen5 at run time  // SIDE-EFFECTS !! Sets LastGoodDegF

#endif /* INC_GEN5_TEMPF_H_ */
