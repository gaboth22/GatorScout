#ifndef I2C_USCB0_H
#define I2C_USCB0_H

#include "I_I2c.h"
#include "TimerModule.h"

/*
 * Get singleton instance of I2c_Uscb0
 */
I_I2c_t * I2c_Uscb0_Init(TimerModule_t *timerModule);

#endif
