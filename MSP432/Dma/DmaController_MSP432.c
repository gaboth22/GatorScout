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
    Event_Synchronous_t onUartUsca0TrxDone;
    Event_Synchronous_t onUartUsca3TrxDone;
    uint32_t uartUsca0ChannelTrxSize;
    uint32_t uartUsca3ChannelTrxSize;
    uint8_t uartUsca0ChannelDmaChunkCount;
    uint8_t uartUsca3ChannelDmaChunkCount;
    void *uartUsca0ChannelSrc;
    void *uartUsca0ChannelDst;
    void *uartUsca3ChannelSrc;
    void *uartUsca3ChannelDst;
} DmaController_MSP432_t;

static DmaController_MSP432_t instance;

static void SetChannelSourceTrigger(I_DmaController_t *instance, uint32_t channel, const void *sourceTriggerConfig)
{
    IGNORE(instance);
    IGNORE(sourceTriggerConfig);

    Uassert((channel < DmaChannel_Max));

    switch(channel)
    {
        case DmaChannel_UartUsca0Rx:
            DMA_assignChannel(DMA_CH1_EUSCIA0RX);
            break;

        case DmaChannel_UartUsca3Tx:
            DMA_assignChannel(DMA_CH6_EUSCIA3TX);
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
            event = &instance.onUartUsca0TrxDone.interface;
            break;

        case DmaChannel_UartUsca3Tx:
            event = &instance.onUartUsca3TrxDone.interface;
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
            instance.uartUsca0ChannelDmaChunkCount = 0;
            instance.uartUsca0ChannelTrxSize = transferSize;
            instance.uartUsca0ChannelSrc = src;
            instance.uartUsca0ChannelDst = dst;

            DMA_setChannelTransfer(
              (DMA_CH1_EUSCIA0RX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              src,
              dst,
              MaxDmaTrxSize);

            DMA_assignInterrupt(INT_DMA_INT1, 1);
            DMA_clearInterruptFlag(DMA_CH1_EUSCIA0RX & 0x0F);
            DMA_enableChannel(1);
            NVIC->ISER[1] |= (1 << ((DMA_INT1_IRQn) & 31));
            break;
        }

        case DmaChannel_UartUsca3Tx:
        {
            instance.uartUsca3ChannelDmaChunkCount = 0;
            instance.uartUsca3ChannelTrxSize = transferSize;
            instance.uartUsca3ChannelSrc = src;
            instance.uartUsca3ChannelDst = dst;

            DMA_setChannelTransfer(
              (DMA_CH6_EUSCIA3TX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              src,
              dst,
              MaxDmaTrxSize);

            DMA_assignInterrupt(INT_DMA_INT2, 6);
            DMA_clearInterruptFlag(DMA_CH6_EUSCIA3TX & 0x0F);
            DMA_enableChannel(6);
            NVIC->ISER[1] |= (1 << ((DMA_INT2_IRQn) & 31));
            break;
        }

        default:
            break;
    }
}

static void ClearState(I_DmaController_t *_instance)
{
    IGNORE(_instance);
    DMA_clearInterruptFlag(1);
    instance.uartUsca0ChannelDmaChunkCount = 0;
    DMA_clearInterruptFlag(6);
    instance.uartUsca3ChannelDmaChunkCount = 0;
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
    Event_Synchronous_Init(&instance.onUartUsca0TrxDone);
    Event_Synchronous_Init(&instance.onUartUsca3TrxDone);
    instance.uartUsca0ChannelTrxSize = 0;
    instance.interface.api = &api;
    return &instance.interface;
}

void DMA_INT1_IRQHandler(void)
{
    instance.uartUsca0ChannelDmaChunkCount++;

    switch(instance.uartUsca0ChannelDmaChunkCount)
    {
        /*
         * The purpose of DMA this channel is to receive data that is always known to be
         * between 3.5K and 3.6K, so no need for further generalization
         */
        case 1:
        case 2:
            DMA_setChannelTransfer(
              (DMA_CH1_EUSCIA0RX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              instance.uartUsca0ChannelSrc,
              (void *) (((uint8_t *)instance.uartUsca0ChannelDst) + instance.uartUsca0ChannelDmaChunkCount * MaxDmaTrxSize),
              MaxDmaTrxSize);
            DMA_enableChannel(1);
            break;
        case 3:
            DMA_setChannelTransfer(
              (DMA_CH1_EUSCIA0RX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              instance.uartUsca0ChannelSrc,
              (void *) (((uint8_t *)instance.uartUsca0ChannelDst) + instance.uartUsca0ChannelDmaChunkCount * MaxDmaTrxSize),
              instance.uartUsca0ChannelTrxSize % MaxDmaTrxSize);
            DMA_enableChannel(1);
            break;
        case 4:
            DMA_clearInterruptFlag(1);
            instance.uartUsca0ChannelDmaChunkCount = 0;
            Event_Publish(&instance.onUartUsca0TrxDone.interface, NULL);
            break;
    }
}

void DMA_INT2_IRQHandler(void)
{
    instance.uartUsca3ChannelDmaChunkCount++;

    switch(instance.uartUsca3ChannelDmaChunkCount)
    {
        /*
         * The purpose of DMA this channel is to send data that is always known to be
         * between 3.5K and 3.6K, so no need for further generalization
         */
        case 1:
        case 2:
            DMA_setChannelTransfer(
              (DMA_CH6_EUSCIA3TX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              (void *) (((uint8_t *)instance.uartUsca3ChannelSrc) + instance.uartUsca3ChannelDmaChunkCount * MaxDmaTrxSize),
              instance.uartUsca3ChannelDst,
              MaxDmaTrxSize);
            DMA_enableChannel(6);
            break;
        case 3:
            DMA_setChannelTransfer(
              (DMA_CH6_EUSCIA3TX | UDMA_PRI_SELECT),
              UDMA_MODE_BASIC,
              (void *) (((uint8_t *)instance.uartUsca0ChannelDst) + instance.uartUsca3ChannelDmaChunkCount * MaxDmaTrxSize),
              instance.uartUsca3ChannelDst,
              instance.uartUsca3ChannelTrxSize % MaxDmaTrxSize);
            DMA_enableChannel(6);
            break;
        case 4:
            DMA_clearInterruptFlag(6);
            instance.uartUsca3ChannelDmaChunkCount = 0;
            Event_Publish(&instance.onUartUsca3TrxDone.interface, NULL);
            break;
    }
}
