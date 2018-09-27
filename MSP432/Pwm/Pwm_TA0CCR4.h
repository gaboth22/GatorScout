#ifndef PWM_TA0CCR4_H
#define PWM_TA0CCR4_H

#include "I_Pwm.h"
#include "I_GpioGroup.h"

/*
 * Get the singleton instance of the PWM
 */
I_Pwm_t * Pwm_TA0CCR4_Init(GpioChannel_t channel);

#endif
