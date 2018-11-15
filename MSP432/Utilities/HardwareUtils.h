#ifndef HARDWAREUTILS_H
#define HARDWAREUTILS_H

#define SetClockTo48Mhz() \
    while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY)); \
    PCM->CTL0 = PCM_CTL0_KEY_VAL | PCM_CTL0_AMR_1; \
    while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY)); \
    FLCTL->BANK0_RDCTL = (FLCTL->BANK0_RDCTL & ~(FLCTL_BANK0_RDCTL_WAIT_MASK)) | FLCTL_BANK0_RDCTL_WAIT_1; \
    FLCTL->BANK1_RDCTL  = (FLCTL->BANK0_RDCTL & ~(FLCTL_BANK1_RDCTL_WAIT_MASK)) | FLCTL_BANK1_RDCTL_WAIT_1; \
    CS->KEY = CS_KEY_VAL; \
    CS->CTL0 = 0; \
    CS->CTL0 = CS_CTL0_DCORSEL_5; \
    CS->CTL1 = CS->CTL1 & ~(CS_CTL1_SELM_MASK | CS_CTL1_DIVM_MASK) | CS_CTL1_SELM_3; \
    CS->KEY = 0; \

#define StopWatchdog() \
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD; \

#define StartGlobalPwmTick() \
    TIMER_A0->CCTL[0] = TIMER_A_CCTLN_OUTMOD_4; \
    TIMER_A0->CCR[0] = 48000; \

#define EnableInterrupts() \
    CS->KEY = CS_KEY_VAL; \
    __enable_irq(); \
    CS->KEY = 0; \

#define DisableInterrupts() \
    CS->KEY = CS_KEY_VAL; \
    __disable_irq(); \
    CS->KEY = 0; \

#endif
