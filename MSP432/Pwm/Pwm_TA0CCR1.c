#include "Pwm_TA0CCR1.h"
#include "PwmCommon.h"
#include "utils.h"
#include "msp.h"

static I_Pwm_t instance;

static void SetDutyCycle(I_Pwm_t *pwm, uint8_t duty)
{
    IGNORE(pwm);

    if(duty > 100)
    {
        duty = 100;
    }

    TIMER_A0->CCR[1] = 480 * (uint16_t)duty;
}

static const PwmApi_t api =
    { SetDutyCycle };

I_Pwm_t * Pwm_TA0CCR1_Init(GpioChannel_t channel)
{
    instance.api = &api;

    PinInfoFromGpioChannel_t pinInfo;
    GetPinInfoFromGpioChannel(channel, &pinInfo);

    P2->SEL0 |= pinInfo.pinMask;

    DoPortMapOnPort2(pinInfo.pinNumber, PMAP_TA0CCR1A);

    TIMER_A0->CCTL[1] = TIMER_A_CCTLN_OUTMOD_6; // CCR2 toggle/set
    TIMER_A0->CCR[1] = 0; // Duty cycle of zero
    TIMER_A0->CTL = TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_1; // SMCLK, count up

    return &instance;
}
