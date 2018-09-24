#include "Uart_Usca0.h"
#include "Uassert.h"
#include "Event_Synchronous.h"
#include "types.h"
#include "utils.h"
#include "msp.h"

typedef struct
{
    I_Uart_t interface;
    Event_Synchronous_t onByteReceived;
    uint8_t receivedByte;
} Uart_Usca0_t;

static Uart_Usca0_t instance;

static void SendByte(I_Uart_t *instance, uint8_t byte)
{
    // Make sure the TX buffer is empty
    while(!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));
    EUSCI_A0->TXBUF = byte;
}

static I_Event_t * GetOnByteReceivedEvent(I_Uart_t *_instance)
{
    IGNORE(_instance);
    return &instance.onByteReceived.interface;
}

static void UpdateBaud(I_Uart_t *_instance, Baud_t baud)
{
    Uassert((baud == 115200));

    IGNORE(_instance);
    EUSCI_A0->IE &= ~(EUSCI_A_IE_RXIE);      // disable USCI_A0 RX interrupt
    EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset
    EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_SWRST | EUSCI_B_CTLW0_SSEL__SMCLK; // SMCLK as source
    EUSCI_A0->BRW = 26;
    EUSCI_A0->MCTLW = (0xD6 << EUSCI_A_MCTLW_BRS_OFS) | EUSCI_A_MCTLW_OS16; // Fix up for 115.2K baud
    EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // Initialize eUSCI
    EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;     // Clear eUSCI RX interrupt flag
    EUSCI_A0->IE |= EUSCI_A_IE_RXIE;         // Enable USCI_A0 RX interrupt
}

static void DisableRx(I_Uart_t *_instance)
{
    IGNORE(_instance);
    EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;    // Clear eUSCI RX interrupt flag
    EUSCI_A0->IE &= ~EUSCI_A_IE_RXIE;       // Disable USCI_A0 RX interrupt
}

static void EnableRx(I_Uart_t *_instance)
{
    IGNORE(_instance);
    EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;    // Clear eUSCI RX interrupt flag
    EUSCI_A0->IE |= EUSCI_A_IE_RXIE;        // Enable USCI_A0 RX interrupt
}

static const UartApi_t api =
    { SendByte, GetOnByteReceivedEvent, UpdateBaud, DisableRx, EnableRx };

I_Uart_t * Uart_Usca0_Init(void)
{
    Event_Synchronous_Init(&instance.onByteReceived);
    instance.interface.api = &api;

    P1->SEL0 |= BIT2 | BIT3;  // Set pin as secondary function
    EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset
    EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_SWRST | EUSCI_B_CTLW0_SSEL__SMCLK; // SMCLK as source
    // Default initialization to 38,400 baud
    EUSCI_A0->BRW = 78;
    EUSCI_A0->MCTLW = (2 << EUSCI_A_MCTLW_BRF_OFS) | EUSCI_A_MCTLW_OS16; // Fix up for 38.4K baud
    EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // Initialize eUSCI
    EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;    // Clear eUSCI RX interrupt flag
    EUSCI_A0->IE |= EUSCI_A_IE_RXIE;        // Enable USCI_A0 RX interrupt

    NVIC->ISER[0] = 1 << ((EUSCIA0_IRQn) & 31);

    return &instance.interface;
}

void EUSCIA0_IRQHandler(void)
{
    if (EUSCI_A0->IFG & EUSCI_A_IFG_RXIFG)
    {
        instance.receivedByte = EUSCI_A0->RXBUF;
        Event_Publish(&instance.onByteReceived.interface, &instance.receivedByte);
    }
}
