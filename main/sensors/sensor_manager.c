#include "sensor_manager.h"

#include "esp_log.h"

static const char *TAG = "SENSORS";

static float simulated_soil = 28.0f;
static bool simulated_day = true;

void sensors_init(void)
{
    ESP_LOGI(TAG, "Initializing sensor manager");

    ESP_LOGI(TAG, "DHT22       -> GPIO4");
    ESP_LOGI(TAG, "Soil ADC    -> GPIO5");
    ESP_LOGI(TAG, "YF-S201     -> GPIO6");
    ESP_LOGI(TAG, "Relay       -> GPIO7");
    ESP_LOGI(TAG, "I2C SDA     -> GPIO8");
    ESP_LOGI(TAG, "I2C SCL     -> GPIO9");
    ESP_LOGI(TAG, "LDR         -> GPIO15");

    ESP_LOGI(TAG, "Sensor hardware currently in SIMULATION mode");
}

void sensors_update(sensor_data_t *data)
{
    if (data == NULL) {
        return;
    }

    /*
     * Temporary simulated environment.
     *
     * Tomorrow the DHT22 values will come from the real sensor.
     */

    data->soil_moisture = simulated_soil;

    data->temperature = 27.5f;

    data->humidity = 61.0f;

    data->is_day = simulated_day;

    /*
     * Simulate water flow whenever the pump is expected
     * to be operating.
     *
     * For now we use a fixed value so the safety controller
     * can be tested independently.
     */
    data->flow_rate = 0.0f;

    /*
     * Keep the simulated soil value inside a realistic range.
     */
    if (simulated_soil < 20.0f) {
        simulated_soil = 20.0f;
    }

    if (simulated_soil > 70.0f) {
        simulated_soil = 70.0f;
    }
}