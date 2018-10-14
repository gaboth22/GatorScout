#ifndef ULTRASONICSENSORCOMMON_H
#define ULTRASONICSENSORCOMMON_H

#include "I_DistanceSensor.h"
#include "TimerModule.h"
#include "TimerPeriodic.h"

typedef uint16_t CountInTensOfMicroSeconds_t;

enum UltrasonicSensorChannel
{
    UltrasonicSensorChannel_Left = 0,
    UltrasonicSensorChannel_Right
};
typedef uint8_t UltrasonicSensorChannel_t;

typedef struct
{
    CountInTensOfMicroSeconds_t runningCount;
    CountInTensOfMicroSeconds_t leftChannelCount;
    CountInTensOfMicroSeconds_t rightChannelCount;
    UltrasonicSensorChannel_t currentChannel;
    TimerPeriodic_t doSensorReadingTimer;
} UltrasonicSensorCommon_t;

/*
 * Get singleton instance of Ultrasonic Sensor Common module
 */
UltrasonicSensorCommon_t * UltrasonicSensorCommon_Init(TimerModule_t *timerModule);

/*
 * Get the distance for an ultrasonic channel
 */
DistanceInCm_t UltrasonicSensorCommon_GetDistanceForChannel(UltrasonicSensorCommon_t *instance, UltrasonicSensorChannel_t channel);

#endif
