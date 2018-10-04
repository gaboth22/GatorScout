#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "I_UltraSonic.h"
#include "EventSubscriber_Synchronous.h"
#include "TimerPeriodic.h"

typedef struct
{
    I_UltraSonic_t interface;
    TimerPeriodic_t ultraSonicSamplingTimer;
    uint32_t rightUltraSonicDistance;
} UltraSonic_t;

void UltraSonic_Init(
     UltraSonic_t *instance,
     TimerModule_t *timerModule,
     TimerTicks_t ultraSonicSamplingRateMs
     );

#endif
