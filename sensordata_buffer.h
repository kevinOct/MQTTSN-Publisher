/**
 * @file sensordata_buffer.h
 * @brief Header file for managing and processing sensor data buffers.
 *
 * This header file provides declarations and documentation for functions and
 * data structures related to managing and processing sensor data buffers. 
 *
 * @author Kevin Tony, Anuja Dadhich
 * @date 08 September 2023
 */


#include "thread.h"
#include "mutex.h"

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#define BUFFER_SIZE 144 // Store data for atleast one day (144) max 400?

// Define the Measurement struct
typedef struct
{
    float rn_measurement;                       // Rn measurement (Bq/l)
    float rn_measurement_interval;              // sec
    float rn_detection_limit;                   // (Bq/l) becquerel per liter
    float temp_measurement_interval;            // sec
    float temp_measurement_accuracy;            // degrees Celsius
    float water_pressure_measurement_interval;  // sec        
    float water_pressure_measurement_accuracy;  // (kPa) kiloPascal
    float water_ph_measurement_interval;        // sec
    float conductivity;                         // (µS/cm) microseimens per centimeter
    bool anomaly;                               // anomaly check
} Measurement;

// Define the CircularBuffer struct
typedef struct
{
    Measurement* data; // Array to hold the data
    int size;          // Maximum size of the buffer
    int head;          // Index to insert new data
    int tail;          // Index to remove data
    int count;         // Current number of elements in the buffer
    mutex_t mutex; // mutex for synchronisation
} CircularBuffer;

// Function prototypes
int initialize_circular_buffer(void);
void free_circular_buffer(void);
bool is_circular_buffer_empty(void);
bool is_circular_buffer_full(void);
int insert_measurement(Measurement new_measurement);
Measurement fetch_oldest_measurement(void);
void remove_oldest_measurement(void);
void print_measurement_at_index(int index);
void print_buffer_contents(void);
Measurement* retrieve_all_measurements(void);
int get_buffer_count(void);
void reset_buffer(void);
int get_buffer_tail(void);
void print_anomaly_at_index(int index);
bool is_anomaly(int index);

#endif /* CIRCULAR_BUFFER_H */
