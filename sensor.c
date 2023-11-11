/**
 * @file sensor.c
 * @brief For managing and processing sensor data and report generation.
 *
 * Provides functions related to generating random measurements, simulating data
 * generation from sensors, and publishing sensor reports. 
 *
 * @author Kevin Tony, Anuja Dadhich
 * @date 08 September 2023
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mbox.h"
#include "msg.h"
#include "sensor.h"
#include "report.h"

// Generate random measurements
int generate_random_measurement(Measurement *measurement)
{
    measurement->rn_measurement = (float)(rand() % 1000) + 100.0;   
    measurement->rn_measurement_interval = 360;  
    measurement->rn_detection_limit = 1.0;         
    measurement->temp_measurement_interval = (float)(rand() % 1000) + 100.0;      
    measurement->temp_measurement_accuracy = (float)(rand() % 110) + 1.0;
    measurement->water_pressure_measurement_interval = (float)(rand() % 1000) + 100.0;
    measurement->water_pressure_measurement_accuracy = (float)(rand() % 10) + 1.0;
    measurement->water_ph_measurement_interval = (float)(rand() % 30) + 11.0;
    measurement->conductivity = (float)(rand() % 100) + 1.0;
    measurement->anomaly = false;

    return 0;
}


/**
 * Simulates data generation from the sensor by producing reports and storing it in the memory buffer.
 */
int _start_data_simulation(void) {

    Measurement new_measurement;

    if (generate_random_measurement(&new_measurement) != 0) {
        printf("Error: New easurement not generated.\n");
        return 1;
    }

    if (insert_measurement(new_measurement) != 0) {
        printf("Error: New measurement not inserted.\n");
        return 1;
    }

    //printMeasurementAtIndex(circular_buffer, index);
    //print_buffer_contents(); 

    return 0;
}




int sensor_spec_report(uint8_t *buf, size_t len, uint8_t *finished,
                  __attribute__((unused)) char **topicp, __attribute__((unused)) char **basenamep) 
{
    char *s = (char *)buf;
    size_t l = len;
    int nread = 0;

    *finished = 0;

    //puts("Publishing sensor_boot_report");

    RECORD_START(s + nread, l - nread);
    PUTFMT(",{\"n\": \"sensor_boot_report\",\"vs\":\"_data_placeholder\"}");
    RECORD_END(nread);


    *finished = 1;
    return nread;
}

int controller_spec_report(uint8_t *buf, size_t len, uint8_t *finished,
                  __attribute__((unused)) char **topicp, __attribute__((unused)) char **basenamep) 
{
    char *s = (char *)buf;
    size_t l = len;
    int nread = 0;

    *finished = 0;

    //puts("Publishing controller_boot_report");

    RECORD_START(s + nread, l - nread);
    PUTFMT(",{\"n\": \"controller_boot_report\",\"vs\":\"_data_placeholder\"}");
    RECORD_END(nread);


    *finished = 1;
    return nread;
}

int sensor_report(uint8_t *buf, size_t len, uint8_t *finished,
                  __attribute__((unused)) char **topicp, __attribute__((unused)) char **basenamep)
{
    char *s = (char *)buf;
    size_t l = len;
    static sensor_report_state_t state = s_sensor;
    int nread = 0;

    *finished = 0;

    
    if (buf != NULL) {

        //print_buffer_contents();

        Measurement measurement;
        while (get_buffer_count() != 0)
        {
            measurement = fetch_oldest_measurement();

            switch (state)
            {
            case s_sensor:
                RECORD_START(s + nread, l - nread);
                PUTFMT(",{\"n\":\"sensor_data\",\"vj\":[");
                PUTFMT(",{\"n\":\"sensor_name:\",\"v\":\"sensor_name_placeholder\"}");
                PUTFMT(",{\"n\":\"sensor_id:\",\"v\":\"sensor_id_placeholder\"}");
                PUTFMT(",{\"n\":\"sensor_time:\",\"t\":\"sensor_time_placeholder\"}");
                PUTFMT("{\"n\":\"Anomaly\",\"u\":\"Boolean\",\"v\":%d},", measurement.anomaly);
                PUTFMT("]}");
                RECORD_END(nread);
                state = s_rn;

            case s_rn:
                RECORD_START(s + nread, l - nread);
                PUTFMT(",{\"n\":\"Rn parameters\",\"vj\":[");
                PUTFMT("{\"n\":\"Rn measurement\",\"u\":\"(Bq/l)/becquerel per liter\",\"v\":%d},", (int)measurement.rn_measurement);
                PUTFMT("{\"n\":\"Rn measurement interval\",\"u\":\"seconds\",\"v\":%d},", (int)measurement.rn_measurement_interval);
                PUTFMT("{\"n\":\"Rn detection limit\",\"u\":\"(Bq/l)/becquerel per liter\",\"v\":%d},", (int)measurement.rn_detection_limit);
                PUTFMT("]}");
                RECORD_END(nread);
                state = s_temperature;

            case s_temperature:
                RECORD_START(s + nread, l - nread);
                PUTFMT(",{\"n\":\"Temperature parameters\",\"vj\":[");
                PUTFMT("{\"n\":\"Temperature measurement interval\",\"u\":\"seconds\",\"v\":%d}", (int)measurement.temp_measurement_interval);
                PUTFMT("{\"n\":\"Temperature measurement accuracy\",\"u\":\"degrees celsius\",\"v\":%d}", (int)measurement.temp_measurement_accuracy); 
                PUTFMT("]}");
                RECORD_END(nread);
                state = s_water;

            case s_water:
                RECORD_START(s + nread, l - nread);
                PUTFMT(",{\"n\":\"Water parameters\",\"vj\":[");
                PUTFMT("{\"n\":\"Water pressure measurement interval\",\"u\":\"seconds\",\"v\":%d}", (int)measurement.water_pressure_measurement_interval);    
                PUTFMT("{\"n\":\"Water pressure measurement accuracy\",\"u\":\"(kPa)/kiloPascal\",\"v\":%d}", (int)measurement.water_pressure_measurement_accuracy);    
                PUTFMT("{\"n\":\"Water acidity(pH) measurement interval\",\"u\":\"seconds\",\"v\":%d}", (int)measurement.water_ph_measurement_interval);    
                PUTFMT("]}");
                RECORD_END(nread);
                state = s_conductivity;

            case s_conductivity:
                RECORD_START(s + nread, l - nread);
                PUTFMT("{\"n\":\"Conductivity\",\"u\":\"(µS/cm)/microseimens per centimeter\",\"v\":%d}", (int)measurement.conductivity);    
                RECORD_END(nread);
                state = s_sensor;

                remove_oldest_measurement();
            }
            
        }
        reset_buffer();
    }

    *finished = 1;
    return nread;
}