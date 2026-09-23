#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "filesystem/vfs.h"
#include "audio.pio.h"

#define DATA_PIN 13
#define SIDESET_BASE 14

#define SAMPLE_RATE 44100.0
#define PIO_CYCLES_PER_SAMPLE 64.0

#define BUFFER_SIZE 32768
static int16_t bufferA[BUFFER_SIZE];
static int16_t bufferB[BUFFER_SIZE];

static void setup_audio_pio(PIO pio, uint sm, uint offset)
{
    pio_sm_config config = audio_program_get_default_config(offset);
    sm_config_set_out_pins(&config, DATA_PIN, 1);
    sm_config_set_sideset_pin_base(&config, SIDESET_BASE);
    sm_config_set_out_shift(&config, false, true, 32);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
    
    pio_gpio_init(pio, DATA_PIN);
    pio_gpio_init(pio, SIDESET_BASE);
    pio_gpio_init(pio, SIDESET_BASE + 1);
    
    pio_sm_set_consecutive_pindirs(pio, sm, DATA_PIN, 1, GPIO_OUT);
    pio_sm_set_consecutive_pindirs(pio, sm, SIDESET_BASE, 2, GPIO_OUT);
    
    float divider = ((float)clock_get_hz(clk_sys)) / (float)(SAMPLE_RATE * PIO_CYCLES_PER_SAMPLE);
    sm_config_set_clkdiv(&config, divider);
    
    pio_sm_init(pio, sm, offset, &config);
    pio_sm_set_enabled(pio, sm, true);
}

void core1_main() {
    size_t items_read = 0;
    bool mounted = fs_init();
    if (!mounted) {
        while (true) {
            printf("failed to mount filesystem\n");
        }
    }

    FILE *file = fopen("/PLAYLIST4.raw", "r");
    if (!file) {
        while (true) {
            printf("failed to open file\n");
        }
    }

    bool selected_buffer = false; // false buffer A, true buffer B
    uint16_t *selected_buffer_pointer = bufferA;

    while (true) {
        if (selected_buffer == false) {
            items_read = fread(bufferA, sizeof(int16_t), BUFFER_SIZE, file);
        } else {
            items_read = fread(bufferB, sizeof(int16_t), BUFFER_SIZE, file);
        }
        if (items_read == 0) break;

        multicore_fifo_push_blocking((uint32_t)selected_buffer_pointer);
        multicore_fifo_pop_blocking(); // wait for response from other core

        selected_buffer = !selected_buffer;
        selected_buffer_pointer = selected_buffer ? bufferB : bufferA;
    }

    while(true) {
        printf("finished reading\n");
        sleep_ms(1000);
    }
}

int main()
{
    set_sys_clock_khz(200000, true);
    stdio_init_all();

    PIO pio;
    uint sm;
    uint offset;
    pio_claim_free_sm_and_add_program(&audio_program, &pio, &sm, &offset);

    setup_audio_pio(pio, sm, offset);

    multicore_launch_core1(core1_main);

    while (true) {
        uint16_t *audio_buffer = (uint16_t *)multicore_fifo_pop_blocking();

        multicore_fifo_push_blocking(1); // tell core 1 that pointer to buffer has been received

        for (int i = 0; i < BUFFER_SIZE; i++) {
            uint32_t sample = (uint32_t)((uint16_t)audio_buffer[i] << 16) | (uint16_t)(audio_buffer[i]);
            pio_sm_put_blocking(pio, sm, sample);
        }
    }


    while(true) {
        printf("finished playing\n");
        sleep_ms(1000);
    }
}
