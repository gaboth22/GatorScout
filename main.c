#include "msp.h"
#include "Application.h"
#include "Uassert.h"
#include "EventSubscriber_Synchronous.h"
#include "GpioGroup_MSP432.h"
#include "GpioTable.h"
#include "HardwareUtils.h"
#include "HeartbeatLed.h"
#include "Interrupt_1MsSystemTick.h"
#include "Interrupt_WheelEncoder.h"
#include "MotorController.h"
#include "TimeSource_1MsSystemTick.h"
#include "TimerModule.h"
#include "Pwm_TA0CCR1.h"
#include "Pwm_TA0CCR2.h"
#include "Pwm_TA0CCR3.h"
#include "Pwm_TA0CCR4.h"
#include "PidController.h"
#include "types.h"
#include "utils.h"
#include "Camera_SpinelVC0706.h"
#include "Uart_Usca0.h"
#include "Uart_Usca3.h"
#include "DmaController_MSP432.h"
#include "ImageForwardingController.h"
#include "uart.h"

void main(void)
{
    StopWatchdog();
    SetClockTo48Mhz();
    StartGlobalPwmTick();

    I_Interrupt_t *oneMsInterrupt = Interrupt_1MsSystemTicks_Init();
    I_Interrupt_t *rightPinInterruptWheelEncoder = Interrupt_WheelEncoder_Init(GpioWheelEncoder2);
    I_Interrupt_t *leftPinInterruptWheelEncoder = Interrupt_WheelEncoder_Init(GpioWheelEncoder1);
    I_TimeSource_t *oneMsTimeSource = TimeSource_1MsSystemTick_Init(oneMsInterrupt);
    TimerModule_t *timerModule = TimerModule_Init(oneMsTimeSource);
    I_GpioGroup_t *gpioGroup = GpioGroup_MSP432_Init();

    PidController_t rightPid;
    PidController_Init(&rightPid, 1, 0, 0.0, 30, 100); //working

    PidController_t leftPid;
    PidController_Init(&leftPid, 1, 0, 0, 30, 100);

    Application_t application;
    Application_Init(&application, timerModule, gpioGroup);

    I_Pwm_t *leftFwd = Pwm_TA0CCR1_Init(GpioPwm1_P2B4);
    I_Pwm_t *rightBwd = Pwm_TA0CCR2_Init(GpioPwm2_P2B5);
    I_Pwm_t *leftBwd = Pwm_TA0CCR3_Init(GpioPwm3_P2B6);
    I_Pwm_t *rightFwd = Pwm_TA0CCR4_Init(GpioPwm4_P2B7);

    MotorController_t motorController;
    MotorController_Init(
        &motorController,
        Interrupt_GetOnInterruptEvent(leftPinInterruptWheelEncoder),
        Interrupt_GetOnInterruptEvent(rightPinInterruptWheelEncoder),
        leftFwd,
        leftBwd,
        rightFwd,
        rightBwd,
        &leftPid,
        &rightPid);

    EnableInterrupts();

//    MotorController_Forward(&motorController, 75*2);
    MotorController_TurnRight(&motorController, 100);
//    MotorController_TurnLeft(&motorController, 90);

    while(1)
    {
        TimerModule_Run(timerModule);
        Application_Run(&application);
        MotorController_Run(&motorController);
    }
}

void Uassert(bool condition)
{
    if(!condition)
    {
        while(1)
        {
            __no_operation();
        }
    }
}
