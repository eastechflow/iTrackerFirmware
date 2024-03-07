/*
 * Hydro.h
 *
 *  Created on: Mar 12, 2020
 *      Author: Zachary
 */

#ifndef INC_HYDRO_H_
#define INC_HYDRO_H_

#include "stdint.h"

typedef struct
{
  // Initial Values From UI
  uint32_t FirstLogAddr;
  uint32_t LastLogAddr;
  float    VerticalMount;
  float    Pipe_ID;
  uint32_t Population;
  float    AveWaterUsage_GPD;
} HYDRO_DECOMP_INPUT;

void Hydro_ProcessImportedData(const TCHAR * ImportedFilename);

void qratio_test(void);

float Hydro_CalcFlowFromDeltaQ(float qratio);
float Hydro_CalcNormalizedDeltaQ(float qratio);
void Hydro_NewDataLogged(void);


#endif /* INC_HYDRO_H_ */
