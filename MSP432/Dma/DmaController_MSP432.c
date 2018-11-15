#include "DmaController_MSP432.h"
#include "Event_Synchronous.h"
#include "dma.h"
#include "uart.h"
#include "interrupt.h"
#include "utils.h"
#include "uAssert.h"

enum
{
    MaxDmaTrxSize = 1024
};

typedef struct
{
    volatile void *srcEndAddr;
    volatile void *dstEndAddr;
    volatile uint32_t control;
    volatile uint32_t spare;
} DmaControlTable_t;

#pragma DATA_ALIGN(MSP432ControlTable, 1024)
static DmaControlTable_t MSP432ControlTable[1024];

typedef struct
{
    I_DmaController_t interface;

    /* UART A0 RX */
    void *uartUsca0RxChannelSrc;
    void *uartUsca0RxChannelDst;
    bool uartUsca0RxTrxDone;
    uint8_t uartUsca0RxChannelDmaChunkCount;
    uint32_t uartUsca0RxChannelDataLeftToTransfer;
    Event_Synchronous_t onUartUsca0RxTrxDone;
    /* UART A0 TX */
    void *uartUsca0TxChannelSrc;
    void *uartUsca0TxChannelDst;
    bool uartUsca0TxTrxDone;
    uint8_t uartUsca0TxChannelDmaChunkCount;
    uint32_t uartUsca0TxChannelDataLeftToTransfer;
    Event_Synchronous_t onUartUsca0TxTrxDone;
    /* UART A3 TX */
    void *uartUsca3TxChannelSrc;
    void *uartUsca3TxChannelDst;
    bool uartUsca3TxTrxDone;
    uint8_t uartUsca3TxChannelDmaChunkCount;
    uint32_t uartUsca3TxChannelDataLeftToTransfer;
    Event_Synchronous_t onUartUsca3TxTrxDone;
    /* UART A3 RX */
    void *uartUsca3RxChannelSrc;
    void *uartUsca3RxChannelDst;
    bool uartUsca3RxTrxDone;
    uint8_t uartUsca3RxChannelDmaChunkCount;
    uint32_t uartUsca3RxChannelDataLeftToTransfer;
    Event_Synchronous_t onUartUsca3RxTrxDone;
} DmaController_MSP432_t;

static DmaController_MSP432_t instance;

static void SetChannelSourceTrigger(I_DmaController_t *instance, uint32_t channel, const void *sourceTriggerConfig)
{
    IGNORE(instance);
    IGNORE(sourceTriggerConfig);

    Uassert((channel < DmaChannel_Max));

    DMA_disableModule();
    DMA_enableModule();

    switch(channel)
    {
        case DmaChannel_UartUsca0Tx:
            DMA_assignChannel(DMA_CH0_EUSCIA0TX);
            break;

        case DmaChannel_UartUsca0Rx:
            DMA_assignChannel(DMA_CH1_EUSCIA0RX);
            break;

        case DmaChannel_UartUsca3Tx:
            DMA_assignChannel(DMA_CH6_EUSCIA3TX);
            break;

        case DmaChannel_UartUsca3Rx:
            DMA_assignChannel(DMA_CH7_EUSCIA3RX);
            break;

        default:
            break;
    }
}

static void SetChannelTransferConfig(I_DmaController_t *_instance, uint32_t channel, const void *config)
{
    IGNORE(_instance);
    IGNORE(config);

    Uassert((channel < DmaChannel_Max));

    switch(channel)
    {
        case DmaChannel_UartUsca0Tx:
            DMA_setChannelControl(
                (DMA_CH0_EUSCIA0TX | UDMA_PRI_SELECT),
                (UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1));
                break;

        case DmaChannel_UartUsca0Rx:
            DMA_setChannelControl(
                (DMA_CH1_EUSCIA0RX | UDMA_PRI_SELECT),
                (UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1));
            break;

        case DmaChannel_UartUsca3Tx:
            DMA_setChannelControl(
                (DMA_CH6_EUSCIA3TX | UDMA_PRI_SELECT),
                (UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1));
            break;

        case DmaChannel_UartUsca3Rx:
            DMA_setChannelControl(
                (DMA_CH7_EUSCIA3RX | UDMA_PRI_SELECT),
                (UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1));
            break;

        default:
            break;
    }
}

