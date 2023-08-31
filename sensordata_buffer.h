#include "thread.h"
#include "mutex.h"

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H


// Define the Measurement struct
typedef struct
{
    float rn_measurement_interval;              // sec
    float rn_detection_limit;                   // (Bq/l) becquerel per liter
    float temp_measurement_interval;            // sec
    float temp_measurement_accuracy;            // degrees Celsius
    float water_pressure_measurement_interval;  // sec        
    float water_pressure_measurement_accuracy;  // (kPa) kiloPascal
    float water_ph_measurement_interval;        // sec
    float conductivity;                         // (µS/cm) microseimens per centimeter
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
void insert_measurement(Measurement new_measurement);
Measurement fetch_oldest_measurement(void);
void remove_oldest_measurement(void);
void print_measurement_at_index(int index);
void print_buffer_contents(void);
Measurement* retrieve_all_measurements(void);
int get_buffer_count(void);
void reset_buffer(void);

#endif /* CIRCULAR_BUFFER_H */
