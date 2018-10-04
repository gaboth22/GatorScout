#include "Adc_Precision14.h"
#include "msp.h"
#include "utils.h"

enum
{
    RunningAvgSize = 3
};

static I_Adc_t instance;
static AdcCounts_t runningAverageData[RunningAvgSize] = { 0 };
static uint8_t avgIndex = 0;

static AdcCounts_t GetAdcCounts(I_Adc_t *_instance)
{
    IGNORE(_instance);
    return ((runningAverageData[0] + runningAverageData[1] + runningAverageData[2] ) / RunningAvgSize);
}

static const AdcApi_t api =
    { GetAdcCounts };

I_Adc_t * Adc_Precision14_Init(void)
{
    instance.api = &api;
    avgIndex = 0;
    P4->SEL0 |= BIT7; // Set P4.7 as second peripheral usage - A6
    P4->SEL1 |= BIT7; // Set P4.7 as second peripheral usage - A6
    NVIC->ISER[0] |= 1 << ((ADC14_IRQn) & 31);
    while(REFCTL0 & REFGENBUSY);
    REF_A->CTL0 = REF_A_CTL0_OUT| REF_A_CTL0_VSEL_3 | REF_A_CTL0_ON;
    ADC14->CTL0 = ADC14_CTL0_SHT0_7 | ADC14_CTL0_SSEL_0 | ADC14_CTL0_SHP | ADC14_CTL0_CONSEQ_0 | ADC14_CTL0_ON;
    ADC14->CTL1 = ADC14_CTL1_RES_2 | (ADC14_CTL1_CSTARTADD_MASK & (6 << 16)) | ADC14_CTL1_REFBURST;
    ADC14->MCTL[6] |= (ADC14_MCTLN_INCH_6 | ADC14_MCTLN_VRSEL_1); // Input A6, buffered reference - 2.5V
    ADC14->CTL0 |= (ADC14_CTL0_ENC | ADC14_CTL0_SC); // Start sampling and conversion
    ADC14->IER0 = ADC14_IER0_IE6;

    return &instance;
}

void ADC14_IRQHandler(void)
{
    if(ADC14->IV == 0x18)
    {
        ADC14->CLRIFGR0 = ADC14_CLRIFGR0_CLRIFG6;
        runningAverageData[avgIndex] = (AdcCounts_t)ADC14->MEM[6];
        avgIndex++;
        avgIndex = avgIndex % RunningAvgSize;
        ADC14->CTL0 |= (ADC14_CTL0_ENC | ADC14_CTL0_SC);
    }
}
