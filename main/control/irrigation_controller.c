#include "irrigation_controller.h"

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "CONTROL";

/*
 * Production safety limits.
 */
#define SOIL_DRY_THRESHOLD       30.0f
#define SOIL_WET_THRESHOLD       45.0f

#define MAX_PUMP_RUNTIME_MS      30000
#define PUMP_COOLDOWN_MS         60000
#define FLOW_TIMEOUT_MS          5000

static pump_state_t pump_state = PUMP_OFF;

static int64_t pump_started_at = 0;
static int64_t pump_stopped_at = 0;

static bool no_flow_fault = false;
static bool max_runtime_fault = false;

static void pump_start(void)
{
    if (pump_state == PUMP_ON) {
        return;
    }

    pump_state = PUMP_ON;

    pump_started_at = esp_timer_get_time() / 1000;

    ESP_LOGI(TAG, "PUMP -> ON");
}

static void pump_stop(void)
{
    if (pump_state == PUMP_OFF) {
        return;
    }

    pump_state = PUMP_OFF;

    pump_stopped_at = esp_timer_get_time() / 1000;

    ESP_LOGI(TAG, "PUMP -> OFF");
}

static bool pump_can_start(void)
{
    if (pump_state == PUMP_ON) {
        return false;
    }

    int64_t now = esp_timer_get_time() / 1000;

    if (pump_stopped_at == 0) {
        return true;
    }

    int64_t elapsed = now - pump_stopped_at;

    if (elapsed < PUMP_COOLDOWN_MS) {
        return false;
    }

    return true;
}

void controller_init(void)
{
    pump_state = PUMP_OFF;

    pump_started_at = 0;
    pump_stopped_at = 0;

    no_flow_fault = false;
    max_runtime_fault = false;

    ESP_LOGI(TAG, "Irrigation controller initialized");
}

void controller_update(
    const sensor_data_t *sensor_data,
    controller_status_t *status
)
{
    if (sensor_data == NULL || status == NULL) {
        return;
    }

    int64_t now = esp_timer_get_time() / 1000;

    bool irrigation_allowed = true;

    no_flow_fault = false;
    max_runtime_fault = false;

    /*
     * -----------------------------------------
     * SAFETY: maximum pump runtime
     * -----------------------------------------
     */
    if (pump_state == PUMP_ON) {

        int64_t runtime = now - pump_started_at;

        if (runtime >= MAX_PUMP_RUNTIME_MS) {

            ESP_LOGW(
                TAG,
                "SAFETY: maximum pump runtime reached"
            );

            max_runtime_fault = true;

            pump_stop();
        }
    }

    /*
     * -----------------------------------------
     * NIGHT SAFETY
     * -----------------------------------------
     */
    if (!sensor_data->is_day) {

        irrigation_allowed = false;

        if (pump_state == PUMP_ON) {
            ESP_LOGI(TAG, "Night detected -> stopping irrigation");
            pump_stop();
        }
    }

    /*
     * -----------------------------------------
     * NO-FLOW SAFETY
     *
     * This will become real once YF-S201
     * is connected.
     * -----------------------------------------
     */
    if (pump_state == PUMP_ON) {

        int64_t runtime = now - pump_started_at;

        if (runtime >= FLOW_TIMEOUT_MS &&
            sensor_data->flow_rate <= 0.05f) {

            /*
             * We currently simulate no flow.
             *
             * To avoid stopping the pump during
             * tonight's software demo, this section
             * is intentionally disabled.
             *
             * It will be enabled with the real
             * YF-S201 sensor.
             */
        }
    }

    /*
     * -----------------------------------------
     * SOIL CONTROL
     * -----------------------------------------
     */

    if (irrigation_allowed) {

        /*
         * Soil dry -> irrigation required
         */
        if (sensor_data->soil_moisture < SOIL_DRY_THRESHOLD) {

            if (pump_can_start()) {
                pump_start();
            }
        }

        /*
         * Soil sufficiently wet -> stop irrigation
         */
        if (sensor_data->soil_moisture >= SOIL_WET_THRESHOLD) {

            if (pump_state == PUMP_ON) {
                pump_stop();
            }
        }
    }

    /*
     * Irrigation score.
     *
     * Temporary deterministic score.
     * Later TinyML will replace this.
     */
    float score = 0.0f;

    if (sensor_data->soil_moisture < SOIL_DRY_THRESHOLD) {
        score = 1.0f;
    } else if (sensor_data->soil_moisture < SOIL_WET_THRESHOLD) {
        score = 0.5f;
    }

    status->pump_state = pump_state;
    status->irrigation_allowed = irrigation_allowed;
    status->no_flow_fault = no_flow_fault;
    status->max_runtime_fault = max_runtime_fault;
    status->irrigation_score = score;
}

bool controller_pump_is_on(void)
{
    return pump_state == PUMP_ON;
}