#include "hardware.h"
#include "sys/clock.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/systick.h>  
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/stm32/usart.h>
#include <libopencmsis/core_cm3.h>
#include <libopencm3/cm3/nvic.h>

void buzzer_pwm_init(void) {
    /* buzzer is on TIM2_CH2 */
    /* configure PB9 as alternate function TIM2 */
    GPIOB_MODER = (GPIOB_MODER & ~(3 << 18)) | (2 << 18); 
    /* setup TIM2_CH2 as alternate function */
    GPIOB_AFRH = (GPIOB_AFRH & ~(0xf << 4)) | (1 << 4);  

     /* configure TIM2_CH2 as PWM output */
    TIM2_CR1 = 0x0080;   /* buffered preload up-counting mode */
    TIM2_CCMR1 = 0x6800; /* PWM mode */
    TIM2_CCER = 0x0010;  /* channel 2 output to pin PB9 */
    
    /* Set buzzer timer frequency to match standard musical notes */
    TIM2_PSC = get_APB1TIMCLK() / BUZZER_BASE - 1; 
    TIM2_CNT = 0;
    TIM2_EGR |= 0x1; /* update event */
}

void play_buzzer(uint32_t freq) {
    if(freq == 0) {
        TIM2_CR1 &= ~0x1;
        return;
    }
    uint32_t arr = BUZZER_BASE / freq - 1;
    TIM2_ARR = arr;
    TIM2_CCR2 = arr / 2; 
    TIM2_EGR |= 0x1;   
    TIM2_CR1 |= 0x1;     
}

void init_RGB(void) {
#if USE_RAW_REGISTER
    RCC_AHB1ENR |= 0x01;
    
    /* Configure PA8 as output (RGB LED pin) */
    GPIOA_MODER = (GPIOA_MODER & ~(0x3 << (16))) | (0x1 << (16));
    GPIOA_OTYPER &= ~(0x1 << 8);
    GPIOA_OSPEEDR = (GPIOA_OSPEEDR & ~(0x3 << 16)) | (0x3 << 16);
    GPIOA_PUPDR &= ~(0x3 << 16);

    /* Configure PA9 as output (RGB LED pin) */
    GPIOA_MODER = (GPIOA_MODER & ~(0x3 << (18))) | (0x1 << (18));
    GPIOA_OTYPER &= ~(0x1 << 9);
    GPIOA_OSPEEDR = (GPIOA_OSPEEDR & ~(0x3 << 18)) | (0x3 << 18);
    GPIOA_PUPDR &= ~(0x3 << 18);

    /* Configure PA10 as output (RGB LED pin) */
    GPIOA_MODER = (GPIOA_MODER & ~(0x3 << (20))) | (0x1 << (20));
    GPIOA_OTYPER &= ~(0x1 << 10);
    GPIOA_OSPEEDR = (GPIOA_OSPEEDR & ~(0x3 << 20)) | (0x3 << 20);
    GPIOA_PUPDR &= ~(0x3 << 20);
#else
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
#endif
}

void init_BP(void) { 
    /* Enable GPIOC clock and configure PC13 as input for Push Button */
    RCC_AHB1ENR |= 0x04;
    GPIOC_MODER &= ~(3 << 26);
    GPIOC_PUPDR &= ~(3<<26);
}

void button_irqPC13_init(void) {
#if USE_RAW_REGISTER
    RCC_APB2ENR |= 1 << 14;
    /* Set PC13 as EXTI13 input */
    SYSCFG_EXTICR4 = (SYSCFG_EXTICR4 & ~(0xf << 4)) | (0x2 << 4);
    /* Setup interrupt for EXTI13, falling edge */
    EXTI_IMR |= (1 << 13);
    EXTI_RTSR &= ~(1 << 13);
    EXTI_FTSR |= (1 << 13);
    EXTI_PR |= (1 << 13);
    /* Enable EXTI15-10 IRQ PC13 */
    NVIC_ISER(40 / 32) = (1 << (40 % 32));
#else
    rcc_periph_clock_enable(RCC_SYSCFG);
    exti_select_source(EXTI13, GPIOC);
    exti_enable_request(EXTI13);
    exti_set_trigger(EXTI13, EXTI_TRIGGER_FALLING);
    exti_reset_request(EXTI13);
    nvic_enable_irq(NVIC_EXTI15_10_IRQ);
    nvic_set_priority(NVIC_EXTI15_10_IRQ, 200);
#endif
}

void init_USART(void) {
#if USE_RAW_REGISTER
    RCC_AHB1ENR |= 0x01;
    /* Configure PA2 (TX) and PA3 (RX) for USART2 */
    GPIOA_MODER = (GPIOA_MODER & 0xFFFFFF0F) | 0x000000A0;
    GPIOA_AFRL = (GPIOA_AFRL & 0xFFFF00FF) | 0x00007700;
    USART2_BRR = get_APB1CLK() / 9600;
    USART2_CR3 = 0;
    USART2_CR2 = 0;
    USART2_CR1 = (1 << 2) | (1 << 3) | (1 << 13) | (1 << 5)| (1<<7);
#else
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART2);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);
    usart_set_baudrate(USART2, 9600);
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    usart_set_mode(USART2, USART_MODE_TX);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_enable(USART2);
#endif
}

void systick_init(uint32_t freq) {
#if USE_RAW_REGISTER
    uint32_t p = get_SYSCLK() / freq;
    STK_RVR = (p - 1) & 0x00FFFFFF;
    STK_CVR = 0;
    STK_CSR |= STK_CSR_CLKSOURCE | STK_CSR_TICKINT | STK_CSR_ENABLE;
#else
    systick_set_frequency(freq, get_SYSCLK());
    systick_counter_enable();
    systick_interrupt_enable();
#endif
}

inline static void memory_barrier(void) { __asm__ volatile("" ::: "memory"); }