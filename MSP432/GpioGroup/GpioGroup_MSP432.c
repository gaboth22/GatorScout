#include "msp.h"
#include "GpioGroup_MSP432.h"
#include "GpioTable.h"
#include "utils.h"

#define EXPAND_AS_INPUT(_port, _pin) P ## _port ## DIR &= ~BIT ## _pin
#define EXPAND_AS_OUTPUT(_port, _pin) P ## _port ## DIR |= BIT ## _pin
#define EXPAND_AS_GETSTATE(_port, _pin) (GpioState_t)((P ## _port ## IN >> _pin) & 0x01)
#define EXPAND_AS_SETSTATE(_port, _pin, _state) (_state == GpioState_High) ? (P ## _port ## OUT |= BIT ## _pin) : (P ## _port ## OUT &= ~BIT ## _pin)

#define EXPAND_AS_SETSTATE_FROMGROUP_INTERNAL(_index, _port, _bit) \
        case _index: \
            EXPAND_AS_SETSTATE(_port, _bit, state); \
            break; \

#define EXPAND_AS_SETSTATE_FROMGROUP(_gpioName, _index, _port, _bit, _startingMode) \
    EXPAND_AS_SETSTATE_FROMGROUP_INTERNAL(_index, _port, _bit) \

#define EXPAND_AS_GETSTATE_FROMGROUP_INTERNAL(_index, _port, _bit) \
    case _index: \
        readState = EXPAND_AS_GETSTATE(_port, _bit); \
        break; \

#define EXPAND_AS_GETSTATE_FROMGROUP(_gpioname, _index, _port, _bit, _startingMode) \
        EXPAND_AS_GETSTATE_FROMGROUP_INTERNAL(_index, _port, _bit) \

#define EXPAND_AS_SETSTARTINGMODE_FROMGROUP_INTERNAL(_index, _port, _bit, _startingMode) \
    case _index: \
        switch(_startingMode) \
        { \
            case(GpioMode_Input): \
                EXPAND_AS_INPUT(_port, _bit); \
                break; \
            case(GpioMode_Output): \
                EXPAND_AS_OUTPUT(_port, _bit); \
                break; \
            default: \
                break; \
        } \
        break; \

#define EXPAND_AS_SETSTARTINGMODE_FROMGROUP(_gpioname, _index, _port, _bit, _startingMode) \
    EXPAND_AS_SETSTARTINGMODE_FROMGROUP_INTERNAL(_index, _port, _bit, _startingMode) \

static I_GpioGroup_t instance;

static void SetState(I_GpioGroup_t *_instance, GpioChannel_t channel, GpioState_t state)
{
    IGNORE(_instance);

    switch(channel)
    {
        GPIO_TABLE(EXPAND_AS_SETSTATE_FROMGROUP);
    }
}

static GpioState_t GetState(I_GpioGroup_t *_instance, GpioChannel_t channel)
{
    IGNORE(_instance);

    GpioState_t readState;

    switch(channel)
    {
        GPIO_TABLE(EXPAND_AS_GETSTATE_FROMGROUP);
    }

    return readState;
}

static const GpioGroupApi_t api =
    { SetState, GetState };

I_GpioGroup_t * GpioGroup_MSP432_Init(void)
{
    uint8_t channel;

    for(channel = 0; channel < GpioMax; channel++)
    {
        switch(channel)
        {
            GPIO_TABLE(EXPAND_AS_SETSTARTINGMODE_FROMGROUP);
        }
    }

    for(channel = 0; channel < GpioMax; channel++)
    {
        SetState(&instance, channel, GpioState_Low);
    }

    instance.api = &api;
    return &instance;
}