static I_Event_t * GetOnChannelTransferDoneEvent(I_DmaController_t *_instance, uint32_t channel)
{
    IGNORE(_instance);
    I_Event_t *event = NULL;

    switch(channel)
    {
        case DmaChannel_UartUsca0Rx:
            event = &instance.onUartUsca0RxTrxDone.interface;
            break;

        case DmaChannel_UartUsca0Tx:
            event = &instance.onUartUsca0TxTrxDone.interface;
            break;

        case DmaChannel_UartUsca3Tx:
            event = &instance.onUartUsca3TxTrxDone.interface;
            break;

        case DmaChannel_UartUsca3Rx:
            event = &instance.onUartUsca3RxTrxDone.interface;
            break;

        default:
            break;
    }

    return event;
}

static void SetAndStartChannelTrasfer(
    I_DmaController_t *_instance,
    uint32_t channel,
    const void *transferConfig,
    void *src,
    void *dst,
    uint32_t transferSize)
{
    IGNORE(_instance);
    IGNORE(transferConfig);

    switch(channel)
    {
        case DmaChannel_UartUsca0Rx:
        {
            instance.uartUsca0RxChannelDmaChunkCount = 1;
            instance.uartUsca0RxChannelSrc = src;
            instance.uartUsca0RxChannelDst = dst;
            instance.uartUsca0RxChannelDataLeftToTransfer =
                (transferSize > MaxDmaTrxSize) ? (transferSize - MaxDmaTrxSize) : transferSize;
            instance.uartUsca0RxTrxDone = (transferSize < MaxDmaTrxSize);

            DMA_setChannelTransfer(
              (DMA_CH1_EUSCIA0RX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              src,
              dst,
              (transferSize > MaxDmaTrxSize) ? MaxDmaTrxSize : transferSize);

            DMA_assignInterrupt(INT_DMA_INT1, 1);
            DMA_clearInterruptFlag(DMA_CH1_EUSCIA0RX & 0x0F);
            DMA_enableChannel(1);
            NVIC->ISER[1] |= (1 << ((DMA_INT1_IRQn) & 31));
            break;
        }

        case DmaChannel_UartUsca0Tx:
        {
            instance.uartUsca0TxChannelDmaChunkCount = 1;
            instance.uartUsca0TxChannelSrc = src;
            instance.uartUsca0TxChannelDst = dst;
            instance.uartUsca0TxChannelDataLeftToTransfer =
                (transferSize > MaxDmaTrxSize) ? (transferSize - MaxDmaTrxSize) : transferSize;
            instance.uartUsca0TxTrxDone = (transferSize < MaxDmaTrxSize);

            DMA_setChannelTransfer(
              (DMA_CH0_EUSCIA0TX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              src,
              dst,
              (transferSize > MaxDmaTrxSize) ? MaxDmaTrxSize : transferSize);

            DMA_assignInterrupt(INT_DMA_INT0, 0);
            DMA_clearInterruptFlag(DMA_CH0_EUSCIA0TX & 0x0F);
            DMA_enableChannel(0);
            NVIC->ISER[1] |= (1 << ((DMA_INT0_IRQn) & 31));
            break;
        }

        case DmaChannel_UartUsca3Tx:
        {
            instance.uartUsca3TxChannelDmaChunkCount = 1;
            instance.uartUsca3TxChannelSrc = src;
            instance.uartUsca3TxChannelDst = dst;
            instance.uartUsca3TxChannelDataLeftToTransfer =
                (transferSize > MaxDmaTrxSize) ? (transferSize - MaxDmaTrxSize) : transferSize;
            instance.uartUsca3TxTrxDone = (transferSize < MaxDmaTrxSize);

            DMA_setChannelTransfer(
              (DMA_CH6_EUSCIA3TX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              src,
              dst,
              (transferSize > MaxDmaTrxSize) ? MaxDmaTrxSize : transferSize);

            DMA_assignInterrupt(INT_DMA_INT2, 6);
            DMA_clearInterruptFlag(DMA_CH6_EUSCIA3TX & 0x0F);
            DMA_enableChannel(6);
            NVIC->ISER[1] |= (1 << ((DMA_INT2_IRQn) & 31));
            break;
        }

        case DmaChannel_UartUsca3Rx:
        {
            instance.uartUsca3RxChannelDmaChunkCount = 1;
            instance.uartUsca3RxChannelSrc = src;
            instance.uartUsca3RxChannelDst = dst;
            instance.uartUsca3RxChannelDataLeftToTransfer =
                (transferSize > MaxDmaTrxSize) ? (transferSize - MaxDmaTrxSize) : transferSize;
            instance.uartUsca3RxTrxDone = (transferSize < MaxDmaTrxSize);

            DMA_setChannelTransfer(
              (DMA_CH7_EUSCIA3RX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              src,
              dst,
              (transferSize > MaxDmaTrxSize) ? MaxDmaTrxSize : transferSize);

            DMA_assignInterrupt(INT_DMA_INT3, 7);
            DMA_clearInterruptFlag(DMA_CH7_EUSCIA3RX & 0x0F);
            DMA_enableChannel(7);
            NVIC->ISER[0] |= 0x80000000; // Set bit 31 (DMA_INT3_IRQn)
            break;
        }

        default:
            break;
    }
}

static void ClearState(I_DmaController_t *_instance)
{
    IGNORE(_instance);
    DMA_disableModule();
}

static const DmaControllerApi_t api =
    {
      SetChannelSourceTrigger,
      SetChannelTransferConfig,
      SetAndStartChannelTrasfer,
      GetOnChannelTransferDoneEvent,
      ClearState
    };

I_DmaController_t * DmaController_MSP432_Init(void)
{
    DMA_enableModule();
    DMA_setControlBase(MSP432ControlTable);
    Interrupt_setPriority(INT_DMA_INT0, 0);
    Interrupt_setPriority(INT_DMA_INT1, 1);
    Interrupt_setPriority(INT_DMA_INT2, 2);
    Interrupt_setPriority(INT_DMA_INT3, 3);
    Event_Synchronous_Init(&instance.onUartUsca0RxTrxDone);
    Event_Synchronous_Init(&instance.onUartUsca3TxTrxDone);
    Event_Synchronous_Init(&instance.onUartUsca0TxTrxDone);
    Event_Synchronous_Init(&instance.onUartUsca3RxTrxDone);
    instance.interface.api = &api;
    return &instance.interface;
}

void DMA_INT0_IRQHandler(void)
{
    if(instance.uartUsca0TxTrxDone)
    {
        DMA_clearInterruptFlag(0);
        Event_Publish(&instance.onUartUsca0TxTrxDone.interface, NULL);
    }
    else
    {
        instance.uartUsca0TxChannelDmaChunkCount++;

        if(instance.uartUsca0TxChannelDataLeftToTransfer > MaxDmaTrxSize && !instance.uartUsca0TxTrxDone)
        {
            DMA_setChannelTransfer(
              (DMA_CH0_EUSCIA0TX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              (void *) (((uint8_t *)instance.uartUsca0TxChannelSrc) + instance.uartUsca0TxChannelDmaChunkCount * MaxDmaTrxSize),
              instance.uartUsca0TxChannelDst,
              MaxDmaTrxSize);
            DMA_enableChannel(0);
        }
        else if(instance.uartUsca0TxChannelDataLeftToTransfer <= MaxDmaTrxSize && !instance.uartUsca0TxTrxDone)
        {
            DMA_setChannelTransfer(
              (DMA_CH0_EUSCIA0TX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              (void *) (((uint8_t *)instance.uartUsca0TxChannelSrc) + instance.uartUsca0TxChannelDmaChunkCount * MaxDmaTrxSize),
              instance.uartUsca0TxChannelDst,
              instance.uartUsca0TxChannelDataLeftToTransfer);
            instance.uartUsca0TxTrxDone = true;
            DMA_enableChannel(0);
        }

        instance.uartUsca0TxChannelDataLeftToTransfer -= MaxDmaTrxSize;
    }
}

void DMA_INT1_IRQHandler(void)
{
    if(instance.uartUsca0RxTrxDone)
    {
        DMA_clearInterruptFlag(1);
        Event_Publish(&instance.onUartUsca0RxTrxDone.interface, NULL);
    }
    else
    {
        instance.uartUsca0RxChannelDmaChunkCount++;

        if(instance.uartUsca0RxChannelDataLeftToTransfer > MaxDmaTrxSize && !instance.uartUsca0RxTrxDone)
        {
            DMA_setChannelTransfer(
              (DMA_CH1_EUSCIA0RX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              instance.uartUsca0RxChannelSrc,
              (void *) (((uint8_t *)instance.uartUsca0RxChannelDst) + instance.uartUsca0RxChannelDmaChunkCount * MaxDmaTrxSize),
              MaxDmaTrxSize);
            DMA_enableChannel(1);
        }
        else if(instance.uartUsca0RxChannelDataLeftToTransfer <= MaxDmaTrxSize && !instance.uartUsca0RxTrxDone)
        {
            DMA_setChannelTransfer(
              (DMA_CH1_EUSCIA0RX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              instance.uartUsca0RxChannelSrc,
              (void *) (((uint8_t *)instance.uartUsca0RxChannelDst) + instance.uartUsca0RxChannelDmaChunkCount * MaxDmaTrxSize),
              instance.uartUsca0RxChannelDataLeftToTransfer);
            instance.uartUsca0RxTrxDone = true;
            DMA_enableChannel(1);
        }

        instance.uartUsca0RxChannelDataLeftToTransfer -= MaxDmaTrxSize;
    }
}

void DMA_INT2_IRQHandler(void)
{
    if(instance.uartUsca3TxTrxDone)
    {
        DMA_clearInterruptFlag(6);
        Event_Publish(&instance.onUartUsca3TxTrxDone.interface, NULL);
    }
    else
    {
        instance.uartUsca3TxChannelDmaChunkCount++;

        if(instance.uartUsca3TxChannelDataLeftToTransfer > MaxDmaTrxSize && !instance.uartUsca3TxTrxDone)
        {
            DMA_setChannelTransfer(
              (DMA_CH6_EUSCIA3TX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              (void *) (((uint8_t *)instance.uartUsca3TxChannelSrc) + instance.uartUsca3TxChannelDmaChunkCount * MaxDmaTrxSize),
              instance.uartUsca3TxChannelDst,
              MaxDmaTrxSize);
            DMA_enableChannel(6);
        }
        else if(instance.uartUsca3TxChannelDataLeftToTransfer <= MaxDmaTrxSize && !instance.uartUsca3TxTrxDone)
        {
            DMA_setChannelTransfer(
              (DMA_CH6_EUSCIA3TX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              (void *) (((uint8_t *)instance.uartUsca3TxChannelSrc) + instance.uartUsca3TxChannelDmaChunkCount * MaxDmaTrxSize),
              instance.uartUsca3TxChannelDst,
              instance.uartUsca3TxChannelDataLeftToTransfer);
            instance.uartUsca3TxTrxDone = true;
            DMA_enableChannel(6);
        }

        instance.uartUsca3TxChannelDataLeftToTransfer -= MaxDmaTrxSize;
    }
}

void DMA_INT3_IRQHandler(void)
{
    if(instance.uartUsca3RxTrxDone)
    {
        DMA_clearInterruptFlag(7);
        Event_Publish(&instance.onUartUsca3RxTrxDone.interface, NULL);
    }
    else
    {
        instance.uartUsca3RxChannelDmaChunkCount++;

        if(instance.uartUsca3RxChannelDataLeftToTransfer > MaxDmaTrxSize && !instance.uartUsca3RxTrxDone)
        {
            DMA_setChannelTransfer(
              (DMA_CH7_EUSCIA3RX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              instance.uartUsca3RxChannelSrc,
              (void *) (((uint8_t *)instance.uartUsca3RxChannelDst) + instance.uartUsca3RxChannelDmaChunkCount * MaxDmaTrxSize),
              MaxDmaTrxSize);
            DMA_enableChannel(7);
        }
        else if(instance.uartUsca3RxChannelDataLeftToTransfer <= MaxDmaTrxSize && !instance.uartUsca3RxTrxDone)
        {
            DMA_setChannelTransfer(
              (DMA_CH7_EUSCIA3RX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              instance.uartUsca3RxChannelSrc,
              (void *) (((uint8_t *)instance.uartUsca3RxChannelDst) + instance.uartUsca3RxChannelDmaChunkCount * MaxDmaTrxSize),
              instance.uartUsca3RxChannelDataLeftToTransfer);
            instance.uartUsca3RxTrxDone = true;
            DMA_enableChannel(7);
        }

        instance.uartUsca3RxChannelDataLeftToTransfer -= MaxDmaTrxSize;
    }
}
