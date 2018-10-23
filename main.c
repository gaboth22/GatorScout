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
#include "CommunicationArbiter.h"
#include "uart.h"
#include "RemoteMotionController.h"
#include "Adc_Precision14.h"
#include "DistanceSensor_SharpGP2Y0A41SK0F.h"
#include "DistanceSensor_UltraSonicHCSR01.h"
#include "UltrasonicSensorCommon.h"

static bool start;

static void StartImageCap(void *context)
{
    IGNORE(context);
    start = true;
}

static uint8_t image[4096] = { 0 };

void main(void)
{
    StopWatchdog();
    SetClockTo48Mhz();
    StartGlobalPwmTick();

    I_Interrupt_t *oneMsInterrupt = Interrupt_1MsSystemTicks_Init();
    I_Interrupt_t *rightPinInterruptWheelEncoder = Interrupt_WheelEncoder_Init(GpioWheelEncoder1);
    I_Interrupt_t *leftPinInterruptWheelEncoder = Interrupt_WheelEncoder_Init(GpioWheelEncoder2);
    I_TimeSource_t *oneMsTimeSource = TimeSource_1MsSystemTick_Init(oneMsInterrupt);
    TimerModule_t *timerModule = TimerModule_Init(oneMsTimeSource);
    I_GpioGroup_t *gpioGroup = GpioGroup_MSP432_Init();


    PidController_t rightPid;
    PidController_Init(&rightPid, 1, 0, 0.0, 25, 60); //working

    PidController_t leftPid;
    PidController_Init(&leftPid, 1, 0, 0, 25, 60);

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
        &rightPid,
        timerModule,
        65);

    I_Uart_t *uart = Uart_Usca0_Init();
    I_DmaController_t *dma = DmaController_MSP432_Init();
    I_Uart_t *wifiUart = Uart_Usca3_Init();

    Camera_SpinelVC0706_t cam;
    Camera_SpinelVC076_Init(
            &cam,
            uart,
            dma,
            DmaChannel_UartUsca0Rx,
            timerModule,
            (void *) UART_getReceiveBufferAddressForDMA(EUSCI_A0_BASE),
            image);

    ImageForwardingController_t imgFwdController;
    ImageForwardingController_Init(
            &imgFwdController,
            &cam.interface,
            wifiUart,
            dma,
            DmaChannel_UartUsca3Tx,
            (void *) UART_getTransmitBufferAddressForDMA(EUSCI_A3_BASE));

    RemoteMotionController_t remoteMotionController;
    RemoteMotionController_Init(&remoteMotionController, &motorController, wifiUart, 2 * 75, 90, 90);

    CommunicationArbiter_t arbiter;
    CommunicationArbiter_Init(
        &arbiter,
        &remoteMotionController,
        &imgFwdController,
        timerModule);

    I_Adc_t *adc = Adc_Precision14_Init();
    DistanceSensor_SharpGP2Y0A41SK0F_t frontDistSensor;
    DistanceSensor_SharpGP2Y0A41SK0F_Init(&frontDistSensor, adc);

    UltrasonicSensorCommon_t *ultrasonicCommon = UltrasonicSensorCommon_Init(timerModule);

    DistanceSensor_UltraSonicHCSR01_t leftDistSensor;
    DistanceSensor_UltraSonicHCSR01_Init(&leftDistSensor, UltrasonicSensorChannel_Left, ultrasonicCommon);

    DistanceSensor_UltraSonicHCSR01_t rightDistSensor;
    DistanceSensor_UltraSonicHCSR01_Init(&rightDistSensor, UltrasonicSensorChannel_Right, ultrasonicCommon);

    start = false;

    TimerOneShot_t timer;
    TimerOneShot_Init(&timer, timerModule, 8000, StartImageCap, &cam);
    TimerOneShot_Start(&timer);

    EnableInterrupts();

    while(1)
    {
        TimerModule_Run(timerModule);
        Application_Run(&application);
        MotorController_Run(&motorController);
        DistanceInCm_t frontDistance = DistanceSensor_GetDistanceInCm(&frontDistSensor.interface);
        DistanceInCm_t leftDistance = DistanceSensor_GetDistanceInCm(&leftDistSensor.interface);
        DistanceInCm_t rightDistance = DistanceSensor_GetDistanceInCm(&rightDistSensor.interface);

        if(start)
        {
          Camera_SpinelVC076_Run(&cam);
          ImageForwardingController_Run(&imgFwdController);
          RemoteMotionController_Run(&remoteMotionController);
          CommunicationArbiter_Run(&arbiter);
        }
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
