#include "msp.h"
#include "I_Spi.h"
#include "Spi_Uscb1.h"
#include "types.h"
#include "utils.h"

typedef struct
{
    I_Spi_t interface;
    bool sending;
    union
    {
        uint8_t receivedByte;
        uint8_t byteToSend;
    } bytes;
} Spi_Uscb1_t;

static Spi_Uscb1_t instance;

static void SendByte(I_Spi_t *_instance, uint8_t byteToSend)
{
    IGNORE(_instance);
    while (!(EUSCI_B1->IFG & EUSCI_B_IFG_TXIFG));
    instance.sending = true;
    instance.bytes.byteToSend = byteToSend;
    EUSCI_B1->IFG |= EUSCI_B_IFG_TXIFG; // Set TX flag
    EUSCI_B1->IE |= EUSCI_B__TXIE;      // Enable TX interrupt
    while(EUSCI_B1->STATW & UCBBUSY);
}

uint8_t GetByte(I_Spi_t *_instance)
{
    IGNORE(_instance);
    while (!(EUSCI_B1->IFG & EUSCI_B_IFG_TXIFG));
    instance.sending = false;
    EUSCI_B1->IFG |= EUSCI_B_IFG_TXIFG; // Set TX flag
    EUSCI_B1->IE |= EUSCI_B__TXIE;      // Enable TX interrupt
    while(EUSCI_B1->STATW & UCBBUSY);
    return instance.bytes.receivedByte;
}

void StartBurstReceive(I_Spi_t *instance, void *destination, uint16_t sizeOfBurst)
{
    IGNORE(instance);
    IGNORE(sizeOfBurst);
}

I_Event_t * GetOnBurstReceiveDoneEvent(I_Spi_t *instance)
{
    IGNORE(instance);
    return NULL;
}

static const SpiApi_t api =
    { SendByte, GetByte, StartBurstReceive, GetOnBurstReceiveDoneEvent };

I_Spi_t * Spi_Uscb1_Init(void)
{
    instance.interface.api = &api;

    P6->SEL0 |= BIT5 | BIT4 | BIT3 | BIT2;  // P6.5 SOMI; P6.4 SIMO; P6.3 CLK; P6.2 CS
    EUSCI_B1->CTLW0 |= EUSCI_B_CTLW0_SWRST;

    EUSCI_B1->CTLW0 = EUSCI_B_CTLW0_SWRST | // Remain in reset state
        EUSCI_B_CTLW0_MST |        // SPI master
        EUSCI_B_CTLW0_SYNC |       // Synchronous mode
        //EUSCI_B_CTLW0_CKPL |     // Set clock polarity high
        EUSCI_B_CTLW0_MODE_2 |     // 4-pin mode
        //EUSCI_B_CTLW0_MODE_0 |
        EUSCI_B_CTLW0_MSB |        // MSB first
        EUSCI_B_CTLW0_STEM |       // STE mode select
        EUSCI_B_CTLW0_SSEL__SMCLK; // SMCLK as source

    EUSCI_B1->BRW = 12; // SMCLK / 12 = 4 MHz
    EUSCI_B1->CTLW0 &= ~EUSCI_B_CTLW0_SWRST; // Enable module for operation

    NVIC->ISER[0] = 1 << ((EUSCIB1_IRQn) & 31);

    return &instance.interface;
}

void EUSCIB1_IRQHandler(void)
{
    if (EUSCI_B1->IFG & EUSCI_B_IFG_TXIFG)
    {
        EUSCI_B1->IE &= ~EUSCI_B__TXIE;// Disable TX interrupt

        if(instance.sending)
        {
            EUSCI_B1->TXBUF = instance.bytes.byteToSend; // Byte to shift out
        }
        else
        {
            while (!(EUSCI_B1->IFG & EUSCI_B_IFG_TXIFG));
            EUSCI_B1->TXBUF = 0x00; // Dummy data so we can receive
            while (!(EUSCI_B1->IFG & EUSCI_B_IFG_RXIFG)); // Wait till a character is received

            instance.bytes.receivedByte = EUSCI_B1->RXBUF;

            EUSCI_B1->IFG &= ~EUSCI_B_IFG_RXIFG; // Clear RX interrupt flag
        }
    }
}
