#ifndef DMACONTROLLER_MSP432_H
#define DMACONTROLLER_MSP432_H

#include "I_DmaController.h"

enum
{
    DmaChannel_UartUsca0Rx = 0,
    DmaChannel_UartUsca0Tx,
    DmaChannel_UartUsca3Rx,
    DmaChannel_UartUsca3Tx,
    DmaChannel_Max
};

/*
 * Get the singleton instance of the DMA controller
 */
I_DmaController_t * DmaController_MSP432_Init(void);

#endif
