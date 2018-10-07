#include "DistanceSensor_UltraSonicHCSR01.h"
#include "utils.h"

static DistanceInCm_t GetDistanceInCm(I_DistanceSensor_t *_instance)
{
    RECAST(instance, _instance, DistanceSensor_UltraSonicHCSR01_t *);

    return GetDistanceForChannel(instance->currentChannel);
}

static const DistanceSensorApi_t api =
    { GetDistanceInCm };

void DistanceSensor_UltraSonicHCSR01_Init(DistanceSensor_UltraSonicHCSR01_t *instance, UltrasonicSensorChannel_t currentChannel)
{
    instance->interface.api = &api;
    instance->currentChannel = currentChannel;
}
