#ifndef PWMCOMMON_H
#define PWMCOMMON_H

#include "I_GpioGroup.h"

#define EXPAND_AS_GET_PORT_AND_PIN(_gpioName, _index, _port, _bit, _initialMode) \
    case _gpioName: \
        pinMask = BIT ## _bit; \
        pinNumber = _bit; \
        break; \

typedef struct
{
    uint16_t pinMask;
    uint8_t pinNumber;
} PinInfoFromGpioChannel_t;

/*
 * Get pin information from GPIO channel - Get actual pin # and pin mask
 */
void GetPinInfoFromGpioChannel(GpioChannel_t channel, PinInfoFromGpioChannel_t *portInfo);

/*
 * Do port map on a pin number ONLY IN PORT 2
 */
void DoPortMapOnPort2(uint8_t pin, uint8_t portMapType);

#endif
