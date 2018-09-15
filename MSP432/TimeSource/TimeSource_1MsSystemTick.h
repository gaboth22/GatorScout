#ifndef TIMESOURCE_1MSSYSTEMTICK_H
#define TIMESOURCE_1MSSYSTEMTICK_H

#include "I_TimeSource.h"
#include "I_Interrupt.h"

/*
 * Get singleton instance of 1ms time source
 *
 * @param oneMsInterrupt Interrupt for this time source
 */
I_TimeSource_t * TimeSource_1MsSystemTick_Init(I_Interrupt_t *oneMsInterrupt);

#endif
