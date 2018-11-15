#include "math.h"
#include "DistanceSensor_SharpGP2Y0A41SK0F.h"
#include "utils.h"

__attribute__((ramfunc))
static DistanceInCm_t GetDistanceFromCounts(AdcCounts_t counts)
{
    // Equation obtained by plotting measured values
//    return (DistanceInCm_t)((-1*(14.76)*log(counts)) + 121.65);
    return (DistanceInCm_t)((-1*(10.83)*log(counts)) + 93.612);
}

static DistanceInCm_t GetDistanceInCm(I_DistanceSensor_t *_instance)
{
    RECAST(instance, _instance, DistanceSensor_SharpGP2Y0A41SK0F_t *);
    AdcCounts_t currentCounts = Adc_GetCounts(instance->adc);
    return GetDistanceFromCounts(currentCounts);
}

static const DistanceSensorApi_t api =
    { GetDistanceInCm };

void DistanceSensor_SharpGP2Y0A41SK0F_Init(DistanceSensor_SharpGP2Y0A41SK0F_t *instance, I_Adc_t *adc)
{
    instance->adc = adc;
    instance->interface.api = &api;
}
