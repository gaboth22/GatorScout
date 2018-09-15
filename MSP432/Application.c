#include "Application.h"
#include "GpioTable.h"
#include "utils.h"

enum
{
    HeartbearLedTogglePeriodMs = 500
};

void Application_Run(Application_t *instance)
{
    IGNORE(instance);
}

void Application_Init(
    Application_t *instance,
    TimerModule_t *timerModule,
    I_GpioGroup_t *gpioGroup)
{
    HeartbeatLed_Init(
        &instance->hearbeatLed,
        timerModule,
        GpioHeartbeatLed,
        gpioGroup,
        HeartbearLedTogglePeriodMs);
}
