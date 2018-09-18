#include "msp.h"
#include "Event_Synchronous.h"
#include "I_Spi.h"
#include "Spi_Uscb1.h"
#include "types.h"
#include "utils.h"

typedef struct
{
    I_Spi_t interface;
    I_GpioGroup_t *gpipGroup;
    GpioChannel_t csChannel;
    Event_Synchronous_t onBurstReceiveDone;
    SpiBurstReceiveInfo_t burstReceiveInfo;
    uint8_t receivedByte;
    bool received;
} Spi_Uscb1_t;

static Spi_Uscb1_t instance;

static void SendByte(I_Spi_t *_instance, uint8_t byteToSend)
{
    IGNORE(_instance);
    while (!(EUSCI_B1->IFG & EUSCI_B_IFG_TXIFG));
    EUSCI_B1->TXBUF = byteToSend;
}

uint8_t GetByte(I_Spi_t *_instance)
{
    IGNORE(_instance);
    instance.received = false;
    EUSCI_B1->IE |= EUSCI_B__TXIE;      // Enable TX interrupt
    while(!instance.received);
    return instance.receivedByte;
}

static bool IsBusy(I_Spi_t *instance)
{
    IGNORE(instance);
    return (bool)((EUSCI_B1->STATW & 0x01) == 0x01);
}

void StartBurstReceive(I_Spi_t *_instance, void *destination, uint16_t sizeOfBurst)
{
    GpioGroup_SetState(instance.gpipGroup, instance.csChannel, GpioState_Low);
    uint16_t i = 0;
    for(i = 0; i < sizeOfBurst; i++)
    {
        ((uint8_t *)destination)[i] = GetByte(&instance.interface);
    }
    while(IsBusy(&instance.interface));
    GpioGroup_SetState(instance.gpipGroup, instance.csChannel, GpioState_High);

    instance.burstReceiveInfo.dataSize = sizeOfBurst;
    instance.burstReceiveInfo.dataAddress = destination;

    Event_Publish(&instance.onBurstReceiveDone.interface, &instance.burstReceiveInfo);
}

I_Event_t * GetOnBurstReceiveDoneEvent(I_Spi_t *_instance)
{
    IGNORE(_instance);
    return &instance.onBurstReceiveDone.interface;
}

static void SetChipSelectState(I_Spi_t *_instance, ChipSelectState_t state)
{
    IGNORE(_instance);
    GpioGroup_SetState(instance.gpipGroup, instance.csChannel, (GpioState_t)state);
}

static const SpiApi_t api =
    { SendByte, GetByte, StartBurstReceive, GetOnBurstReceiveDoneEvent, IsBusy, SetChipSelectState };

I_Spi_t * Spi_Uscb1_Init(I_GpioGroup_t *gpioGroup, GpioChannel_t csChannel)
{
    instance.gpipGroup = gpioGroup;
    instance.csChannel = csChannel;
    instance.interface.api = &api;

    Event_Synchronous_Init(&instance.onBurstReceiveDone);

    P6->SEL0 |= BIT5 | BIT4 | BIT3 ;//| BIT2;  // P6.5 SOMI; P6.4 SIMO; P6.3 CLK
    EUSCI_B1->CTLW0 |= EUSCI_B_CTLW0_SWRST;

    EUSCI_B1->CTLW0 = EUSCI_B_CTLW0_SWRST | // Remain in reset state
        EUSCI_B_CTLW0_MST |    // SPI master
        EUSCI_B_CTLW0_SYNC |   // Synchronous mode
        //EUSCI_B_CTLW0_MODE_2 | // 4-pin mode
        EUSCI_B_CTLW0_MODE_0 | // 3-pin mode
        EUSCI_B_CTLW0_MSB |    // MSB first
        //EUSCI_B_CTLW0_STEM |   // Auto trigger CS
        EUSCI_B_CTLW0_SSEL__SMCLK; // SMCLK as source

    EUSCI_B1->BRW = 96; // SMCLK / 96 = 500 KHz
    EUSCI_B1->CTLW0 &= ~EUSCI_B_CTLW0_SWRST; // Enable module for operation

    NVIC->ISER[0] = 1 << ((EUSCIB1_IRQn) & 31);

    return &instance.interface;
}

void EUSCIB1_IRQHandler(void)
{
    if (EUSCI_B1->IFG & EUSCI_B_IFG_TXIFG)
    {
        EUSCI_B1->IE &= ~EUSCI_B__TXIE;
        EUSCI_B1->TXBUF = 0x00;

        while (!(EUSCI_B1->IFG & EUSCI_B_IFG_RXIFG));

        instance.receivedByte = EUSCI_B1->RXBUF;
        EUSCI_B1->IFG &= ~EUSCI_B_IFG_RXIFG;
        instance.received = true;
    }
}
