#include "data_logger.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_littlefs.h"
#include "esp_timer.h"

#define LITTLEFS_PARTITION_LABEL "storage"
#define LOG_FILE_PATH "/storage/agriaid_sensor_log.csv"

static const char *TAG = "DATA_LOGGER";

static bool logger_ready = false;

static void create_csv_header(void)
{
    FILE *file = fopen(LOG_FILE_PATH, "r");

    if (file != NULL) {
        fclose(file);
        return;
    }

    file = fopen(LOG_FILE_PATH, "w");

    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to create log file");
        return;
    }

    fprintf(
        file,
        "timestamp_us,"
        "soil_moisture,"
        "temperature,"
        "humidity,"
        "flow_rate,"
        "is_day,"
        "pump_state,"
        "irrigation_event,"
        "plant_stage,"
        "plant_stress_label\n"
    );

    fclose(file);

    ESP_LOGI(TAG, "Created CSV log file");
}

void data_logger_init(void)
{
    ESP_LOGI(TAG, "Initializing LittleFS data logger");

    const esp_vfs_littlefs_conf_t conf = {
        .base_path = "/storage",
        .partition_label = LITTLEFS_PARTITION_LABEL,
        .format_if_mount_failed = true,
        .grow_on_mount = true,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "LittleFS mount failed: %s",
            esp_err_to_name(ret)
        );

        logger_ready = false;
        return;
    }

    size_t total = 0;
    size_t used = 0;

    ret = esp_littlefs_info(
        LITTLEFS_PARTITION_LABEL,
        &total,
        &used
    );

    if (ret == ESP_OK) {
        ESP_LOGI(
            TAG,
            "LittleFS: total=%u bytes, used=%u bytes",
            (unsigned)total,
            (unsigned)used
        );
    }

    create_csv_header();

    logger_ready = true;

    ESP_LOGI(TAG, "Data logger ready");
    ESP_LOGI(TAG, "Log file: %s", LOG_FILE_PATH);
}

void data_logger_log(
    const sensor_data_t *sensor_data,
    const controller_status_t *controller_status
)
{
    if (!logger_ready) {
        return;
    }

    if (sensor_data == NULL || controller_status == NULL) {
        return;
    }

    FILE *file = fopen(LOG_FILE_PATH, "a");

    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open log file");
        return;
    }

    int64_t timestamp_us = esp_timer_get_time();

    int irrigation_event =
        controller_status->pump_state == PUMP_ON ? 1 : 0;

    /*
     * plant_stage and plant_stress_label are intentionally
     * placeholders for now.
     *
     * We will populate these once we start collecting
     * real mungbean observations.
     */
    const char *plant_stage = "unknown";
    const char *plant_stress_label = "unknown";

    fprintf(
        file,
        "%lld,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%d,"
        "%d,"
        "%d,"
        "%s,"
        "%s\n",

        (long long)timestamp_us,

        sensor_data->soil_moisture,
        sensor_data->temperature,
        sensor_data->humidity,
        sensor_data->flow_rate,

        sensor_data->is_day ? 1 : 0,

        controller_status->pump_state == PUMP_ON ? 1 : 0,

        irrigation_event,

        plant_stage,
        plant_stress_label
    );

    fclose(file);
}

bool data_logger_is_ready(void)
{
    return logger_ready;
}