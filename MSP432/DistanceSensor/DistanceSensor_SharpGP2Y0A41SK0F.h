#ifndef DISTANCESENSOR_SHARPGP2Y0A41SK0F_H
#define DISTANCESENSOR_SHARPGP2Y0A41SK0F_H

#include "I_DistanceSensor.h"
#include "I_Adc.h"

typedef struct
{
    I_DistanceSensor_t interface;
    I_Adc_t *adc;
} DistanceSensor_SharpGP2Y0A41SK0F_t;

void DistanceSensor_SharpGP2Y0A41SK0F_Init(DistanceSensor_SharpGP2Y0A41SK0F_t *instance, I_Adc_t *adc);

#endif
