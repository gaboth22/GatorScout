#ifndef INTERRUPT_1MSSYSTEMTICK_H
#define INTERRUPT_1MSSYSTEMTICK_H

#include "I_Interrupt.h"

/*
 * Get singleton instance of 1ms system tick
 */
I_Interrupt_t * Interrupt_1MsSystemTicks_Init(void);

#endif
