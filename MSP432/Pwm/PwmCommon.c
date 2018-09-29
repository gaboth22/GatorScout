#include "msp.h"
#include "PwmCommon.h"
#include "GpioTable.h"

void GetPinInfoFromGpioChannel(GpioChannel_t channel, PinInfoFromGpioChannel_t *portInfo)
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

void DoPortMapOnPort2(uint8_t pin, uint8_t portMapType)
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
