#include "msp.h"
#include "Application.h"
#include "Assert.h"
#include "Camera_OV264I2cAndSpiCom.h"
#include "EventSubscriber_Synchronous.h"
#include "GpioGroup_MSP432.h"
#include "GpioTable.h"
#include "HardwareUtils.h"
#include "HeartbeatLed.h"
#include "Interrupt_1MsSystemTick.h"
#include "Interrupt_WheelEncoder.h"
#include "TimeSource_1MsSystemTick.h"
#include "TimerModule.h"
#include "I2c_Uscb0.h"
#include "Spi_Uscb1.h"
#include "Pwm_ConfigurableTA0CCR1.h"
#include "Pwm_ConfigurableTA0CCR2.h"
#include "PidController.h"
#include "types.h"
#include "utils.h"

static void EncoderCallBack(void *context, void *args)
{
    IGNORE(args);
    RECAST(count, context, uint64_t *);

    (*count)++;
}

static void EncoderCallBack2(void *context, void *args)
{
    IGNORE(args);
    RECAST(count, context, uint64_t *);

    (*count)++;
}

static bool test, test2;

void main(void)
{
    StopWatchdog();
    SetClockTo48Mhz();
    StartGlobalPwmTick();

    uint64_t encoderCount = 0;
    uint64_t encoderCount2 = 0;

    I_Interrupt_t *oneMsInterrupt = Interrupt_1MsSystemTicks_Init();
    I_Interrupt_t *pinInterruptWheelEncoder = Interrupt_WheelEncoder_Init(GpioWheelEncoder1);
    I_Interrupt_t *pinInterruptWheelEncoder2 = Interrupt_WheelEncoder_Init(GpioWheelEncoder2);
    I_TimeSource_t *oneMsTimeSource = TimeSource_1MsSystemTick_Init(oneMsInterrupt);
    TimerModule_t *timerModule = TimerModule_Init(oneMsTimeSource);
    I_GpioGroup_t *gpioGroup = GpioGroup_MSP432_Init();

    EventSubscriber_Synchronous_t encoderSubscriber;
    EventSubscriber_Synchronous_Init(&encoderSubscriber, EncoderCallBack, &encoderCount);
    Event_Subscribe(Interrupt_GetOnInterruptEvent(pinInterruptWheelEncoder), &encoderSubscriber.interface);

    EventSubscriber_Synchronous_t encoderSubscriber2;
    EventSubscriber_Synchronous_Init(&encoderSubscriber2, EncoderCallBack2, &encoderCount2);
    Event_Subscribe(Interrupt_GetOnInterruptEvent(pinInterruptWheelEncoder2), &encoderSubscriber2.interface);

    PidController_t pid;
    //PidController_Init(&pid, 0.0975, 0.00000003, 0.007, 30, 100);
    PidController_Init(&pid, 0.0999, 0.00000001889, 0.006, 33, 100);

    PidController_t pid2;
    //PidController_Init(&pid2, 0.0975, 0.00000003, 0.007, 30, 100);
    PidController_Init(&pid2, 0.097, 0.00000001889, 0.006, 30, 100);

    Application_t application;
    Application_Init(&application, timerModule, gpioGroup);

    //I_I2c_t *i2c = I2c_Uscb0_Init(timerModule);
    //I_Spi_t *spi = Spi_Uscb1_Init(gpioGroup, GpioSpiCs);

    I_Pwm_t *pwm1 = Pwm_ConfigurableTA0CCR1_Init(GpioPwm1_P2B4);
    I_Pwm_t *pwm2 = Pwm_ConfigurableTA0CCR2_Init(GpioPwm4_P2B7);

    EnableInterrupts();

//    Pwm_SetDutyCycle(pwm1, (uint8_t)100);

    test = true;
    test2 = true;
    uint64_t turns = 7;

    while(1)
    {
        TimerModule_Run(timerModule);
        Application_Run(&application);

        if(test)
        {
            uint64_t reading = PidController_Run(&pid, encoderCount, 74 * turns);
            Pwm_SetDutyCycle(pwm1, (uint8_t)reading);
        }

        if(test2)
        {
            uint64_t reading = PidController_Run(&pid2, encoderCount, 74 * turns);
            Pwm_SetDutyCycle(pwm2, (uint8_t)reading);
        }

        if(PidController_GoalAchieved(&pid))
        {
            test = false;
        }

        if(PidController_GoalAchieved(&pid2))
        {
            test2 = false;
        }
    }
}

void Assert(bool condition)
{
    if(!condition)
    {
        while(1)
        {
            __no_operation();
        }
    }
}
