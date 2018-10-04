#include "msp.h"
#include "UltraSonic.h"
#include "utils.h"
#include "Event_Synchronous.h"

static uint32_t rightUltraSonicCount;
static uint32_t rightCount;
static uint32_t leftUltraSonicCount;
static uint32_t leftCount;

static uint32_t CalculateDistanceInCmFromInputCaptureCount(uint32_t inputCaptureCount)
{
    return (uint32_t)(((count)/2)*0.034);
}

static uint32_t GetUltraSonicDistanceInCm(I_UltraSonic_t *_instance)
{
    RECAST(instance, _instance, UltraSonic_t *);

    instance->rightUltraSonicDistance = CalculateDistanceInCmFromInputCaptureCount(count);

    return instance->rightUltraSonicDistance;
}

static const UltraSonicApi_t api =
    {GetUltraSonicDistanceInCm};

static void Start10msTimerToEnableTA1_0_UltraSonicTrigger(void *context)
{
    IGNORE(context);

    P7->OUT |= BIT1;
    TIMER_A1->R = 0;
    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
}

static void UltraSonic_Setup10usTimerA1_0_Interrupt()
{
    NVIC->ISER[0] = 1 << ((TA1_0_IRQn) & 31);

    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
    TIMER_A1->CCTL[0] = TIMER_A_CCTLN_CCIE; // TACCR0 interrupt enabled
    TIMER_A1->CCR[0] = 480-30;
    TIMER_A1->CTL = TIMER_A_CTL_TASSEL_2 | // SMCLK, UP mode
            TIMER_A_CTL_MC_1 | TIMER_A_CTL_IE;
}

static void UltraSonic_Setup1usTimerA2_0_Interrupt()
{
    NVIC->ISER[0] |= 1 << ((TA2_0_IRQn) & 31);

    TIMER_A2->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
    TIMER_A2->CCTL[0] |= TIMER_A_CCTLN_CCIE; // TACCR0 interrupt enable
    TIMER_A2->CCR[0] = 480-30; // 1 us
    TIMER_A2->CTL |= TIMER_A_CTL_TASSEL_2 | // ACLK
                     TIMER_A_CTL_MC_1 | TIMER_A_CTL_IE; // continues mode
}

static void UltraSonic_SetupP5InputCaptureEcho()
{
    NVIC->ISER[1] |= (1 << ((PORT5_IRQn) & 31));
    P5->SEL0 = 0;
    P5->SEL1 = 0;
    P5->IES &= ~BIT0;
    P5->IFG &= ~BIT0; //clear interrupt flags on p5.0
    P5->IE |= BIT0; // enable interrupt on p5.0
}

void UltraSonic_Init(
     UltraSonic_t *instance,
     TimerModule_t *timerModule,
     TimerTicks_t ultraSonicSamplingRateMs)
{
    rightUltraSonicCount = 0;

    count = 0;

    instance->rightUltraSonicDistance = 0;

    instance->interface.api = &api;

    UltraSonic_Setup10usTimerA1_0_Interrupt();

    UltraSonic_SetupP5InputCaptureEcho();

    UltraSonic_Setup1usTimerA2_0_Interrupt();

    TimerPeriodic_Init(
        &instance->ultraSonicSamplingTimer,
        timerModule,
        ultraSonicSamplingRateMs,
        Start10msTimerToEnableTA1_0_UltraSonicTrigger,
        instance);

    P7->DIR |= BIT2;
    P7->OUT |= BIT2;

    TimerPeriodic_Start(&instance->ultraSonicSamplingTimer);
}

void TA2_0_IRQHandler(void)
{
    TIMER_A2->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
    rightUltraSonicCount+=10;

    if((P5->IN & BIT0) != BIT0)
    {
        P7->OUT &= ~BIT2;
        TIMER_A2->R = 0; // Disable timer 2
        TIMER_A2->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
        TIMER_A2->CCTL[0] &= ~TIMER_A_CCTLN_CCIE; // TACCR0 interrupt enabled
        TIMER_A2->CTL &= ~TIMER_A_CTL_IE; // disable TimerA2_0
        count = rightUltraSonicCount;
        rightUltraSonicCount = 0;
    }
}

//Handles 10us trigger
void TA1_0_IRQHandler(void)
{
    P7->OUT &= ~BIT1;
    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
}

//Handles input capture for Echo pin
void PORT5_IRQHandler(void)
{
    if(P5->IFG & BIT0)
    {
        P5->IFG &= ~BIT0;
        P7->OUT |= BIT2;
        TIMER_A2->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
        TIMER_A2->CCTL[0] |= TIMER_A_CCTLN_CCIE; // TACCR0 interrupt enabled
        TIMER_A2->R = 0;
        TIMER_A2->CTL |= TIMER_A_CTL_IE; // enable TimerA2_0
    }
}
