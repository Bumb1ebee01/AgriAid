#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float plant_health_score;
    float irrigation_score;
    bool model_ready;
} ml_result_t;

bool ml_init(void);

ml_result_t ml_predict(
    float soil_moisture,
    float temperature,
    float humidity,
    float flow_rate,
    bool is_day
);

void ml_deinit(void);

#ifdef __cplusplus
}
#endif

#endif