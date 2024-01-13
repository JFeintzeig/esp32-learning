/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "color_cycle";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO
#define CYCLESPEED_MS 100

static led_strip_handle_t led_strip;

static void set_led(uint8_t s_led_state, uint8_t red, uint8_t blue, uint8_t green)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, red, blue, green);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

void app_main(void)
{

    /* Configure the peripheral according to the LED type */
    configure_led();

    static uint8_t s_led_state = 1;

    // counter goes from -> period_ms
    // red goes from 0 -> FF
    // green goes from FF -> 0
    uint8_t red = 0, green = 0, blue = 0;

    // goes from R -> G -> B in 3x this
    uint32_t period_ms = 3000;
    uint32_t counter = 0;
    uint8_t period_number = 0;

    while (1) {
        if (period_number == 0) {
            red = 0xFF * ((float)counter / period_ms);
            blue = ~red;
            green = 0x00;
        } else if (period_number == 1) {
            green = 0xFF * ((float)counter / period_ms);
            red = ~green;
            blue = 0x00;
        } else if (period_number == 2) {
            red = 0x00;
            blue = 0xFF * ((float)counter / period_ms);
            green = ~blue;
        }

        set_led(s_led_state, red, green, blue);
        //printf("set LED period_ms %lu period_number %x counter %lu color: %x %x %x\n", period_ms, period_number, counter, red, green, blue);

        counter++;
        if (counter == period_ms) {
            ESP_LOGI(TAG, "incrementing period number");
            period_number++;
            period_number %= 3;
        }
        counter %= period_ms;

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}
