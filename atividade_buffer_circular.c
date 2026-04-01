#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "driver/adc.h"

static const char *TAG = "CB";

static volatile uint32_t isr_count = 0;
static volatile int adc_raw;

typedef struct {
  int *buffer;
  int size;
  unsigned int head;
  unsigned int tail;
} circular_buffer;

circular_buffer cbuf;

void cb_init (circular_buffer *buf, int size) {
   buf->buffer = (int *) malloc (size*sizeof(int));
   buf->head = 0;
   buf->tail = 0;
   buf->size = size;
}

int cb_get_avail (circular_buffer *buf) {
   return buf->size - (buf->head - buf->tail);
}

int cb_get_filled (circular_buffer *buf) {
   return buf->head - buf->tail;
}

int cb_push (circular_buffer *buf, int *data, int N) {
    int i;
    if (cb_get_avail (buf)< N)
        return 0;

    for (i=0; i<N; i++) {
      buf->buffer[buf->head % buf->size] = data[i];
      buf->head += 1;
    }
    return i;
}

int cb_pop (circular_buffer *buf, int *data, int N) {
    int i;
    if (buf->head - buf->tail < N)
        return 0;

    for (i=0; i<N; i++) {
      data[i] = buf->buffer[buf->tail % buf->size];
      buf->tail += 1;
    }
    return i;
}




static bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer,
                                        const gptimer_alarm_event_data_t *edata,
                                        void *user_ctx)
{
    isr_count++;
    adc_raw = adc1_get_raw(ADC1_CHANNEL_6);
    cb_push (&cbuf, &adc_raw, 1);
    return false;  // não solicita troca de contexto
}

void timer_adc_config () {
    gptimer_handle_t gptimer = NULL;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz -> 1 tick = 1 us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_on_alarm_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = 10000,     // 10.000 us = 10 ms
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));



    // Configuração da unidade ADC
    adc1_config_width(ADC_WIDTH_BIT_12);

    // Configura atenuação do canal
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // GPIO34

}



void app_main() {

  int media = 0;

  int vetor[64];
  timer_adc_config();

  cb_init (&cbuf, 1000);

  while (true) {
        if (cb_get_filled(&cbuf) >=64 ) {
            cb_pop(&cbuf, vetor,64);
            for (int i=0; i<64; i++)
              media += vetor[i];
            media /= 64;
            if (media> 2000)
                printf( "Media: %d\n", media);
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
