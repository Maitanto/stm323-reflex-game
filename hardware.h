#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

#ifndef USE_RAW_REGISTER
#define USE_RAW_REGISTER 1
#endif

#define BUZZER_BASE 84000 

void buzzer_pwm_init(void);
void play_buzzer(uint32_t freq);
void init_RGB(void);
void init_BP(void);
void button_irqPC13_init(void);
void init_USART(void);
void systick_init(uint32_t freq);

#endif // HARDWARE_H