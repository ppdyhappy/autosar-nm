#ifndef _CAN_H_
#define _CAN_H_

#include "autosarNM.h"
#include "Driver_Common.h"
int MCU_CAN_Transmit(NMPDU_t* NMPDU);
void MCU_CAN_Init(void);

#endif
