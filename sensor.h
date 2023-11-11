/**
 * @file sensor.h
 * @brief Header file for managing and processing sensor data and report generation.
 *
 * This header file provides declarations and documentation for functions 
 * related to generating random measurements, simulating data
 * generation from sensors, and publishing sensor reports. 
 *
 * @author Kevin Tony, Anuja Dadhich
 * @date 08 September 2023
 */


#include "sensordata_buffer.h"

#ifndef SENSOR_REPORT_H
#define SENSOR_REPORT_H


typedef enum {
    s_sensor, s_rn, s_temperature, s_water, s_conductivity} sensor_report_state_t;


int generate_random_measurement(Measurement *measurement);

int _start_data_simulation(void);

int sensor_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);
int sensor_spec_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);
int controller_spec_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);

#endif // SENSOR_REPORT_H
