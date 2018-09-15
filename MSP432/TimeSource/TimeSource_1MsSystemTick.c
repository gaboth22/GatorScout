#include "TimeSource_1MsSystemTick.h"
#include "I_Interrupt.h"
#include "utils.h"

typedef struct
{
    I_TimeSource_t interface;
    I_Interrupt_t *oneMsInterrupt;
} TimeSource_1MsSystemTick_t;

static TimeSource_1MsSystemTick_t timeSource;

static I_Event_t * GetOnTimePeriodEvent(I_TimeSource_t *instance)
{
    IGNORE(instance);
    return Interrupt_GetOnInterruptEvent(timeSource.oneMsInterrupt);
}

static const TimeSourceApi_t api =
    { GetOnTimePeriodEvent };

I_TimeSource_t * TimeSource_1MsSystemTick_Init(I_Interrupt_t *oneMsInterrupt)
{
    timeSource.oneMsInterrupt = oneMsInterrupt;
    timeSource.interface.api = &api;
    return &timeSource.interface;
}
