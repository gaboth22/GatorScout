#ifndef UART_USCA0_H
#define UART_USCA0_H

#include "I_Uart.h"
#include "TimerModule.h"

/*
 * Get the singleton UART instance
 */
I_Uart_t * Uart_Usca0_Init(TimerModule_t *timerModule);

#endif
