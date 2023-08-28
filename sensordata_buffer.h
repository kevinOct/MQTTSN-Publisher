#include "thread.h"
#include "mutex.h"

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H


// Define the Measurement struct
typedef struct
{
    float temperature;
    float humidity;
    float air_pressure;
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

#endif /* CIRCULAR_BUFFER_H */
