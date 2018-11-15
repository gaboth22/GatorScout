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
#include "LcdDisplayParallel2Line.h"
#include "LcdDisplayController.h"
#include "SensorDataController.h"
#include "ScoutingController.h"
#include "WayPointProvider_Unscouted.h"
#include "PathFinder_AStar.h"
#include "DistanceProviderCm.h"
#include "MotorControllerBusyChecker_WheelEncoders.h"
#include "MotorDriveCorrectionController.h"

TimerOneShot_t timer;
TimerModule_t *timerModule;
bool start;

static uint8_t image[5120] = { 0 };

static void StartImageCap(void *context)
{
    RECAST(scoutingController, context, ScoutingController_t *);
    start = true;
    ScoutingController_Start(scoutingController);
}

void main(void)
 {
    StopWatchdog();
    SetClockTo48Mhz();
    StartGlobalPwmTick();

    I_Interrupt_t *oneMsInterrupt = Interrupt_1MsSystemTicks_Init();
    I_TimeSource_t *oneMsTimeSource = TimeSource_1MsSystemTick_Init(oneMsInterrupt);
    timerModule = TimerModule_Init(oneMsTimeSource);
    I_GpioGroup_t *gpioGroup = GpioGroup_MSP432_Init();

    I_Uart_t *cameraUart = Uart_Usca0_Init(timerModule);
    I_Uart_t *wifiUart = Uart_Usca3_Init(timerModule);

    I_Interrupt_t *rightPinInterruptWheelEncoder =
        Interrupt_WheelEncoder_Init(GpioWheelEncoder1);
    I_Interrupt_t *leftPinInterruptWheelEncoder =
        Interrupt_WheelEncoder_Init(GpioWheelEncoder2);

    I_DmaController_t *dmaController = DmaController_MSP432_Init();

    I_Adc_t *adc14 = Adc_Precision14_Init();
    DistanceSensor_SharpGP2Y0A41SK0F_t frontDistSensor;
    DistanceSensor_SharpGP2Y0A41SK0F_Init(&frontDistSensor, adc14);

    UltrasonicSensorCommon_t *ultraSonicCommon = UltrasonicSensorCommon_Init(timerModule);

    DistanceSensor_UltraSonicHCSR01_t ultraSonicLeft;
    DistanceSensor_UltraSonicHCSR01_Init(&ultraSonicLeft, UltrasonicSensorChannel_Left, ultraSonicCommon);

    DistanceSensor_UltraSonicHCSR01_t ultraSonicRight;
    DistanceSensor_UltraSonicHCSR01_Init(&ultraSonicRight, UltrasonicSensorChannel_Right, ultraSonicCommon);

    PidController_t rightPid;
    PidController_Init(&rightPid, 1, 0, 0.0, 25, 60);

    PidController_t leftPid;
    PidController_Init(&leftPid, 1, 0, 0, 25, 60);

    Application_t application;
    Application_Init(&application, timerModule, gpioGroup);

    I_Pwm_t *leftFwd = Pwm_TA0CCR1_Init(GpioPwm1_P2B4);
    I_Pwm_t *rightBwd = Pwm_TA0CCR2_Init(GpioPwm2_P2B5);
    I_Pwm_t *leftBwd = Pwm_TA0CCR3_Init(GpioPwm3_P2B6);
    I_Pwm_t *rightFwd = Pwm_TA0CCR4_Init(GpioPwm4_P2B7);

    MotorControllerBusyChecker_WheelEncoders_t motorContorllerBusyChecker;
    MotorControllerBusyChecker_WheelEncoders_Init(
        &motorContorllerBusyChecker,
        Interrupt_GetOnInterruptEvent(rightPinInterruptWheelEncoder),
        Interrupt_GetOnInterruptEvent(leftPinInterruptWheelEncoder),
        timerModule);

    MotorDriveCorrectionController_t correctinController;
    MotorDriveCorrectionController_Init(
        &correctinController,
        &ultraSonicLeft.interface,
        &ultraSonicRight.interface);

    Camera_SpinelVC0706_t cam;
    Camera_SpinelVC076_Init(
        &cam,
        cameraUart,
        dmaController,
        DmaChannel_UartUsca0Rx,
        timerModule,
        (void *) UART_getReceiveBufferAddressForDMA(EUSCI_A0_BASE),
        image,
        CameraType_5MP);

    ImageForwardingController_t imgFwdController;
    ImageForwardingController_Init(
        &imgFwdController,
        &cam.interface,
        wifiUart,
        dmaController,
        DmaChannel_UartUsca3Tx,
        (void *) UART_getTransmitBufferAddressForDMA(EUSCI_A3_BASE),
        timerModule);

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
        60,
        &motorContorllerBusyChecker.interface,
        &correctinController);

    RemoteMotionController_t remoteMotionController;
    RemoteMotionController_Init(&remoteMotionController, &motorController.interface, wifiUart, 27, 90, 90, timerModule);

    MapSender_t mapSender;
    MapSender_Init(&mapSender, wifiUart, dmaController, DmaChannel_UartUsca3Tx, (void *) UART_getTransmitBufferAddressForDMA(EUSCI_A3_BASE), timerModule);

    LcdDisplayParallel2Line_t *lcdDisplay = LcdDisplayParallel2Line_Init(
        gpioGroup,
        GpioLcdD7,
        GpioLcdD6,
        GpioLcdD5,
        GpioLcdD4,
        GpioLcdEn,
        GpioLcdRw,
        GpioLcdRs,
        GpioLcdD7,
        timerModule);

    LcdDisplayController_t lcdDisplayController;
    LcdDisplayController_Init(&lcdDisplayController, &lcdDisplay->interface);

    DistanceProviderCm_t distanceProvider;
    DistanceProviderCm_Init(
        &distanceProvider,
        Interrupt_GetOnInterruptEvent(leftPinInterruptWheelEncoder),
        27,
        6);

    WayPointProvider_Unscouted_t waypointProvider;
    ScoutingController_t scoutingController;

    WayPointProvider_Unscouted_Init(
        &waypointProvider,
        &scoutingController.visitedAreasGrid,
        &scoutingController.blockedAreasGrid);

    PathFinder_AStar_t pathFinder;
    PathFinder_AStar_Init(&pathFinder);

    MapBuilder_t mapBuilder;
    MapBuilder_Init(
        &mapBuilder,
        &distanceProvider,
        &ultraSonicLeft.interface,
        &ultraSonicRight.interface);

    ScoutingController_Init(
        &scoutingController,
        &frontDistSensor.interface,
        &ultraSonicLeft.interface,
        &ultraSonicRight.interface,
        &motorController.interface,
        &distanceProvider,
        &waypointProvider.interface,
        &pathFinder.interface,
        32,
        32,
        &lcdDisplayController,
        &mapBuilder);

    CommunicationArbiter_t arbiter;
    CommunicationArbiter_Init(
        &arbiter,
        &remoteMotionController,
        &imgFwdController,
        &mapSender,
        &scoutingController.visitedAreasGrid,
        &scoutingController.blockedAreasGrid,
        timerModule);

    SensorDataController_t sensorDataDisplay;
    SensorDataController_Init(
        &sensorDataDisplay,
        &frontDistSensor.interface,
        &ultraSonicLeft.interface,
        &ultraSonicRight.interface,
        timerModule,
        &lcdDisplayController);

    start = false;

    TimerOneShot_Init(&timer, timerModule, 7000, StartImageCap, &scoutingController);
    TimerOneShot_Start(&timer);

    EnableInterrupts();

    while(1)
    {
        TimerModule_Run(timerModule);
        Application_Run(&application);
        MotorController_Run(&motorController.interface);

        if(start)
        {
            ScoutingController_Run(&scoutingController);
            CommunicationArbiter_Run(&arbiter);
            Camera_SpinelVC076_Run(&cam);
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
