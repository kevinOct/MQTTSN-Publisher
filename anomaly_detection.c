#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "sensordata_buffer.h"
#include "anomaly_detection.h"


//static CircularBuffer* mem_buffer;

float STANDARD_DEVIATION = 22.75; //60.0;
float MEAN = 150.0; //100.0;
float Z_THRESHOLD = 2.0;
/*
static double _calculate_mean(void) {
    double sum = 0.0;

    for (int i = 0; i < BUFFER_SIZE; i++) {
        int index = (mem_buffer->tail + i) % BUFFER_SIZE;
        //Measurement measurement = buffer->data[index];

        sum += (mem_buffer->data[index]).rn_measurement;
    }
    return (sum / BUFFER_SIZE);
}

static double _calculate_sd(double mean) {
    double sum = 0.0;

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        int index = (mem_buffer->tail + i) % BUFFER_SIZE;
        sum += pow((mem_buffer->data[index]).rn_measurement - mean, 2);
    }

    return sqrt(sum / BUFFER_SIZE);
    
} */

static float _calculate_z_score(float rn_value){
    return ((rn_value - MEAN) / STANDARD_DEVIATION);
}

int modify_params(float mean, float sd, float z_threshold) {
    STANDARD_DEVIATION = sd;
    MEAN = mean;
    Z_THRESHOLD = z_threshold;
    return 1;
}

float get_sd(void) {
    return STANDARD_DEVIATION;
}

float get_mean(void) {
    return MEAN;
}

float get_z(void) {
    return Z_THRESHOLD;
}

void print_params(void) {
    printf("Anomaly detection: standard deviation=%d, mean=%d, z_threshold=%d\n", (int)STANDARD_DEVIATION, (int)MEAN, (int)Z_THRESHOLD);
}

bool detect_anomaly(float rn_value) {

    //double mean = _calculate_mean();
    //double sd = _calculate_sd(mean);
    double z_score = _calculate_z_score(rn_value);

    if (((float) abs(z_score)) >= Z_THRESHOLD) return true;
    else return false;
}