#include "Pwm_ConfigurableTA0CCR1.h"
#include "msp.h"
#include "GpioTable.h"
#include "UsefulMacros.h"
#include "I_GpioGroup.h"
#include "utils.h"

typedef struct
{
    uint16_t pinMask;
    uint8_t pinNumber;
} PinInfoFromGpioChannel_t;

typedef struct
{
    I_Pwm_t interface;
    uint16_t currentPinMask;
    uint8_t currentPinNumber;
} Pwm_ConfigurableTA0CCR1_t;

static Pwm_ConfigurableTA0CCR1_t instance;

static void GetPinInfoFromGpioChannel(GpioChannel_t channel, PinInfoFromGpioChannel_t *portInfo)
{
    uint16_t pinMask;
    uint8_t pinNumber;

    switch(channel)
    {
        GPIO_TABLE(EXPAND_AS_GET_PORT_AND_PIN);
    }

    portInfo->pinMask = pinMask;
    portInfo->pinNumber = pinNumber;
}

static void DoPortMap(uint8_t pin, uint8_t portMapType)
{
    uint32_t interruptState;
    interruptState = __get_PRIMASK();
    __disable_irq();

    PMAP->KEYID = PMAP_KEYID_VAL;
    PMAP->CTL = PMAP_CTL_PRECFG;
    volatile uint8_t *registerPointer = (volatile uint8_t *)(&P2MAP->PMAP_REGISTER[0]);
    registerPointer += pin;
    *registerPointer = portMapType;
    PMAP->KEYID = 0x00;

    __set_PRIMASK(interruptState);
}

static void SetDutyCycle(I_Pwm_t *pwm, uint8_t duty)
{
    IGNORE(pwm);

    if(duty > 100)
    {
        duty = 100;
    }

    TIMER_A0->CCR[1] = 480 * (uint16_t)duty;
}

static void ChangePortMap(I_Pwm_t *pwm, GpioChannel_t channel)
{
    IGNORE(pwm);
    DoPortMap(instance.currentPinNumber, PMAP_NONE);

    P2->SEL0 &= ~(instance.currentPinMask);
    PinInfoFromGpioChannel_t pinInfo;
    GetPinInfoFromGpioChannel(channel, &pinInfo);
    instance.currentPinMask = pinInfo.pinMask;
    instance.currentPinNumber = pinInfo.pinNumber;

    P2->SEL0 |= instance.currentPinMask;
    DoPortMap(instance.currentPinNumber, PMAP_TA0CCR1A);
}

static const PwmApi_t api =
    { SetDutyCycle, ChangePortMap };

I_Pwm_t * Pwm_ConfigurableTA0CCR1_Init(GpioChannel_t channel)
{
    instance.interface.api = &api;

    PinInfoFromGpioChannel_t pinInfo;
    GetPinInfoFromGpioChannel(channel, &pinInfo);
    instance.currentPinNumber = pinInfo.pinNumber;
    instance.currentPinMask = pinInfo.pinMask;

    P2->SEL0 |= pinInfo.pinMask;

    DoPortMap(pinInfo.pinNumber, PMAP_TA0CCR1A);

    TIMER_A0->CCTL[1] = TIMER_A_CCTLN_OUTMOD_6; // CCR1 toggle/set
    TIMER_A0->CCR[1] = 0; // Duty cycle of zero
    TIMER_A0->CTL = TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_1; // SMCLK, count up

    return &instance.interface;
}
