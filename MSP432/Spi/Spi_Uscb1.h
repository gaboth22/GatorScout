#ifndef SPI_USCB1_H
#define SPI_USCB1_H

#include "I_Spi.h"
#include "I_GpioGroup.h"

/*
 * Get singleton instance of the SPI peripheral
 */
I_Spi_t * Spi_Uscb1_Init(I_GpioGroup_t *gpioGroup, GpioChannel_t csChannel);

#endif
