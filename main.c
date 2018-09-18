#include "msp.h"
#include "Application.h"
#include "Assert.h"
#include "Camera_OV264I2cAndSpiCom.h"
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
    I_Spi_t *spi = Spi_Uscb1_Init(gpioGroup, GpioSpiCs);

    I_Pwm_t *pwm1 = Pwm_ConfigurableTA0CCR1_Init(GpioPwmForward1);
    I_Pwm_t *pwm2 = Pwm_ConfigurableTA0CCR2_Init(GpioPwmForward2);

    EnableInterrupts();

    Camera_OV264I2cAndSpiCom_t camera;
    Camera_OV264I2cAndSpi_Init(&camera, i2c, spi, GpioSpiCs, timerModule);

    Camera_StartImageCapture(&camera.interface);

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
