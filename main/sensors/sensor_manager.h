#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float soil_moisture;
    float temperature;
    float humidity;
    float flow_rate;
    bool is_day;
} sensor_data_t;

void sensors_init(void);

void sensors_update(sensor_data_t *data);

#ifdef __cplusplus
}
#endif

#endif