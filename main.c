#include <stdio.h>
#include <string.h>
#include "hardware.h"
#include "notes.h"
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

/* Global State Architectures */
typedef struct {
    volatile uint32_t tick;
    volatile uint32_t ms;
    volatile uint32_t seconds;
    volatile uint32_t start_time;
    volatile uint32_t end_time;
    volatile uint32_t mode;
    volatile int round;
    volatile int results[3];
} GameState_t;

typedef struct {
    volatile uint32_t ticks;
    volatile uint32_t freq;
    volatile bool played;
    volatile uint32_t index;
} AudioState_t;

typedef struct {
    const char *sentence;
    volatile int position;
    volatile bool busy;
    volatile char buffer[256];
    volatile int cursor;
} UartState_t;

volatile GameState_t game = {0};
volatile AudioState_t audio = {0};
volatile UartState_t uart = {0};

/* Musical Scores */
uint32_t nb_notes_lose = 12;
uint32_t notes_lose[] = {
  NOTE_C5, NOTE_G4, NOTE_E4,                               
  NOTE_A4, NOTE_B4, NOTE_A4, NOTE_GS4, NOTE_AS4, NOTE_GS4, 
  NOTE_G4, NOTE_D4, NOTE_E4                                
};
uint32_t durations_lose[] = {
  450, 450, 300,       
  225, 225, 225, 225, 225, 225, 
  150, 150, 900      
};

uint32_t nb_notes_win = 50;
uint32_t notes_win[] = {
  NOTE_E5, NOTE_E5, PAUSE, NOTE_E5, PAUSE, NOTE_C5, NOTE_E5,
  NOTE_G5, PAUSE, NOTE_G4, PAUSE,
  NOTE_C5, NOTE_G4, PAUSE, NOTE_E4,
  NOTE_A4, NOTE_B4, NOTE_AS4, NOTE_A4,
  NOTE_G4, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_F5, NOTE_G5,
  PAUSE, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_B4,
  NOTE_C5, NOTE_G4, PAUSE, NOTE_E4,
  NOTE_A4, NOTE_B4, NOTE_AS4, NOTE_A4,
  NOTE_G4, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_F5, NOTE_G5,
  PAUSE, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_B4, PAUSE
};
uint32_t durations_win[] = {
  150, 150, 150, 150, 150, 150, 150,
  300, 300, 150, 300,
  450, 150, 300, 450,
  300, 300, 150, 300,
  225, 225, 225, 300, 150, 150,
  150, 300, 150, 150, 450,
  450, 150, 300, 450,
  300, 300, 150, 300,
  225, 225, 225, 300, 150, 150,
  150, 300, 150, 150, 450, 250
};

/* Interrupt Service Routines */
void sys_tick_handler() {
    game.tick++;
    game.ms++;
    if(game.tick >= 1000){
        game.seconds++;
        game.tick = 0;
    }
    if(audio.ticks > 0) {
        audio.ticks--;
        if(audio.ticks == 0 && audio.played) {
            play_buzzer(0); 
            audio.index++;
            audio.played = false;
        }
    }
}

void exti15_10_isr() { 
    static uint32_t last = 0;
    EXTI_PR = (1 << 13); // Clear Interrupt Flag

    if (game.ms - last > 200) { // Debounce threshold
        switch(game.mode){
            case 0:
                game.mode = 1;
                break;
            case 1:
                if(GPIOA_ODR & (1<<9)){ // If LED is ON
                    game.end_time = game.ms;
                    GPIOA_ODR  &= ~(1<<9); // Turn off LED
                    game.mode = 2;
                }
                else{
                    game.mode = 7; // Pressed early trigger
                }
                break;
            default:
                break;
        }
        last = game.ms;
    }
}

void usart2_isr(void){
    if (USART2_SR & USART_SR_TXE) {
        if((USART2_SR & (1<<7))){
            if(uart.sentence[uart.position] =='\0'){
                usart_disable_tx_interrupt(USART2);
                uart.busy = false;
            } else {
                USART2_DR= uart.sentence[uart.position];
                uart.position++;
            }
        }
    }
    if (USART2_SR & USART_SR_RXNE) {
        char c = USART2_DR;
        if (uart.cursor < 255) {
            uart.buffer[uart.cursor++] = c;
        }
    }
}

/* Base Functions */
void _putc(const char c) {
    while ((USART2_SR & 0x80) == 0);
    USART2_DR = c;
}

void _puts(const char *c) {
    int len = strlen(c);
    for (int i = 0; i < len; i++) {
        _putc(c[i]);
    }
}

