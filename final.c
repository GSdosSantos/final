#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "lib/musica.h"

// UART defines
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1
// Define os pinos
#define PIN_BUZZER 21
#define PIN_MIC 28
#define PIN_LEDG 11
#define PIN_BA 5
#define PIN_BB 6
#define PIN_BC 22
#define PIN_X 27
#define PIN_Y 26
// Configurações do ADC
#define ADC_CHANNEL 2 // Canal ADC para o GPIO 28 (ADC2)
#define ADC_RANGE 4096 // Resolução do ADC (12 bits)
#define TAM 493920
#define VOLUME 100


uint slice;
int i=0;
static int faixa =0;
static int volume = 50;
uint16_t vr_x; // valor analogico lido
uint16_t vr_y;

//funções
uint32_t divisor(uint32_t freq){
    return 125000000/(1000*freq);
}
// funções para pinos digitais
void botoes1(uint gpio, uint32_t eventos) {
    static absolute_time_t ultimo_tempo_a = 0;
    static absolute_time_t ultimo_tempo_b = 0;

    if (gpio == PIN_BA) {
        static absolute_time_t ultimo_tempo_interrupcao = 0;
        absolute_time_t agora = get_absolute_time();

        // Debounce
        if (absolute_time_diff_us(ultimo_tempo_interrupcao, agora) > 200000) {

        }
        ultimo_tempo_interrupcao = agora;
    } else if (gpio == PIN_BB) {
        static absolute_time_t ultimo_tempo_interrupcao = 0;
        absolute_time_t agora = get_absolute_time();

        // Debounce
        if (absolute_time_diff_us(ultimo_tempo_interrupcao, agora) > 200000) {

        }
        ultimo_tempo_interrupcao = agora;
    } else if (gpio == PIN_BC) {
        static absolute_time_t ultimo_tempo_interrupcao = 0;
        absolute_time_t agora = get_absolute_time();

        // Debounce
        if (absolute_time_diff_us(ultimo_tempo_interrupcao, agora) > 200000) {

        }
        ultimo_tempo_interrupcao = agora;
    }
}
// funções para pinos analogicos
bool botoes2(struct repeating_timer *t) {
    adc_select_input(PIN_X -26);
    vr_x = adc_read();
    adc_select_input(PIN_Y -26);
    vr_y = adc_read();
    // alterna a faixa
    if(vr_x>3000){
        faixa = (faixa+1)%QUANT_MUSICAS;
        i=0;
        printf("Faixa: %d\n",faixa);
    }
    if(vr_x>1000){
        faixa = (faixa-1+10)%QUANT_MUSICAS;
        i=0;
        printf("Faixa: %d\n",faixa);
    }
    // altera volume
    if(vr_y>3000){
        volume = (volume+1)%VOLUME;
        pwm_set_chan_level(slice,PWM_CHAN_B,10*volume); // ciclo de trabalho = volume
        printf("Volume: %d\n",volume);
    }
    if(vr_y>1000){
        volume = (volume-1+10)%VOLUME;
        pwm_set_chan_level(slice,PWM_CHAN_B,10*volume); // ciclo de trabalho = volume
        printf("Volume: %d\n",volume);
    }
    return true;
}
void config_uart(){
    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_puts(UART_ID, " Hello, UART!\n");
}
void config_pwm(){
    gpio_set_function(PIN_BUZZER, GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(PIN_BUZZER);
    //pwm_set_clkdiv(slice, 0.11f); // (125MHz / (44.1kHz * 4095) = )
    pwm_set_wrap(slice, 1000);    // 1 wrap = 1us 
    pwm_set_enabled(slice, true);  // Habilita o PWM
    pwm_set_chan_level(slice,PWM_CHAN_B,10*volume); // ciclo de trabalho = volume
}
void config_adc(){
    adc_init();
    adc_gpio_init(PIN_X);
    adc_gpio_init(PIN_Y);
}
void config_pinos() {
    gpio_init(PIN_LEDG);
    gpio_set_dir(PIN_LEDG, GPIO_OUT);
    gpio_init(PIN_BA);
    gpio_set_dir(PIN_BA, GPIO_IN);
    gpio_pull_up(PIN_BA);
    gpio_init(PIN_BB);
    gpio_set_dir(PIN_BB, GPIO_IN);
    gpio_pull_up(PIN_BB);
    gpio_init(PIN_BC);
    gpio_set_dir(PIN_BC, GPIO_IN);
    gpio_pull_up(PIN_BC);
    gpio_set_irq_enabled_with_callback(PIN_BA, GPIO_IRQ_EDGE_FALL, true, &botoes1);
    gpio_set_irq_enabled_with_callback(PIN_BB, GPIO_IRQ_EDGE_FALL, true, &botoes1);
    gpio_set_irq_enabled_with_callback(PIN_BB, GPIO_IRQ_EDGE_FALL, true, &botoes1);  
}
int main(){
    stdio_init_all();
    config_pwm();
    config_adc();
    config_uart();

    repeating_timer_t timer;
    add_repeating_timer_ms(200, botoes2,NULL, &timer);
    while (true){
        pwm_set_clkdiv(slice,divisor(notas[faixa][i]));
        sleep_ms(periodo[faixa][i]);
        i=(i+1)%tamnote[faixa];
    }
    
}

