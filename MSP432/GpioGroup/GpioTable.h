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
    HOW(GpioPwm1_P2B4,          3,  2, 4, GpioMode_Output) \
    HOW(GpioPwm2_P2B5,          4,  2, 5, GpioMode_Output) \
    HOW(GpioPwm3_P2B6,          5,  2, 6, GpioMode_Output) \
    HOW(GpioPwm4_P2B7,          6,  2, 7, GpioMode_Output) \
    HOW(GpioWheelEncoder1,      7,  3, 2, GpioMode_Input)  \
    HOW(GpioWheelEncoder2,      8,  3, 3, GpioMode_Input)  \
    HOW(GpioSpiCs,              9,  6, 2, GpioMode_Output) \
    HOW(GpioSpiClk,             10, 6, 3, GpioMode_Output) \
    HOW(GpioSpiMosi,            11, 6, 4, GpioMode_Output) \
    HOW(GpioSpiMiso,            12, 6, 5, GpioMode_Input)  \


#define EXPAND_AS_ENUM(_gpioName, _index, _port, _bit, _initialMode) \
    _gpioName = _index, \

enum
{
    GPIO_TABLE(EXPAND_AS_ENUM)
    GpioMax
};

#endif
