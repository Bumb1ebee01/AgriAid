#ifndef IRRIGATION_CONTROLLER_H
#define IRRIGATION_CONTROLLER_H

#include <stdbool.h>

#include "sensor_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PUMP_OFF = 0,
    PUMP_ON
} pump_state_t;

typedef struct {
    pump_state_t pump_state;

    bool irrigation_allowed;

    bool no_flow_fault;

    bool max_runtime_fault;

    float irrigation_score;
} controller_status_t;

void controller_init(void);

void controller_update(
    const sensor_data_t *sensor_data,
    controller_status_t *status
);

bool controller_pump_is_on(void);

#ifdef __cplusplus
}
#endif

#endif