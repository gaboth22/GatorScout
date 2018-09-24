#ifndef UART_H
#define UART_H

#include "msp.h"
#include "types.h"

#define EUSCI_A_CMSIS(x) ((EUSCI_A_Type *) x)

uint32_t UART_getReceiveBufferAddressForDMA(uint32_t moduleInstance);

#endif
