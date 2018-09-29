#ifndef UART_H
#define UART_H

#include "msp.h"
#include "types.h"

#define EUSCI_A_CMSIS(x) ((EUSCI_A_Type *) x)

#define EUSCI_B_CMSIS(x) ((EUSCI_B_Type *) x)

/*
 * Get RX buffer mem-mapped address to do DMA
 */
uint32_t UART_getReceiveBufferAddressForDMA(uint32_t moduleInstance);

/*
 * Get TX buffer mem-mapped address to do DMA
 */
uint32_t UART_getTransmitBufferAddressForDMA(uint32_t moduleInstance);

#endif
