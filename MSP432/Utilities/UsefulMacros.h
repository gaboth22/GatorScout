#ifndef USEFULMACROS_H
#define USEFULMACROS_H

#define EXPAND_AS_GET_PORT_AND_PIN(_gpioName, _index, _port, _bit, _initialMode) \
    case _gpioName: \
        pinMask = BIT ## _bit; \
        pinNumber = _bit; \
        break; \

#endif
