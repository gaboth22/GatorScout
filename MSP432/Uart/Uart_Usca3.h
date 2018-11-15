#ifndef UART_USCA3_H
#define UART_USCA3_H

#include "I_Uart.h"
#include "TimerModule.h"

/*
 * Get singleton instance of the UART
 */
I_Uart_t * Uart_Usca3_Init(TimerModule_t *timerModule);

#endif
