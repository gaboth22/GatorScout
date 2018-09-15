#ifndef APPLICATION_H
#define APPLICATION_H

#include "HeartbeatLed.h"

typedef struct
{
    HeartbeatLed_t hearbeatLed;
} Application_t;

/*
 * Code that needs to run every cycle, placed in main loop
 * * @param instance the application objects
 */
void Application_Run(Application_t *instance);

/*
 * Initialize application
 * @param instance the application objects
 * @param timerModule timer module instance
 * @param gpioGroup GPIO group instance
 */
void Application_Init(
    Application_t *instance,
    TimerModule_t *timerModule,
    I_GpioGroup_t *gpioGroup);

#endif
