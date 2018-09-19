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
#include "types.h"


#include "utils.h"

//static uint8_t versionId, productId;
//
//static void GetProductId(void *context, uint8_t byte)
//{
//    IGNORE(context);
//
//    productId = byte;
//}
//
//static void GetVersionId(void *context, uint8_t byte)
//{
//    RECAST(i2c, context, I_I2c_t *);
//    versionId = byte;
//
//    I2c_ReadByte(
//            i2c,
//            0x60,
//            0x0B,
//            GetProductId,
//            i2c);
//}
//
//static void StartModelRead(void *context)
//{
//    RECAST(i2c, context, I_I2c_t *);
//
//    I2c_ReadByte(
//            i2c,
//            0x60,
//            0x0A,
//            GetVersionId,
//            i2c);
//}

static void EncoderCallBack(void *context, void *args)
{
    IGNORE(args);
    RECAST(count, context, uint8_t *);

    (*count)++;
}

void main(void)
{
    StopWatchdog();
    SetClockTo48Mhz();
    StartGlobalPwmTick();

    uint8_t encoderCount = 0;

    I_Interrupt_t *oneMsInterrupt = Interrupt_1MsSystemTicks_Init();
    I_Interrupt_t *pinInterruptWheelEncoder = Interrupt_WheelEncoder_Init(GpioWheelEncoder1);
    I_TimeSource_t *oneMsTimeSource = TimeSource_1MsSystemTick_Init(oneMsInterrupt);
    TimerModule_t *timerModule = TimerModule_Init(oneMsTimeSource);
    I_GpioGroup_t *gpioGroup = GpioGroup_MSP432_Init();

    EventSubscriber_Synchronous_t encoderSubscriber;
    EventSubscriber_Synchronous_Init(&encoderSubscriber, EncoderCallBack, &encoderCount);
    Event_Subscribe(Interrupt_GetOnInterruptEvent(pinInterruptWheelEncoder), &encoderSubscriber.interface);

    Application_t application;
    Application_Init(&application, timerModule, gpioGroup);

    I_I2c_t *i2c = I2c_Uscb0_Init(timerModule);
    I_Spi_t *spi = Spi_Uscb1_Init(gpioGroup, GpioSpiCs);

    I_Pwm_t *pwm1 = Pwm_ConfigurableTA0CCR1_Init(GpioPwm1_P2B4);
    I_Pwm_t *pwm2 = Pwm_ConfigurableTA0CCR2_Init(GpioPwm2_P2B5);

    Pwm_SetDutyCycle(pwm1, 50);

    EnableInterrupts();

//    Camera_OV264I2cAndSpiCom_t camera;
//    Camera_OV264I2cAndSpi_Init(&camera, i2c, spi, GpioSpiCs, timerModule);

//    Camera_StartImageCapture(&camera.interface);

//    I2c_WriteByte(
//            i2c,
//            0x60,
//            0xFF,
//            0x01,
//            StartModelRead,
//            i2c);

    while(1)
    {
        TimerModule_Run(timerModule);
        Application_Run(&application);
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
