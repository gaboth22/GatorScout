#include "UltrasonicSensorCommon.h"
#include "types.h"
#include "utils.h"
#include "msp.h"

#define SetupTimerA10() \
    NVIC->ISER[0] = 1 << ((TA1_0_IRQn) & 31); \
    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG; \
    TIMER_A1->CCTL[0] = TIMER_A_CCTLN_CCIE; \
    TIMER_A1->CCR[0] = 450; \
    TIMER_A1->CTL = TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_1; \

#define SetupTimerA20() \
    NVIC->ISER[0] |= 1 << ((TA2_0_IRQn) & 31); \
    TIMER_A2->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG; \
    TIMER_A2->CCTL[0] |= TIMER_A_CCTLN_CCIE; \
    TIMER_A2->CCR[0] = 450; \
    TIMER_A2->CTL |= TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_1; \

#define SetupP5B0And1ForEchoInput() \
    NVIC->ISER[1] |= (1 << ((PORT5_IRQn) & 31)); \
    P5->SEL0 = 0; \
    P5->SEL1 = 0; \
    P5->IES &= ~BIT0; \
    P5->IES &= ~BIT1; \
    P5->IFG &= ~BIT0; \
    P5->IFG &= ~BIT1; \

#define EnableP5B0RisignEdgeInterrupt() \
    P5->IE |= BIT0; \

#define EnableP5B1RisingEdgeInterrupt() \
    P5->IE |= BIT1; \

#define DisableP5B0RisingEdgeInterrupt() \
    P5->IE &= ~BIT0; \

#define DisableP5B1RisingEdgeInterrupt() \
    P5->IE &= ~BIT1; \

#define WriteTriggerP7B1High() \
    P7->OUT |= BIT1; \

#define WriteTriggerP7B1Low() \
    P7->OUT &= ~BIT1; \

#define WriteTriggerP7B2High() \
    P7->OUT |= BIT2; \

#define WriteTriggerP7B2Low() \
    P7->OUT &= ~BIT2; \

#define StartTimeA10Count() \
    TIMER_A1->R = 0; \
    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG; \
    TIMER_A1->CTL |= TIMER_A_CTL_IE; \

#define StopTimeA10Count() \
    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG; \
    TIMER_A1->CTL &= ~TIMER_A_CTL_IE; \

#define StartTimeA20() \
    TIMER_A2->R = 0; \
    TIMER_A2->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG; \
    TIMER_A2->CTL |= TIMER_A_CTL_IE; \

#define ClearTimerA20InterruptFlag() \
    TIMER_A2->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG; \

#define StopTimerA20() \
    TIMER_A2->CTL &= ~TIMER_A_CTL_IE; \

#define IsP5B0RisingEdge() \
    P5->IFG & BIT0 \

#define IsP5B0FallingEdge() \
    (P5->IN & BIT0) != BIT0 \

#define IsP5B1RisingEdge() \
    P5->IFG & BIT1 \

#define IsP5B1FallingEdge() \
    (P5->IN & BIT1) != BIT1 \

#define ClearP5B0InterruptFlag() \
    P5->IFG &= ~BIT0; \

#define ClearP5B1InterruptFlag() \
    P5->IFG &= ~BIT1; \

static UltrasonicSensorCommon_t instance;

enum
{
    PeriodToDoUltrasonicCycleMs = 10
};

static void StartUltrasonicCycle(void *context)
{
    IGNORE(context);
    switch(instance.currentChannel)
    {
        case UltrasonicSensorChannel_Right:
            EnableP5B0RisignEdgeInterrupt();
            WriteTriggerP7B1High();
            StartTimeA10Count();
            break;

        case UltrasonicSensorChannel_Left:
            EnableP5B1RisingEdgeInterrupt();
            WriteTriggerP7B2High();
            StartTimeA10Count();
            break;

        default:
            break;
    }
}

UltrasonicSensorCommon_t * UltrasonicSensorCommon_Init(TimerModule_t *timerModule)
{
    instance.currentChannel = UltrasonicSensorChannel_Right;

    SetupTimerA10();
    SetupTimerA20();
    SetupP5B0And1ForEchoInput();

    TimerPeriodic_Init(
        &instance.doSensorReadingTimer,
        timerModule,
        PeriodToDoUltrasonicCycleMs,
        StartUltrasonicCycle,
        NULL);

    TimerPeriodic_Start(&instance.doSensorReadingTimer);

    return &instance;
}

__attribute__((ramfunc))
DistanceInCm_t UltrasonicSensorCommon_GetDistanceForChannel(UltrasonicSensorCommon_t *_instance, UltrasonicSensorChannel_t channel)
{
    IGNORE(_instance);
    DistanceInCm_t toReturn = 0;

    switch(channel)
    {
        case UltrasonicSensorChannel_Right:
            toReturn = (DistanceInCm_t)(((instance.rightChannelCount) / 2) * 0.034);
            break;
        case UltrasonicSensorChannel_Left:
            toReturn = (DistanceInCm_t)(((instance.leftChannelCount) / 2) * 0.034);
            break;
        default:
            break;
    }

    return toReturn;
}

void TA1_0_IRQHandler(void)
{
    StopTimeA10Count();

    switch(instance.currentChannel)
    {
        case UltrasonicSensorChannel_Right:
            WriteTriggerP7B1Low();
            break;

        case UltrasonicSensorChannel_Left:
            WriteTriggerP7B2Low();
            break;

        default:
            break;
    }
}

void PORT5_IRQHandler(void)
{
    switch(instance.currentChannel)
    {
        case UltrasonicSensorChannel_Right:
            if(IsP5B0RisingEdge())
            {
                ClearP5B0InterruptFlag();
                DisableP5B0RisingEdgeInterrupt();
                StartTimeA20();
            }
            break;

        case UltrasonicSensorChannel_Left:
            if(IsP5B1RisingEdge())
            {
                ClearP5B1InterruptFlag();
                DisableP5B1RisingEdgeInterrupt();
                StartTimeA20();
            }
            break;

        default:
            break;
    }
}

void TA2_0_IRQHandler(void)
{
    ClearTimerA20InterruptFlag();

    instance.runningCount += 10;

    switch(instance.currentChannel)
    {
        case UltrasonicSensorChannel_Right:
            {
                if(IsP5B0FallingEdge())
                {
                    StopTimerA20();
                    if(instance.runningCount > 10)
                    {
                        instance.rightChannelCount = instance.runningCount;
                    }
                    instance.runningCount = 0;
                    instance.currentChannel = UltrasonicSensorChannel_Left;
                }
            }
            break;

        case UltrasonicSensorChannel_Left:
            {
                if(IsP5B1FallingEdge())
                {
                    StopTimerA20();
                    if(instance.runningCount > 10)
                    {
                        instance.leftChannelCount = instance.runningCount;
                    }
                    instance.runningCount = 0;
                    instance.currentChannel = UltrasonicSensorChannel_Right;
                }
            }
            break;

        default:
            break;
    }
}
