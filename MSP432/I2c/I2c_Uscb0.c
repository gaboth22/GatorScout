#include "msp.h"
#include "I2c_Uscb0.h"
#include "TimerModule.h"
#include "TimerPeriodic.h"
#include "utils.h"

typedef struct
{
    I_I2c_t interface;
    TimerModule_t *timerModule;
    TimerPeriodic_t checkIfDoneReadingOrWritingTimer;
    uint8_t currentDeviceAddress;
    uint8_t currentDeviceInternalAddress;
    uint8_t currentByteToWrite;
    uint8_t currentReadByte;
    uint8_t i2cState;
    bool readCycle;
    bool doneWriting;
    bool doneReading;
    union
    {
        void *readCallbackContext;
        void *writeCallbackContext;
    } callbackContexts;
    union
    {
        I2cWriteByteCallbackFunction_t writeCallback;
        I2cReadByteCallbackFunction_t readCallback;
    } callbacks;
} I2c_Uscb0_t;

enum
{
    I2cUnitialized = 0,
    I2cWriteDeviceInternalAddress,
    I2cWriteData,
    I2cSendStopCondition,
    I2cGetReadByte,
    I2cFinishRead,
    PeriodToCheckIfWeAreDoneReadingOrWritingMs = 10
};

static I2c_Uscb0_t instance;

static void CheckIfDoneReadingOrWriting(void *context)
{
    IGNORE(context);

    if(instance.doneWriting)
    {
        instance.doneWriting = false;
        TimerPeriodic_Command(&instance.checkIfDoneReadingOrWritingTimer, TimerPeriodicCommand_Pause);
        instance.callbacks.writeCallback(instance.callbackContexts.writeCallbackContext);
    }
    else if(instance.doneReading)
    {
        instance.doneReading =  false;
        TimerPeriodic_Command(&instance.checkIfDoneReadingOrWritingTimer, TimerPeriodicCommand_Pause);
        instance.callbacks.readCallback(instance.callbackContexts.readCallbackContext, instance.currentReadByte);
    }
}

static void CommonConfig(I2c_Uscb0_t *instance)
{
    instance->doneWriting = false;
    instance->doneReading = false;
    instance->i2cState = I2cWriteDeviceInternalAddress;

    TimerPeriodic_Command(&instance->checkIfDoneReadingOrWritingTimer, TimerPeriodicCommand_Resume);

    EUSCI_B0->I2CSA = instance->currentDeviceAddress;
    while (EUSCI_B0->CTLW0 & EUSCI_B_CTLW0_TXSTP); // Ensure stop condition got sent
    EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_TR | EUSCI_B_CTLW0_TXSTT; // Set transmitter mode and Generate start condition
}

static void WriteByte(
    I_I2c_t *_instance,
    uint8_t deviceAddress,
    uint8_t writeAddres,
    uint8_t byteToWrite,
    I2cWriteByteCallbackFunction_t callback,
    void *context)
{
    RECAST(instance, _instance, I2c_Uscb0_t *);

    instance->readCycle = false;
    instance->currentDeviceAddress = (deviceAddress >> 1); // Peripheral will only send lower 7 bits
    instance->currentDeviceInternalAddress = writeAddres;
    instance->currentByteToWrite = byteToWrite;
    instance->callbacks.writeCallback = callback;
    instance->callbackContexts.writeCallbackContext = context;

    CommonConfig(instance);
}

static void ReadByte(
    I_I2c_t *_instance,
    uint8_t deviceAddress,
    uint8_t readAddress,
    I2cReadByteCallbackFunction_t callback,
    void *context)
{
    RECAST(instance, _instance, I2c_Uscb0_t *);

    instance->readCycle = true;
    instance->currentDeviceAddress = (deviceAddress >> 1); // Peripheral will only send lower 7 bits
    instance->currentDeviceInternalAddress = readAddress;
    instance->callbacks.readCallback = callback;
    instance->callbackContexts.readCallbackContext = context;

    CommonConfig(instance);
}

static const I2cApi_t api =
    { WriteByte, ReadByte };

