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

#include "rng.h"

RNG_HandleTypeDef hrng;

/* RNG init function */
void MX_RNG_Init(void) {
	hrng.Instance = RNG;
	if (HAL_RNG_Init(&hrng) != HAL_OK) {
	}
}

void MX_RNG_DeInit(void) {
	hrng.Instance = RNG;
	if (HAL_RNG_DeInit(&hrng) != HAL_OK) {
	}
}

void HAL_RNG_MspInit(RNG_HandleTypeDef* rngHandle) {
	if (rngHandle->Instance == RNG) {
		/* RNG clock enable */
		__HAL_RCC_RNG_CLK_ENABLE()
		;
	}
}

void HAL_RNG_MspDeInit(RNG_HandleTypeDef* rngHandle) {
	if (rngHandle->Instance == RNG) {
		__HAL_RCC_RNG_CLK_DISABLE();
	}
}
