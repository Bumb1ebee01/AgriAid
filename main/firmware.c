#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "sensor_manager.h"
#include "irrigation_controller.h"
#include "ml_inference.h"
#include "web_server.h"
#include "led_status.h"

static const char *TAG = "AGRIAID";

static sensor_data_t sensor_data;
static controller_status_t controller_status;
static ml_result_t ml_result;

static void agri_task(void *arg)
{
    (void)arg;

    while (1) {

        /*
         * -----------------------------------------
         * 1. READ SENSORS
         * -----------------------------------------
         */

        sensors_update(&sensor_data);

        /*
         * -----------------------------------------
         * 2. TINYML
         * -----------------------------------------
         */

        ml_result = ml_predict(
            sensor_data.soil_moisture,
            sensor_data.temperature,
            sensor_data.humidity,
            sensor_data.flow_rate,
            sensor_data.is_day
        );

        /*
         * -----------------------------------------
         * 3. DETERMINISTIC SAFETY + IRRIGATION
         * -----------------------------------------
         */

        controller_update(
            &sensor_data,
            &controller_status
        );

        /*
         * -----------------------------------------
         * 4. UPDATE WEB DASHBOARD
         * -----------------------------------------
         */

        web_server_update_state(
            &sensor_data,
            &controller_status,
            &ml_result
        );

        /*
         * -----------------------------------------
         * 5. SERIAL LOG
         * -----------------------------------------
         */

        ESP_LOGI(TAG, "----------------------------------------");

        ESP_LOGI(
            TAG,
            "Soil        : %.1f %%",
            sensor_data.soil_moisture
        );

        ESP_LOGI(
            TAG,
            "Temperature : %.1f C",
            sensor_data.temperature
        );

        ESP_LOGI(
            TAG,
            "Humidity    : %.1f %%",
            sensor_data.humidity
        );

        ESP_LOGI(
            TAG,
            "Flow        : %.2f L/min",
            sensor_data.flow_rate
        );

        ESP_LOGI(
            TAG,
            "Light       : %s",
            sensor_data.is_day ? "DAY" : "NIGHT"
        );

        ESP_LOGI(
            TAG,
            "Pump        : %s",
            controller_status.pump_state == PUMP_ON
                ? "ON"
                : "OFF"
        );

        ESP_LOGI(
            TAG,
            "Irrigation  : %.2f",
            controller_status.irrigation_score
        );

        ESP_LOGI(
            TAG,
            "Plant ML    : %s",
            ml_result.model_ready
                ? "READY"
                : "NOT LOADED"
        );

        /*
         * -----------------------------------------
         * LOOP DELAY
         * -----------------------------------------
         */

        vTaskDelay(
            pdMS_TO_TICKS(2000)
        );
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "          AgriAid v0.2");
    ESP_LOGI(TAG, "          ESP32-S3-N16R8");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    /*
     * -----------------------------------------
     * SENSOR SYSTEM
     * -----------------------------------------
     */

    sensors_init();

    /*
     * -----------------------------------------
     * IRRIGATION CONTROLLER
     * -----------------------------------------
     */

    controller_init();

    /*
     * -----------------------------------------
     * ONBOARD RGB LED
     * -----------------------------------------
     */

    led_status_init();
    led_status_skyblue();

    /*
     * -----------------------------------------
     * TINYML
     * -----------------------------------------
     */

    if (!ml_init()) {
        ESP_LOGE(
            TAG,
            "TinyML initialization failed"
        );
    }

    /*
     * -----------------------------------------
     * WIFI + WEB SERVER
     * -----------------------------------------
     */

    web_server_start();

    /*
     * -----------------------------------------
     * MAIN AGRIAID TASK
     * -----------------------------------------
     */

    xTaskCreate(
        agri_task,
        "agri_task",
        8192,
        NULL,
        5,
        NULL
    );

    ESP_LOGI(TAG, "AgriAid system started");
}