#include "msp.h"
#include "Application.h"
#include "GpioGroup_MSP432.h"
#include "GpioTable.h"
#include "HardwareUtils.h"
#include "HeartbeatLed.h"
#include "Interrupt_1MsSystemTick.h"
#include "TimeSource_1MsSystemTick.h"
#include "TimerModule.h"
#include "I2c_Uscb0.h"
#include "Spi_Uscb1.h"
#include "Pwm_ConfigurableTA0CCR1.h"
#include "Pwm_ConfigurableTA0CCR2.h"

void main(void)
{
    StopWatchdog();
    SetClockTo48Mhz();
    StartGlobalPwmTick();

    I_Interrupt_t *oneMsInterrupt = Interrupt_1MsSystemTicks_Init();
    I_TimeSource_t *oneMsTimeSource = TimeSource_1MsSystemTick_Init(oneMsInterrupt);
    TimerModule_t *timerModule = TimerModule_Init(oneMsTimeSource);
    I_GpioGroup_t *gpioGroup = GpioGroup_MSP432_Init();

    Application_t application;
    Application_Init(&application, timerModule, gpioGroup);

    I_I2c_t *i2c = I2c_Uscb0_Init(timerModule);
    I_Spi_t *spi = Spi_Uscb1_Init();

    I_Pwm_t *pwm1 = Pwm_ConfigurableTA0CCR1_Init(GpioPwmForward1);
    I_Pwm_t *pwm2 = Pwm_ConfigurableTA0CCR2_Init(GpioPwmForward2);

    EnableInterrupts();

    Pwm_SetDutyCycle(pwm1, 25);
    Pwm_SetDutyCycle(pwm2, 25);

    // Dummy delay to see the PWM signal before it changes
    uint64_t i = 0;
    for(i = 0; i < 10000000; i++);

    Pwm_ChangePortMap(pwm1, GpioPwmBackward1);
    Pwm_ChangePortMap(pwm2, GpioPwmBackward2);

    Pwm_SetDutyCycle(pwm1, 70);
    Pwm_SetDutyCycle(pwm2, 70);

    while(1)
    {
        TimerModule_Run(timerModule);
        Application_Run(&application);
    }
}
