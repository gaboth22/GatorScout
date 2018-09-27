#include "Interrupt_WheelEncoder.h"
#include "msp.h"
#include "I_Event.h"
#include "Event_Synchronous.h"
#include "utils.h"
#include "types.h"
#include "Assert.h"

static I_Interrupt_t pinInterruptWheelEncoder1;
static Event_Synchronous_t onPinInterruptWheelEncoder1;
static I_Interrupt_t pinInterruptWheelEncoder2;
static Event_Synchronous_t onPinInterruptWheelEncoder2;

static I_Event_t * GetOnInterruptEvent1(I_Interrupt_t *instance)
{
    IGNORE(instance);
    return &onPinInterruptWheelEncoder1.interface;
}

static I_Event_t * GetOnInterruptEvent2(I_Interrupt_t *instance)
{
    IGNORE(instance);
    return &onPinInterruptWheelEncoder2.interface;
}

static const InterruptApi_t api1 =
    { GetOnInterruptEvent1 };

static const InterruptApi_t api2 =
    { GetOnInterruptEvent2 };

I_Interrupt_t * Interrupt_WheelEncoder_Init(GpioChannel_t gpioChannel)
{
    NVIC->ISER[1] |= (1 << ((PORT3_IRQn) & 31));

    P3->SEL0  = 0;
    P3->SEL1 = 0;

    if(gpioChannel == GpioWheelEncoder1)
    {
        Event_Synchronous_Init(&onPinInterruptWheelEncoder1);

        pinInterruptWheelEncoder1.api = &api1;

//        P3->REN |= BIT2; // Enable pull down resistor
//        P3->REN &= ~BIT2; // disable pull down or pull up resistor
//        P3->IES |= BIT2; // Selects interrupt edge for I/O pin (high to low transition)
//        P3->IES &= ~BIT2; // Selects interrupt edge for I/O pin (low to high)
        P3->IFG &= ~BIT2;    // Clear interrupt flags
        P3->IE |= BIT2;  // Enable interrupt on p3.2 (enable on main?)

        return &pinInterruptWheelEncoder1;
    }
    else if(gpioChannel == GpioWheelEncoder2)
    {
        Event_Synchronous_Init(&onPinInterruptWheelEncoder2);

        pinInterruptWheelEncoder2.api = &api2;

//        P3->REN |= BIT3; // Enable pull down resistor
//        P3->REN &= ~BIT3; // disable pull down or pull down resistor
//        P3->IES |= BIT3; // Selects interrupt edge for I/O pin (high to low transition)
//        P3->IES &= ~BIT3; // Selects interrupt edge for I/O pin (low to high )
        P3->IFG &= ~BIT3;    // Clear interrupt flags
        P3->IE |= BIT3;  // Enable interrupt on p3.2 (enable on main?)

        return &pinInterruptWheelEncoder2;
    }
    else
    {
        Assert(false);
    }

    return NULL;
}

void PORT3_IRQHandler(void)
{
    if(P3->IFG & BIT2)
    {
        Event_Publish(&onPinInterruptWheelEncoder1.interface, NULL);
        P3->IFG &= ~BIT2;
    }
    if(P3->IFG & BIT3)
    {
        Event_Publish(&onPinInterruptWheelEncoder2.interface, NULL);
        P3->IFG &= ~BIT3;
    }
}
