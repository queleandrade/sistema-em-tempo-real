#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"

#define INTERVALO_TIMER 10000
#define LIMITE_AMOSTRA 2000
#define TAMANHO_BLOCO 64

int blocoA[TAMANHO_BLOCO];
int blocoB[TAMANHO_BLOCO];

int *bloco_escrita = blocoA;
int *bloco_leitura = blocoB;

volatile int posicao_buffer = 0;
volatile int bloco_completo = 0;
volatile int flag_troca = 0;

int id_escrita = 0;
int id_leitura = 1;

adc_oneshot_unit_handle_t handle_adc;
static TaskHandle_t handle_tarefa_adc = NULL;

// Callback do timer (ISR): acorda a tarefa do ADC a cada intervalo configurado
static bool IRAM_ATTR callback_timer(gptimer_handle_t timer,
                                     const gptimer_alarm_event_data_t *edata,
                                     void *user_ctx) {
    BaseType_t tarefa_acordada = pdFALSE;
    vTaskNotifyGiveFromISR(handle_tarefa_adc, &tarefa_acordada);
    return tarefa_acordada == pdTRUE;
}

// Tarefa responsável por ler o ADC periodicamente e armazenar em buffer duplo.
// Quando o buffer enche, troca os buffers (double buffering) e sinaliza processamento.
void tarefa_adc(void *arg) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        int amostra = 0;
        adc_oneshot_read(handle_adc, ADC_CHANNEL_6, &amostra);

        if (amostra > LIMITE_AMOSTRA) {
            printf("Amostra %d acima do limite, descartada\n", amostra);
        } else {
            bloco_escrita[posicao_buffer++] = amostra;

            if (posicao_buffer >= TAMANHO_BLOCO) {

                int *temp = (int *)bloco_escrita;
                bloco_escrita = bloco_leitura;
                bloco_leitura = temp;

                int tmp_id = id_escrita;
                id_escrita = id_leitura;
                id_leitura = tmp_id;

                posicao_buffer = 0;
                bloco_completo = 1;
                flag_troca = 1;
            }
        }
    }
}

// Inicializa o ADC no modo oneshot (leitura sob demanda) no canal 6
void init_adc(void) {
    adc_oneshot_unit_init_cfg_t cfg_unidade = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&cfg_unidade, &handle_adc));

    adc_oneshot_chan_cfg_t cfg_canal = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(handle_adc, ADC_CHANNEL_6, &cfg_canal));
}

// Configura e inicia um timer de hardware que dispara periodicamente e chama o callback
void init_timer(void) {
    gptimer_handle_t timer = NULL;

    gptimer_config_t config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&config, &timer));

    gptimer_alarm_config_t alarme = {
        .alarm_count = INTERVALO_TIMER,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarme));

    gptimer_event_callbacks_t callbacks = {
        .on_alarm = callback_timer,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &callbacks, NULL));

    ESP_ERROR_CHECK(gptimer_enable(timer));
    ESP_ERROR_CHECK(gptimer_start(timer));
}

// Função principal: inicializa ADC, cria a tarefa de leitura e inicia o timer.
// No loop, processa blocos completos calculando a média das amostras.
void app_main(void) {

    init_adc();
    xTaskCreate(tarefa_adc, "tarefa_adc", 4096, NULL, 5, &handle_tarefa_adc);
    init_timer();

    while (1) {

        if (flag_troca) {
            flag_troca = 0;
            printf("\n=== TROCA DE BUFFER ===\n");
            printf("Escrevendo em: %c\n", id_escrita == 0 ? 'A' : 'B');
            printf("Lendo de:      %c\n", id_leitura == 0 ? 'A' : 'B');
        }

        if (bloco_completo) {
            bloco_completo = 0;

            float soma = 0;
            printf("Processando buffer %c\n", id_leitura == 0 ? 'A' : 'B');

            for (int i = 0; i < TAMANHO_BLOCO; i++) {
                soma += bloco_leitura[i];
            }

            float media = soma / TAMANHO_BLOCO;
            printf("Média das amostras: %.2f\n", media);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
