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
    HOW(GpioI2cSda,             1,  1, 6, GpioMode_Output) \
    HOW(GpioI2cScl,             2,  1, 7, GpioMode_Output) \
    HOW(GpioPwmForward1,        3,  2, 4, GpioMode_Output) \
    HOW(GpioPwmForward2,        4,  2, 5, GpioMode_Output) \
    HOW(GpioPwmBackward1,       5,  2, 6, GpioMode_Output) \
    HOW(GpioPwmBackward2,       6,  2, 7, GpioMode_Output) \
    HOW(GpioSpiCs,              7,  6, 2, GpioMode_Output) \
    HOW(GpioSpiClk,             8,  6, 3, GpioMode_Output) \
    HOW(GpioSpiMosi,            9,  6, 4, GpioMode_Output) \
    HOW(GpioSpiMiso,            10, 6, 5, GpioMode_Input)  \

#define EXPAND_AS_ENUM(_gpioName, _index, _port, _bit, _initialMode) \
    _gpioName = _index, \

enum
{
    GPIO_TABLE(EXPAND_AS_ENUM)
    GpioMax
};

#endif