static inline void UCB0_Init(void)
{
    P1->SEL0 |= BIT6 | BIT7; // P1.6 = SDA; P1.7 = SCL

    // Enable eUSCIB0 interrupt in NVIC module
    NVIC->ISER[0] = 1 << ((EUSCIB0_IRQn) & 31);

    // Configure USCI_B0 for I2C mode
    EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_SWRST; // put eUSCI_B in reset state
    EUSCI_B0->CTLW0 = EUSCI_B_CTLW0_SWRST | // Remain eUSCI_B in reset state
            EUSCI_B_CTLW0_MODE_3 |          // I2C mode
            EUSCI_B_CTLW0_MST |             // I2C master mode
            EUSCI_B_CTLW0_SYNC |            // Sync mode
            EUSCI_B_CTLW0_SSEL__SMCLK;      // SMCLK
    EUSCI_B0->BRW = 480; // baudrate = SMCLK / 480 = 100 KHz
    EUSCI_B0->CTLW0 &= ~EUSCI_B_CTLW0_SWRST;// clear reset register
    EUSCI_B0->IE |= EUSCI_B_IE_TXIE0 | EUSCI_B_IE_NACKIE; // Enable transmit interrupt
}

I_I2c_t * I2c_Uscb0_Init(TimerModule_t *timerModule)
{
    instance.interface.api = &api;

    UCB0_Init();

    instance.timerModule = timerModule;
    instance.i2cState = I2cUnitialized;
    TimerPeriodic_Init(
        &instance.checkIfDoneReadingOrWritingTimer,
        timerModule,
        PeriodToCheckIfWeAreDoneReadingOrWritingMs,
        CheckIfDoneReadingOrWriting,
        NULL);
    TimerPeriodic_Command(&instance.checkIfDoneReadingOrWritingTimer, TimerPeriodicCommand_Pause);
    TimerPeriodic_Start(&instance.checkIfDoneReadingOrWritingTimer);

    return &instance.interface;
}

void EUSCIB0_IRQHandler(void)
{
    if(EUSCI_B0->IFG & EUSCI_B_IFG_NACKIFG)
    {
        EUSCI_B0->IFG &= ~EUSCI_B_IFG_NACKIFG; // Clear NACK flag
        EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_TXSTT; // Send I2C start condition
    }

    if(EUSCI_B0->IFG & EUSCI_B_IFG_RXIFG0)
    {
        if(instance.i2cState  == I2cFinishRead)
        {
            EUSCI_B0->IFG &= ~ EUSCI_B_IFG_RXIFG0;
            instance.i2cState = I2cUnitialized;
            instance.currentReadByte = EUSCI_B0->RXBUF;
            instance.readCycle = false;
            instance.doneReading = true;
        }
    }

    if(EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG0)
    {
        if(instance.i2cState == I2cWriteDeviceInternalAddress)
        {
            instance.i2cState = instance.readCycle ? I2cGetReadByte : I2cWriteData;
            EUSCI_B0->TXBUF = instance.currentDeviceInternalAddress;
        }
        else if(instance.i2cState == I2cWriteData)
        {
            instance.i2cState = I2cSendStopCondition;
            EUSCI_B0->TXBUF  = instance.currentByteToWrite;
        }
        else if(instance.i2cState == I2cSendStopCondition)
        {
            EUSCI_B0->IFG &= ~EUSCI_B_IFG_TXIFG0;
            instance.i2cState = I2cUnitialized;
            EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_TXSTP;
            EUSCI_B0->IFG &= ~EUSCI_B_IFG_TXIFG;
            instance.doneWriting = true;
        }
        else if(instance.i2cState == I2cGetReadByte)
        {
            EUSCI_B0->IFG &= ~EUSCI_B_IFG_TXIFG0; // Clear TX flag
            instance.i2cState = I2cFinishRead;
            EUSCI_B0->IE |= EUSCI_A_IE_RXIE; // Enable RX interrupt
            EUSCI_B0->CTLW0  &= ~EUSCI_B_CTLW0_TR; // Set receiver mode
            EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_TXSTT;
            // Begin polling for start bit flag to clear so we can send the stop bit
            // In this context, when UCTXSTT (start condition bit) clears,
            // the byte begins to be received, and we can set UCTXSTP (stop condition bit)
            while(EUSCI_B0->CTLW0 & EUSCI_B_CTLW0_TXSTT);
            EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_TXSTP;
        }
    }
}
