#ifndef PWM_TA0CCR1_H
#define PWM_TA0CCR1_H

#include "I_Pwm.h"
#include "I_GpioGroup.h"

/*
 * Get the singleton instance of the PWM
 */
I_Pwm_t * Pwm_TA0CCR1_Init(GpioChannel_t channel);

#endif