char _getc(void) {
    char c = 'a';
    scanf(&c);
    return c;
}

int _async_puts(const char *s) {
    if(s == NULL){
        return uart.busy ? 1 : 0;
    }
    if(!uart.busy){
        uart.sentence = s;
        uart.busy = true;
        uart.position=0;
        usart_enable_tx_interrupt(USART2);
        return 0;
    }
    return 1;
}

void process_keyboard(void){
    usart_disable_rx_interrupt(USART2); 
    
    int local_cursor = uart.cursor; 
    uart.cursor = 0;            
    
    usart_enable_rx_interrupt(USART2);

    char letter;
    int positionR=0;
    while(positionR < local_cursor){
        letter = uart.buffer[positionR];
        switch(letter){
            case '\n':
            case '\r':
                switch(game.mode){
                    case 0:
                        game.mode = 1;
                        break;
                    default:
                        game.mode = 0;
                        break;
                }
            default:
                break;
        }
        positionR++;
    }
    uart.cursor=0;
}

void play_note(uint32_t freq, uint32_t duration_ms) {
    audio.freq = freq;
    audio.ticks = duration_ms;
    play_buzzer(freq);
    audio.played = true;
}

void play_song_win(void){
    audio.index = 0;
    while(audio.index < nb_notes_win){
        if(!audio.played){
            play_note(notes_win[audio.index], durations_win[audio.index]);
            audio.played = true;
        }
    }
    play_buzzer(0);
    audio.index = 0;
    audio.played = false;
}

void play_song_lose(void){
    audio.index = 0;
    while(audio.index < nb_notes_lose){
        if(!audio.played){
            play_note(notes_lose[audio.index], durations_lose[audio.index]);
            audio.played = true;
        }
    }
    play_buzzer(0);
    audio.index = 0;
    audio.played = false;
}

int main(void) {
    printf("\e[2J\e[1;1H\r\n");
    _async_puts("The game will test your reflexes over 3 rounds.\n\rPress Enter to start.\n\r");

    /* Initializations */
    init_RGB();
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_TIM2);

    buzzer_pwm_init();
    systick_init(1000); // SysTick 1 ms
    init_BP();
    init_USART();
    
    nvic_enable_irq(NVIC_USART2_IRQ);
    button_irqPC13_init();

    uint32_t random_time = 0;
    uint32_t final_time = 0;
    bool state_changed = false;
    
    while (1){
        process_keyboard();
        
        switch(game.mode){
            case 0:
                if(!state_changed){
                    _async_puts("\n\rPress the push button on the motherboard\n\r");
                    state_changed = true;
                }
                break;
            
            case 1:
                if(state_changed){
                    _async_puts("Started\n\r");
                    state_changed = false;
                }
                random_time = game.ms % 15000 + 1;
                if(random_time > 5000 && (game.seconds * 1000 % random_time == 0)){
                    GPIOA_ODR |= (1<<9); // Turn on LED
                    game.start_time = game.ms;
                }
                break;
                
            case 2:
                final_time = game.end_time - game.start_time;
                game.results[game.round] = final_time;
                game.round++;
                printf("Your time: %ld ms\n\r", final_time);
                if(game.round >= 3){
                    game.mode = 5;
                } else {
                    game.mode = 4;
                }
                break;
                
            case 3:
                play_song_win();
                _async_puts("\n\rNew round\n\rPress the push button on the motherboard\n\r");
                game.mode = 4;
                break;
                
            case 4:
                game.end_time = 0;
                game.start_time = 0;
                game.mode = 0;
                if(game.round >= 3){
                    game.round = 0;
                }
                break;
                
            case 5:
                int sum = (game.results[0] + game.results[1] + game.results[2]) / 3;
                if(sum < 300){
                    printf("Your average is: %i ms\n\rCongratulations!\n\r", sum);
                    game.mode = 3;
                } else {
                    printf("Your average is: %i ms\n\rYou lost!\n\r", sum);
                    game.mode = 6;
                }
                break;
                
            case 6:
                play_song_lose();
                game.mode = 4;
                _async_puts("\n\rNew round\n\rPress the push button on the motherboard\n\r");
                break;
                
            case 7:
                _async_puts("Too fast! Restarting from the beginning\n\r");
                play_song_lose();
                game.round = 3; // Forces reset condition in case 4
                game.mode = 4;
                _async_puts("\n\rNew round\n\rPress the push button on the motherboard\n\r");
                break;
                
            default:
                break;
        }
    }
    return 0;
}