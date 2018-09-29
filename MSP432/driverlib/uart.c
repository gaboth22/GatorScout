#include "uart.h"

uint32_t UART_getReceiveBufferAddressForDMA(uint32_t moduleInstance)
{
    return (uint32_t)&EUSCI_A_CMSIS(moduleInstance)->RXBUF;
}

uint32_t UART_getTransmitBufferAddressForDMA(uint32_t moduleInstance)
{
    return (uint32_t)&EUSCI_B_CMSIS(moduleInstance)->TXBUF;
}
