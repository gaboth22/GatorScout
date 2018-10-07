#ifndef DISTANCESENSOR_ULTRASONICHCSR01_H
#define DISTANCESENSOR_ULTRASONICHCSR01_H

#include "I_DistanceSensor.h"
#include "UltrasonicSensorCommon.h"

typedef struct
{
    I_DistanceSensor_t interface;
    UltrasonicSensorChannel_t currentChannel;
} DistanceSensor_UltraSonicHCSR01_t;

void DistanceSensor_UltraSonicHCSR01_Init(DistanceSensor_UltraSonicHCSR01_t *instance, UltrasonicSensorChannel_t currentChannel);

#endif
