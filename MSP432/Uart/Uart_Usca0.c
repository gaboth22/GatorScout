#include "Uart_Usca0.h"
#include "Uassert.h"
#include "Event_Synchronous.h"
#include "types.h"
#include "utils.h"
#include "msp.h"
#include "HardwareUtils.h"
#include "TimerPeriodic.h"
#include "Queue.h"

enum
{
    MaxUart0IncomingBuffSize = 256,
    PeriodToPubishReceivedByteMs = 1
};

typedef struct
{
    I_Uart_t interface;
    Event_Synchronous_t onByteReceived;
    bool acquired;
    uint8_t receiveQueueBuffer[MaxUart0IncomingBuffSize];
    Queue_t receiveQueue;
    TimerPeriodic_t publishReceivedByteTimer;
    uint8_t receivedByte;
} Uart_Usca0_t;

static Uart_Usca0_t instance;

static void SendByte(I_Uart_t *instance, uint8_t byte)
{
    // Make sure TX buffer is empty
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
    EUSCI_A0->IE &= ~(EUSCI_A_IE_RXIE);     // disable USCI_A0 RX interrupt
    EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset
    EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_SWRST | EUSCI_B_CTLW0_SSEL__SMCLK; // SMCLK as source
    EUSCI_A0->BRW = 26;
    EUSCI_A0->MCTLW = (0xD6 << EUSCI_A_MCTLW_BRS_OFS) | EUSCI_A_MCTLW_OS16; // Fix up for 115.2K baud
    EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // Initialize eUSCI
    EUSCI_A0->IE |= EUSCI_A_IE_RXIE;         // Enable USCI_A0 RX interrupt
}

static void DisableRx(I_Uart_t *_instance)
{
    IGNORE(_instance);
    EUSCI_A0->IE &= ~EUSCI_A_IE_RXIE; // Disable USCI_A0 RX interrupt
}

static void EnableRx(I_Uart_t *_instance)
{
    IGNORE(_instance);
    EUSCI_A0->IE |= EUSCI_A_IE_RXIE; // Enable USCI_A0 RX interrupt
}

static bool Acquire(I_Uart_t *_instance)
{
    IGNORE(_instance);
    DisableInterrupts();

    bool gotIt = false;

    if(!instance.acquired)
    {
        gotIt = true;
        instance.acquired = true;
    }

    EnableInterrupts();
    return gotIt;
}

static void Release(I_Uart_t *_instance)
{
    IGNORE(_instance);
    instance.acquired = false;
}

static void PublishReceivedByte(void *context)
{
    IGNORE(context);
    uint32_t interruptState;
    interruptState = __get_PRIMASK();
    __disable_irq();
    if(Queue_Size(&instance.receiveQueue) > 0)
    {
        Queue_Pop(&instance.receiveQueue, &instance.receivedByte);
        Event_Publish(&instance.onByteReceived.interface, &instance.receivedByte);
    }
    __set_PRIMASK(interruptState);
}

static const UartApi_t api =
    { SendByte, GetOnByteReceivedEvent, UpdateBaud, DisableRx, EnableRx, Acquire, Release };

I_Uart_t * Uart_Usca0_Init(TimerModule_t *timerModule)
{
    Queue_Init(&instance.receiveQueue, &instance.receiveQueueBuffer[0], MaxUart0IncomingBuffSize, sizeof(uint8_t));
    Event_Synchronous_Init(&instance.onByteReceived);
    instance.interface.api = &api;

    instance.acquired = false;

    P1->SEL0 |= BIT2 | BIT3;  // Set pin as secondary function
    EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset
    EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_SWRST | EUSCI_B_CTLW0_SSEL__SMCLK; // SMCLK as source
    // Default initialization to 38,400 baud
    EUSCI_A0->BRW = 78;
    EUSCI_A0->MCTLW = (2 << EUSCI_A_MCTLW_BRF_OFS) | EUSCI_A_MCTLW_OS16; // Fix up for 38.4K baud
    EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // Initialize eUSCI
    EUSCI_A0->IE |= EUSCI_A_IE_RXIE;        // Enable USCI_A0 RX interrupt

    TimerPeriodic_Init(
        &instance.publishReceivedByteTimer,
        timerModule,
        PeriodToPubishReceivedByteMs,
        PublishReceivedByte,
        NULL);

    TimerPeriodic_Start(&instance.publishReceivedByteTimer);

    NVIC->ISER[0] |= 1 << ((EUSCIA0_IRQn) & 31);

    return &instance.interface;
}

void EUSCIA0_IRQHandler(void)
{
    if (EUSCI_A0->IFG & EUSCI_A_IFG_RXIFG)
    {
        uint8_t inByte = EUSCI_A0->RXBUF;
        Queue_Push(&instance.receiveQueue, &inByte);
    }
}
