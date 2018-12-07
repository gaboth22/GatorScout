#include "Adc_Precision14.h"
#include "msp.h"
#include "utils.h"

enum
{
    RunningAvgSize = 4
};

static I_Adc_t instance;
static AdcCounts_t readings[RunningAvgSize] = { 0 };
static uint8_t index = 0;

static AdcCounts_t GetAdcCounts(I_Adc_t *_instance)
{
    IGNORE(_instance);
    uint8_t i = 0;
    AdcCounts_t val = 0;
    for(i = 0; i < RunningAvgSize; i++)
    {
        val += readings[i];
    }
    return val / RunningAvgSize;
}

static const AdcApi_t api =
    { GetAdcCounts };

I_Adc_t * Adc_Precision14_Init(void)
{
    instance.api = &api;
    P8->SEL0 |= BIT6; // Set P8.6 as second peripheral usage - A19
    P8->SEL1 |= BIT6; // Set P8.6 as second peripheral usage - A19
    NVIC->ISER[0] |= 1 << ((ADC14_IRQn) & 31);
    while(REFCTL0 & REFGENBUSY);
    REF_A->CTL0 = REF_A_CTL0_OUT | REF_A_CTL0_VSEL_3 | REF_A_CTL0_ON;
    ADC14->CTL0 = ADC14_CTL0_SHT0_6 | ADC14_CTL0_SSEL_0 | ADC14_CTL0_SHP | ADC14_CTL0_CONSEQ_0 | ADC14_CTL0_ON;
    ADC14->CTL1 = ADC14_CTL1_RES_2 | (ADC14_CTL1_CSTARTADD_MASK & (19 << 16)) | ADC14_CTL1_REFBURST;
    ADC14->MCTL[19] |= (ADC14_MCTLN_INCH_19); // Input A19, buffered reference - AVCC
    ADC14->CTL0 |= (ADC14_CTL0_ENC | ADC14_CTL0_SC); // Start sampling and conversion
    ADC14->IER0 = ADC14_IER0_IE19;

    return &instance;
}

void ADC14_IRQHandler(void)
{
    if(ADC14->IV == 0x32)
    {
        ADC14->CLRIFGR0 = ADC14_CLRIFGR0_CLRIFG19;
        readings[index] = (AdcCounts_t)ADC14->MEM[19];
        index++;
        index = index % RunningAvgSize;
        ADC14->CTL0 |= (ADC14_CTL0_ENC | ADC14_CTL0_SC);
    }
}
