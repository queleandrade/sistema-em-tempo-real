// Utilizando a implementação de buffer circular desenvolvida anteriormente 
// (com as funções get_size(), push() e pop()), implemente um sistema no ESP32 
// que calcula a média móvel de um sinal adquirido pelo ADC.

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"

#define BUFFER_SIZE 64
#define WINDOW 16
#define TIMER_INTERVAL 10000
#define LIMIAR 2000

typedef struct {
    int *buffer;
    int size;
    int head;
    int tail;
} BufferCircular;

BufferCircular estrutura;
adc_oneshot_unit_handle_t adc_handle;
static TaskHandle_t adc_task_handle = NULL;

int getSize(BufferCircular *p) {
    return (p->head - p->tail);
}
int vazioBuffer(BufferCircular *p) {
    return p->head == p->tail;
}
int cheioBuffer(BufferCircular *p) {
    return (p->head % p->size == p->tail % p->size) && (p->head != p->tail);
}
int lerBuffer(BufferCircular *p) {
    if (vazioBuffer(p)) return 0;
    int val = p->buffer[p->tail % p->size];
    p->tail++;
    return val;
}
void escreverNoBuffer(BufferCircular *p, int valor) {
    if (cheioBuffer(p)) return;
    p->buffer[p->head % p->size] = valor;
    p->head++;
}
int pop(BufferCircular *cb, int *value) {
    if (vazioBuffer(cb)) return -1;

    *value = cb->buffer[cb->tail % cb->size];
    cb->tail++;
    return 0;
}
void init_adc(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &chan_cfg)); 
}



static bool IRAM_ATTR timer_callback(gptimer_handle_t timer,
                                     const gptimer_alarm_event_data_t *edata,
                                     void *user_ctx) {
    BaseType_t high_task_woken = pdFALSE;
    vTaskNotifyGiveFromISR(adc_task_handle, &high_task_woken);
    return high_task_woken == pdTRUE;
}

void adc_task(void *arg) {

    float soma = 0;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        int amostra = 0;
        adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &amostra); 
        escreverNoBuffer(&estrutura, amostra);
        if (getSize(&estrutura) >= WINDOW) {
            int antigo;
            if (pop(&estrutura, &antigo) == 0) {
                soma -= antigo;
            }
        }

        // printf("Amostra lida: %d\n", amostra); 

        // if (amostra > LIMIAR) {
        //     printf("ALERTA: amostra %d maior que o limiar!\n", amostra);
        // }
        soma += amostra;

        int tamanho = getSize(&estrutura);

        float media = soma / tamanho;
        if (media > LIMIAR) {
            printf("Média móvel: %.2f\n", media);
        }

        


    }
}

void init_timer(void) {
    gptimer_handle_t timer = NULL;

    gptimer_config_t config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&config, &timer));

    gptimer_alarm_config_t alarm = {
        .alarm_count = TIMER_INTERVAL,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_callback,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL));

    ESP_ERROR_CHECK(gptimer_enable(timer));
    ESP_ERROR_CHECK(gptimer_start(timer));
}

void app_main(void) {
    estrutura.size   = 1000;
    estrutura.buffer = (int *)malloc(estrutura.size * sizeof(int));
    estrutura.head   = 0;
    estrutura.tail   = 0;

    init_adc();
    xTaskCreate(adc_task, "adc_task", 4096, NULL, 5, &adc_task_handle);
    init_timer();

    // while (1) {
    //     if (getSize(&estrutura) >= 64) {
    //         float soma = 0;
    //         for (int i = 0; i < 64; i++) {
    //             soma += lerBuffer(&estrutura);
    //         }
    //         printf("Potência média: %.2f\n", soma / 64.0f);
    //     }
    //     vTaskDelay(pdMS_TO_TICKS(10));
    // }
}
