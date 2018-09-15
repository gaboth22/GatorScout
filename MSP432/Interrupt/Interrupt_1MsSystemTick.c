#include "msp.h"
#include "Interrupt_1MsSystemTick.h"
#include "I_Event.h"
#include "Event_Synchronous.h"
#include "utils.h"
#include "types.h"

enum
{
    ValueToCountUptoToGet1MsRunningAt48MHz = 48000
};

static I_Interrupt_t oneMsInterrupt;
static Event_Synchronous_t onOneMsInterrupt;

static I_Event_t * GetOnInterruptEvent(I_Interrupt_t *instance)
{
    IGNORE(instance);
    return &onOneMsInterrupt.interface;
}

static const InterruptApi_t api =
    { GetOnInterruptEvent };

I_Interrupt_t * Interrupt_1MsSystemTicks_Init(void)
{
    Event_Synchronous_Init(&onOneMsInterrupt);

    oneMsInterrupt.api = &api;
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    SysTick->LOAD = ValueToCountUptoToGet1MsRunningAt48MHz;
    SysTick->VAL = 0x01;
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

    return &oneMsInterrupt;
}

void SysTick_Handler(void)
{
    Event_Publish(&onOneMsInterrupt.interface, NULL);
}


