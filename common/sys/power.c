#include "power.h"
#include "cm4.h"
#include <libopencm3/stm32/rcc.h>

void set_voltage_scale(VOS scale) {
    rcc_periph_clock_enable(RCC_PWR);
    PWR_CR = (PWR_CR & ~(3 << 14)) | ((scale & 0x3) << 14);
    __DSB();
}

VOS get_voltage_scale() {
    rcc_periph_clock_enable(RCC_PWR);
    return ((PWR_CR >> 14) & 0x3);
}

void start_Overdrive() {
    rcc_periph_clock_enable(RCC_PWR);
    /* Start overdrive */
    PWR_CR |= (1 << 16);
    while ((PWR_CSR & (1 << 16)) == 0)
        ;
    /* Switch to overdrive */
    PWR_CR |= (1 << 17);
    while ((PWR_CSR & (1 << 17)) == 0)
        ;
}

void stop_Overdrive() {
    rcc_periph_clock_enable(RCC_PWR);
    /* Stop overdrive */
    PWR_CR &= ~((1 << 16) | (1 << 17));
    __DSB();
}
