#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <stdbool.h>
#include "sensor_manager.h"
#include "irrigation_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

void data_logger_init(void);

void data_logger_log(
    const sensor_data_t *sensor_data,
    const controller_status_t *controller_status
);

bool data_logger_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif