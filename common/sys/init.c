#include "clock.h"
#include "cm4.h"
#include "power.h"
#include "serial_io.h"
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/syscfg.h>

void set_FLASH_latency(uint32_t latency) {
    FLASH_ACR = (FLASH_ACR & ~(15 << 0) & ~(7 << 8)) |
                ((((latency > 0) ? 7 : 0) << 8) | ((latency & 15) << 0));
    /* if latency > 0, we also enable cache and prefetch */
    while (((FLASH_ACR >> 0) & 15) != latency)
        ;
}

void fpu_init(void) {
/* Only on ARMv7 and above */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* enable FPU for unpriviledged access  */
    SCB_CPACR |= (3 << 20) | (3 << 22);
    __DSB();
#endif
}

void enable_io_compensation(void) {
    rcc_periph_clock_enable(RCC_SYSCFG);
    SYSCFG_CMPCR |= (1 << 0);
    while ((SYSCFG_CMPCR & (1 << 8)) == 0)
        ;
}

void clock_init(void) {
#ifdef STM32F446RE
    /* FIXME: found in another code (already done ?) */
    // RCC.APB1ENR |= RCC_APB1ENR_PWREN;
    set_MCO(MCO1_PLL, 5, MCO2_SYS, 5);
    /* Select regulator voltage output Scale 1 mode, System frequency up to
     * 168..180 MHz */
    set_voltage_scale(scale1);
    /* Enable the Over-drive to extend the clock frequency to 180 (pour 360) Mhz
     */
    start_Overdrive();
    /* Configure the main PLL
     * Enable the main PLL
     * And wait till the main PLL is ready
     */
    start_PLL(PLLSRC_HSI, 16, 336, 2, 7, 7);

    /* HCLK = SYSCLK / 1 */
    /* PCLK1 = HCLK / 4 */
    /* PCLK2 = HCLK / 2 */
    set_bus_clock_dividers_exp(0, 2, 1);
    /* Configure Flash prefetch, Instruction cache, Data cache and wait state */
    set_FLASH_latency(5);
    /* Select the main PLL as system clock source
     * and wait till the main PLL is used as system clock source
     */
    set_SYSCLK(SW_PLL_P);
    enable_io_compensation();
    enable_MCO_GPIO();
#else
    set_MCO(MCO1_PLL, 5, MCO2_SYS, 5);
    /* FIXME: p73 Scale 2, or scale 3 can be configured through the
       VOS[1:0] bits of the PWR_CR register (scale 1 is not used in this
       product)
    */
    set_voltage_scale(scale2);
    /* FIXME: no overdrive mode on this plateform */
    // start_Overdrive();
    start_PLL(PLLSRC_HSI, 16, 336, 2, 7, 0); // TODO
    set_bus_clock_dividers_exp(0, 2, 1);
    set_FLASH_latency(0); // TODO
    set_SYSCLK(SW_HSI);   // TODO
    enable_io_compensation();
    enable_MCO_GPIO();
#endif
}

void clock_init2(void) {
    // improved version with overdrive
    rcc_clock_setup_pll_x(&rcc_hsi_configs[RCC_CLOCK_3V3_168MHZ], 1);
    // It is not possible to have 180MHz clock and use the USB at the same time
    // as you cant get 48MHz required by the USB peripheral.
    // rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_180MHZ]);
}

void software_init_hook(void) { /* called from crt0.S */
    fpu_init();
    clock_init2();
    serial_io_init();
}

__attribute__((section(".preinit_array"))) void (*compat_lib_init)(void) =
    software_init_hook;
