/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO
#define CYCLESPEED_MS 100

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static void blink_led(uint8_t s_led_state, uint32_t color)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
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
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

// make it so it smoothly goes from R -> G -> B -> R
// over period each color goes fully on and fully off
// each color is offset by period/3
uint32_t set_color_by_phase(uint8_t current_cycle, uint8_t period) {
  uint8_t red, green, blue;
  uint8_t red_cycle = current_cycle;
  uint8_t blue_cycle = current_cycle + period/3;
  uint8_t green_cycle = current_cycle + 2*period/3;
  if (red_cycle < period / 2) {
    red = 0xFF * (red_cycle * 1.0 / period / 2);
  } else {
    red = 0xFF * ~(int)(red_cycle * 1.0 / period / 2);
  }
  if (green_cycle < period / 2) {
    green = 0xFF * (green_cycle * 1.0 / period / 2);
  } else {
    green = 0xFF * ~(int)(green_cycle * 1.0 / period / 2);
  }
  return ((uint32_t)red << 16) | ((uint32_t)green << 8) | ((uint32_t)blue);
}

void app_main(void)
{

    /* Configure the peripheral according to the LED type */
    configure_led();

    static uint8_t s_led_state = 0;


    uint8_t phase = 0;
    color_t color = {0xFF, 0xFF, 0xFF};

    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        set_color_by_phase(&color, phase);
        blink_led(s_led_state, &color);
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        if (!s_led_state) {
          phase += 1;
          phase %= 3;
        }
    }
}
