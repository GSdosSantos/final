#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "lib/musica.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/time.h"

// I2C defines
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
// Definições do display
#define LARGURA 128
#define ALTURA 64
#define PORTA_I2C i2c1
#define PINO_SDA 14
#define PINO_SCL 15
#define ENDERECO_OLED 0x3C
ssd1306_t display;
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
#define LIM_SUP 3500
#define LIM_INF 500

//Variaveis
uint slice;
int i=0;
static int faixa =0;
static int volume = 50;
uint16_t vr_x; // valor analogico lido
uint16_t vr_y;
char txt_faixa[13];
char txt_vol [20];
bool e_display = true;
bool play = true; // 
repeating_timer_t timer0;
repeating_timer_t timer1;

wave[] = [20, 30, 40, 25, 10, 15, 35, 5, 0, 20, 25, 38, 40, 30, 15, 10, 20, 35, 5, 20];
//funções
uint32_t divisor(uint32_t freq){
    return 125000000/(1000*freq);
}
// função que imprime no display
bool displayon(struct repeating_timer *t) {
    ssd1306_fill(&display, false);
    for(int j=0; j<20;j++){
        ssd1306_vline(&display,4 + 5*j,35-wave[j],35,true);
        ssd1306_vline(&display,5 + 5*j,35-wave[j],35,true);
        ssd1306_vline(&display,6 + 5*j,35-wave[j],35,true);
        ssd1306_vline(&display,7 + 5*j,35-wave[j],35,true);
        ssd1306_vline(&display,8 + 5*j,35-wave[j],35,true);
    }
    sprintf(txt_faixa, "Faixa: %d", faixa);
    ssd1306_draw_string(&display, txt_faixa, 10, 50);
    sprintf(txt_vol, "Volume: %d", volume);
    ssd1306_draw_string(&display, txt_vol, 10, 40);
    ssd1306_send_data(&display);
    return true;
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
            e_display=!e_display;
            gpio_put(PIN_LEDG,e_display);
            if(e_display){
                add_repeating_timer_ms(500, displayon,NULL, &timer1);
            }else{
                cancel_repeating_timer(&timer1);
                ssd1306_fill(&display, false);
                ssd1306_send_data(&display);
            }
        }
        ultimo_tempo_interrupcao = agora;
    } else if (gpio == PIN_BC) {
        static absolute_time_t ultimo_tempo_interrupcao = 0;
        absolute_time_t agora = get_absolute_time();

        // Debounce
        if (absolute_time_diff_us(ultimo_tempo_interrupcao, agora) > 200000) {
            play=!play;
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
    if(vr_x>LIM_SUP){
        faixa = (faixa+1)%QUANT_MUSICAS;
        i=0;
    }
    if(vr_x<LIM_INF){
        faixa = (faixa-1+10)%QUANT_MUSICAS;
        i=0;
    }
    // altera volume
    if(vr_y>LIM_SUP){
        volume = (volume+1);
        if(volume>100){
            volume=100;
        }
        pwm_set_chan_level(slice,PWM_CHAN_B,10*volume); // ciclo de trabalho = volume
    }
    if(vr_y<LIM_INF){
        volume = (volume-1);
        if(volume<0){
            volume=0;
        }
        pwm_set_chan_level(slice,PWM_CHAN_B,10*volume); // ciclo de trabalho = volume
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
    gpio_set_irq_enabled_with_callback(PIN_BC, GPIO_IRQ_EDGE_FALL, true, &botoes1);  
}
void config_i2c(){
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);   
}
void config_display() {
   ssd1306_init(&display, LARGURA, ALTURA, false, ENDERECO_OLED, PORTA_I2C);
   ssd1306_config(&display);
   ssd1306_fill(&display, false);
   ssd1306_send_data(&display);
}
int main(){
    stdio_init_all();
    config_pwm();
    config_adc();
    config_uart();
    config_i2c();
    config_display();
    config_pinos();

    add_repeating_timer_ms(200, botoes2,NULL, &timer0);
    add_repeating_timer_ms(500, displayon,NULL, &timer1);
    while (true){
        if(play){
            if(notas[faixa][i]==0){
                pwm_set_enabled(slice, false);
            }else{
                pwm_set_clkdiv(slice,divisor(notas[faixa][i]));
                pwm_set_enabled(slice, true);
            }
            sleep_ms(periodo[faixa][i]);
            i=(i+1)%tamnote[faixa];
            //fim de musica
            if(i==0){
                pwm_set_enabled(slice, false);
                sleep_ms(1000);
            }
        }
        
    }
    
}

