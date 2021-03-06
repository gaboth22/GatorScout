#include "Uart_Usca3.h"
#include "Event_Synchronous.h"
#include "types.h"
#include "utils.h"
#include "Uassert.h"
#include "msp.h"

typedef struct
{
    I_Uart_t interface;
    Event_Synchronous_t onByteReceived;
    uint8_t receivedByte;
} Uart_Usca3_t;

static Uart_Usca3_t instance;

static void SendByte(I_Uart_t *instance, uint8_t byte)
{
    // Make sure TX buffer is empty
    while(!(EUSCI_A3->IFG & EUSCI_A_IFG_TXIFG));
    EUSCI_A3->TXBUF = byte;
}

static I_Event_t * GetOnByteReceivedEvent(I_Uart_t *_instance)
{
    IGNORE(_instance);
    return &instance.onByteReceived.interface;
}

static void UpdateBaud(I_Uart_t *_instance, Baud_t baud)
{
    IGNORE(_instance);
    IGNORE(baud);

    Uassert(false);
}

static void DisableRx(I_Uart_t *_instance)
{
    IGNORE(_instance);
    EUSCI_A3->IE &= ~EUSCI_A_IE_RXIE;       // Disable USCI_A0 RX interrupt
}

static void EnableRx(I_Uart_t *_instance)
{
    IGNORE(_instance);
    EUSCI_A3->IE |= EUSCI_A_IE_RXIE;        // Enable USCI_A0 RX interrupt
}

static const UartApi_t api =
    { SendByte, GetOnByteReceivedEvent, UpdateBaud, DisableRx, EnableRx };

I_Uart_t * Uart_Usca3_Init(void)
{
    Event_Synchronous_Init(&instance.onByteReceived);
    instance.interface.api = &api;

    P9->SEL0 |= BIT6 | BIT7;  // Set pin as secondary function
    EUSCI_A3->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset
    EUSCI_A3->CTLW0 = EUSCI_A_CTLW0_SWRST | EUSCI_B_CTLW0_SSEL__SMCLK; // SMCLK as source
    // Default initialization to 921,600 baud
    EUSCI_A3->BRW = 3;
    // Fix up for 921.6K baud
    EUSCI_A3->MCTLW = (4 << EUSCI_A_MCTLW_BRF_OFS) | (0x04 << EUSCI_A_MCTLW_BRS_OFS) | EUSCI_A_MCTLW_OS16;
    EUSCI_A3->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // Initialize eUSCI
    EUSCI_A3->IFG &= ~EUSCI_A_IFG_RXIFG;    // Clear eUSCI RX interrupt flag
    EUSCI_A3->IE |= EUSCI_A_IE_RXIE;        // Enable USCI_A3 RX interrupt

    NVIC->ISER[0] |= 1 << ((EUSCIA3_IRQn) & 31);

    return &instance.interface;
}

void EUSCIA3_IRQHandler(void)
{
    if (EUSCI_A3->IFG & EUSCI_A_IFG_RXIFG)
    {
        instance.receivedByte = EUSCI_A3->RXBUF;
        Event_Publish(&instance.onByteReceived.interface, &instance.receivedByte);
    }
}
