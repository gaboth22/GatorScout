#ifndef GPIOTABLE_H
#define GPIOTABLE_H

typedef enum
{
    GpioMode_Input = 0,
    GpioMode_Output = 1
} GpioMode_t;

  //HOW(name, index in this table, port, bit, startingMode)
#define GPIO_TABLE(HOW) \
    HOW(GpioHeartbeatLed,       0,  1, 0, GpioMode_Output) \
    HOW(GpioUartUsca0Rx,        1,  1, 2, GpioMode_Input)  \
    HOW(GpioUartUsca0Tx,        2,  1, 3, GpioMode_Output) \
    HOW(GpioI2cSda,             3,  1, 6, GpioMode_Output) \
    HOW(GpioI2cScl,             4,  1, 7, GpioMode_Output) \
    HOW(GpioPwm1_P2B4,          5,  2, 4, GpioMode_Output) \
    HOW(GpioPwm2_P2B5,          6,  2, 5, GpioMode_Output) \
    HOW(GpioPwm3_P2B6,          7,  2, 6, GpioMode_Output) \
    HOW(GpioPwm4_P2B7,          8,  2, 7, GpioMode_Output) \
    HOW(GpioWheelEncoder1,      9,  3, 2, GpioMode_Input)  \
    HOW(GpioWheelEncoder2,      10, 3, 3, GpioMode_Input)  \
    HOW(GpioIrSensorInput,      11, 4, 7, GpioMode_Input)  \
    HOW(GpioUltraSonicEcho1,    12, 5, 0, GpioMode_Input)  \
    HOW(GpioUltraSonicEcho2,    13, 5, 1, GpioMode_Input)  \
    HOW(GpioSpiCs,              14, 6, 2, GpioMode_Output) \
    HOW(GpioSpiClk,             15, 6, 3, GpioMode_Output) \
    HOW(GpioSpiMosi,            16, 6, 4, GpioMode_Output) \
    HOW(GpioSpiMiso,            17, 6, 5, GpioMode_Input)  \
    HOW(GpioUltraSonicTrigger1, 18, 7, 1, GpioMode_Output) \
    HOW(GpioUltraSonicTrigger2, 19, 7, 2, GpioMode_Output) \
    HOW(GpioUartUsca3Rx,        20, 9, 6, GpioMode_Input)  \
    HOW(GpioUartUsca3Tx,        21, 9, 7, GpioMode_Output) \

#define EXPAND_AS_ENUM(_gpioName, _index, _port, _bit, _initialMode) \
    _gpioName = _index, \

enum
{
    GPIO_TABLE(EXPAND_AS_ENUM)
    GpioMax
};

#endif
