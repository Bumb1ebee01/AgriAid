#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "sensor_manager.h"
#include "irrigation_controller.h"
#include "ml_inference.h"

#ifdef __cplusplus
extern "C" {
#endif

void web_server_start(void);

void web_server_update_state(
    const sensor_data_t *sensor,
    const controller_status_t *controller,
    const ml_result_t *ml
);

#ifdef __cplusplus
}
#endif

#endif