#ifndef INTERRUPT_WHEELENCODER_H
#define INTERRUPT_WHEELENCODER_H

#include "I_Interrupt.h"
#include "I_GpioGroup.h"
#include "GpioTable.h"

/*
 * Get singleton instance of wheel encoder interrupt
 */
I_Interrupt_t * Interrupt_WheelEncoder_Init(GpioChannel_t gpioChannel);

#endif
