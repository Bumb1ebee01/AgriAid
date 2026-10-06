#include "ml_inference.h"

#include <cstddef>
#include <cstdint>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/c/common.h"

static const char *TAG = "AGRIAID_ML";

/*
 * Initial tensor arena.
 *
 * This is only the runtime memory reserved for TinyML.
 * The actual trained model will be added later.
 */
static constexpr size_t TENSOR_ARENA_SIZE = 64 * 1024;

static uint8_t *tensor_arena = nullptr;

static tflite::MicroInterpreter *interpreter = nullptr;

static TfLiteTensor *input_tensor = nullptr;
static TfLiteTensor *output_tensor = nullptr;


bool ml_init(void)
{
    ESP_LOGI(TAG, "Initializing AgriAid TinyML engine");

    if (tensor_arena != nullptr) {
        ESP_LOGI(TAG, "TinyML engine already initialized");
        return true;
    }


    /*
     * -----------------------------------------
     * TRY PSRAM FIRST
     * -----------------------------------------
     */

    tensor_arena = static_cast<uint8_t *>(
        heap_caps_malloc(
            TENSOR_ARENA_SIZE,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
        )
    );

    if (tensor_arena != nullptr) {

        ESP_LOGI(
            TAG,
            "Tensor arena allocated in PSRAM: %u bytes",
            static_cast<unsigned>(TENSOR_ARENA_SIZE)
        );

    } else {

        /*
         * -----------------------------------------
         * FALLBACK TO INTERNAL RAM
         * -----------------------------------------
         *
         * This keeps the firmware running even if
         * PSRAM isn't currently enabled in the
         * project configuration.
         */

        ESP_LOGW(
            TAG,
            "PSRAM allocation unavailable"
        );

        ESP_LOGW(
            TAG,
            "Trying internal RAM instead..."
        );

        tensor_arena = static_cast<uint8_t *>(
            heap_caps_malloc(
                TENSOR_ARENA_SIZE,
                MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
            )
        );

        if (tensor_arena != nullptr) {

            ESP_LOGI(
                TAG,
                "Tensor arena allocated in internal RAM: %u bytes",
                static_cast<unsigned>(TENSOR_ARENA_SIZE)
            );

        } else {

            ESP_LOGE(
                TAG,
                "Failed to allocate TinyML tensor arena"
            );

            return false;
        }
    }


    /*
     * -----------------------------------------
     * MODEL STATUS
     * -----------------------------------------
     *
     * We intentionally do NOT load a fake model.
     *
     * The real mungbean model will be added after
     * we prepare the dataset and train it.
     */

    interpreter = nullptr;
    input_tensor = nullptr;
    output_tensor = nullptr;

    ESP_LOGI(TAG, "TinyML runtime initialized");
    ESP_LOGI(TAG, "Model status: NOT LOADED");

    return true;
}


ml_result_t ml_predict(
    float soil_moisture,
    float temperature,
    float humidity,
    float flow_rate,
    bool is_day
)
{
    /*
     * These will become model inputs later.
     */
    (void)soil_moisture;
    (void)temperature;
    (void)humidity;
    (void)flow_rate;
    (void)is_day;


    ml_result_t result = {
        .plant_health_score = 0.0f,
        .irrigation_score = 0.0f,
        .model_ready = false
    };


    /*
     * No trained model exists yet.
     *
     * Therefore we deliberately return:
     *
     * model_ready = false
     *
     * rather than inventing an ML prediction.
     */

    return result;
}


void ml_deinit(void)
{
    if (tensor_arena != nullptr) {

        heap_caps_free(tensor_arena);

        tensor_arena = nullptr;
    }

    interpreter = nullptr;
    input_tensor = nullptr;
    output_tensor = nullptr;

    ESP_LOGI(TAG, "TinyML engine deinitialized");
}