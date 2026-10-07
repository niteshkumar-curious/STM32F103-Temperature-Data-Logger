#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

void temp_sensor_init(void);
bool temp_sensor_get(volatile float *calculated_temp);

#endif 