#include "led_status.h"

#include "esp_log.h"
#include "led_strip.h"

#define RGB_LED_GPIO 48
#define RGB_LED_COUNT 1

// Sky Blue: #87CEEB
#define SKY_BLUE_R 135
#define SKY_BLUE_G 206
#define SKY_BLUE_B 235

static const char *TAG = "LED";

static led_strip_handle_t led_strip = NULL;

void led_status_init(void)
{
    ESP_LOGI(TAG, "Initializing onboard WS2812 RGB LED");

    led_strip_config_t strip_config = {
        .strip_gpio_num = RGB_LED_GPIO,
        .max_leds = RGB_LED_COUNT,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 0,
        .flags = {
            .with_dma = false,
        },
    };

    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(
            &strip_config,
            &rmt_config,
            &led_strip
        )
    );

    led_status_off();

    ESP_LOGI(TAG, "RGB LED ready on GPIO%d", RGB_LED_GPIO);
}

void led_status_skyblue(void)
{
    if (led_strip == NULL) {
        return;
    }

    ESP_ERROR_CHECK(
        led_strip_set_pixel(
            led_strip,
            0,
            SKY_BLUE_R,
            SKY_BLUE_G,
            SKY_BLUE_B
        )
    );

    ESP_ERROR_CHECK(led_strip_refresh(led_strip));

    ESP_LOGI(TAG, "RGB LED -> SKY BLUE (#87CEEB)");
}

void led_status_off(void)
{
    if (led_strip == NULL) {
        return;
    }

    ESP_ERROR_CHECK(led_strip_clear(led_strip));
}