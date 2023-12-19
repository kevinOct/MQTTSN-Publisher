/**
 * @file sensordata_buffer.c
 * @briefFor managing and processing sensor data buffers.
 *
 * Provides functions related to managing and processing sensor data buffers. 
 *
 * @author Kevin Tony, Anuja Dadhich
 * @date 08 September 2023
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "sensordata_buffer.h"
#include "anomaly_detection.h"


CircularBuffer* buffer; 

// Function to initialize the circular buffer
int initialize_circular_buffer(void)
{
    //CircularBuffer* buffer = (CircularBuffer*)malloc(sizeof(CircularBuffer));
    buffer = (CircularBuffer*)malloc(sizeof(CircularBuffer));

    if (buffer == NULL)
    {
        printf("Buffer allocation failed");
        return 0; // Return NULL to indicate failure
    }

    buffer->data = (Measurement*)malloc(sizeof(Measurement) * BUFFER_SIZE);

    if (buffer->data == NULL)
    {
        printf("Data allocation failed");
        free(buffer); // Free the buffer memory
        return 0;  
    }
    buffer->size = BUFFER_SIZE;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;

    mutex_init(&buffer->mutex);

    return 1;
}

// Function to check if the circular buffer is empty
bool is_circular_buffer_empty(void)
{
    return (buffer->count == 0);
}

// Function to check if the circular buffer is full
bool is_circular_buffer_full(void)
{
    return (buffer->count == buffer->size);
}

// Function to insert a new Measurement into the circular buffer
int insert_measurement(Measurement new_measurement)
{
    mutex_lock(&buffer->mutex);

    if (detect_anomaly(new_measurement.rn_measurement)) {
        new_measurement.anomaly = true;
    }

    if (is_circular_buffer_full())
    {
        // If buffer is full, re-initialize from the beginning
        buffer->tail = (buffer->tail + 1) % buffer->size;
        buffer->count--;
    }

    // Insert the new_measurement at the head of the buffer
    buffer->data[buffer->head] = new_measurement;
    buffer->head = (buffer->head + 1) % buffer->size;
    buffer->count++;

    mutex_unlock(&buffer->mutex);

    return 0;
}


int get_buffer_count(void) {
    mutex_lock(&buffer->mutex);

    int count = buffer->count;
    mutex_unlock(&buffer->mutex);

    return count;
}


int get_buffer_tail(void) {
    mutex_lock(&buffer->mutex);

    int tail = buffer->tail;
    mutex_unlock(&buffer->mutex);

    return tail;
}

bool is_anomaly(int index) {
    mutex_lock(&buffer->mutex);
    if (index >= 0 && index < buffer->count)
    {
        //int real_index = (buffer->tail + index) % buffer->size;
        Measurement measurement = buffer->data[index];

        if (measurement.anomaly) {
            mutex_unlock(&buffer->mutex);
            return true;
        }
        else {
            mutex_unlock(&buffer->mutex);
            return false;
        }
    }
    else
    {
        printf("Invalid index\n");
        mutex_unlock(&buffer->mutex);
        return false;
    }
}

Measurement fetch_oldest_measurement(void) {
    mutex_lock(&buffer->mutex);

    if (is_circular_buffer_empty())
    {
        // Handle the case when the buffer is empty (returning a dummy Measurement)
        Measurement dummy_measurement = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, false};
        mutex_unlock(&buffer->mutex);
        return dummy_measurement;
    }

    Measurement oldest_measurement = buffer->data[buffer->tail];

    mutex_unlock(&buffer->mutex);

    return oldest_measurement;
}


// Function to remove and return the oldest Measurement from the circular buffer
void remove_oldest_measurement(void)
{
    mutex_lock(&buffer->mutex);    

    // Remove and return the Measurement at the tail of the buffer
    //Measurement oldest_measurement = buffer->data[buffer->tail];
    buffer->tail = (buffer->tail + 1) % buffer->size;
    buffer->count--;

    mutex_unlock(&buffer->mutex);
}

// Function to free the memory allocated for the circular buffer
void free_circular_buffer(void)
{
    free(buffer->data);
    free(buffer);
}


// Print Rn measurement and anomaly reading of a Measurement at a given index.
void print_anomaly_at_index(int index) {

    mutex_lock(&buffer->mutex);
    if (index >= 0 && index < buffer->count)
    {
        //int real_index = (buffer->tail + index) % buffer->size;
        Measurement measurement = buffer->data[index];

        printf("Measurement at index: %d, "
        "Rn measurement=%d Bq/l, ",
        index, 
        (int)measurement.rn_measurement);

        if (measurement.anomaly) {
            printf("Anomaly: True\n");
        }
        else { printf("Anomaly: False\n"); }

        mutex_unlock(&buffer->mutex);
        return;
    }
    else
    {
        printf("Invalid index\n");
        mutex_unlock(&buffer->mutex);
        return;
    }

}


// Function to print the Measurement at a given index
void print_measurement_at_index(int index)
{
    mutex_lock(&buffer->mutex);
    if (index >= 0 && index < buffer->count)
    {
        int real_index = (buffer->tail + index) % buffer->size;
        Measurement measurement = buffer->data[real_index];

       printf("Measurement at index, real_index: %d, %d, "
       "Rn measurement=%d Bq/l, "
       "Radiation Measurement Interval=%d sec, "
       "Detection Limit=%d Bq/l, "
       "Temperature Measurement Interval=%d sec, "
       "Temperature Measurement Accuracy=%d°C, "
       "Water Pressure Measurement Interval=%d sec, "
       "Water Pressure Measurement Accuracy=%d kPa, "
       "Water pH Measurement Interval=%d sec, "
       "Conductivity=%d µS/cm\n",
       index, real_index, 
       (int)measurement.rn_measurement,
       (int)measurement.rn_measurement_interval,
       (int)measurement.rn_detection_limit,
       (int)measurement.temp_measurement_interval,
       (int)measurement.temp_measurement_accuracy,
       (int)measurement.water_pressure_measurement_interval,
       (int)measurement.water_pressure_measurement_accuracy,
       (int)measurement.water_ph_measurement_interval,
       (int)measurement.conductivity);


        mutex_unlock(&buffer->mutex);
        return;
    }
    else
    {
        printf("Invalid index\n");
        mutex_unlock(&buffer->mutex);
        return;
    }
    
}


void print_buffer_contents(void)
{
    mutex_lock(&buffer->mutex);

    printf("Printing Buffer\n\n");
    printf("Buffer Stats...\n");

    printf("count: %d\n", buffer->count);
    printf("head: %d\n", buffer->head);
    printf("tail: %d\n", buffer->tail);

    printf("Printing Buffer Contents...\n");
    for (int i = 0; i < buffer->count; i++)
    {
        int index = (buffer->tail + i) % BUFFER_SIZE;
        Measurement measurement = buffer->data[index];

        printf("At index: %d, "
       "Rn measurement=%d Bq/l, "
       "Radiation Measurement Interval=%d sec, "
       "Detection Limit=%d Bq/l, "
       "Temperature Measurement Interval=%d sec, "
       "Temperature Measurement Accuracy=%d°C, "
       "Water Pressure Measurement Interval=%d sec, "
       "Water Pressure Measurement Accuracy=%d kPa, "
       "Water pH Measurement Interval=%d sec, "
       "Conductivity=%d µS/cm",
       index, 
       (int)measurement.rn_measurement,
       (int)measurement.rn_measurement_interval,
       (int)measurement.rn_detection_limit,
       (int)measurement.temp_measurement_interval,
       (int)measurement.temp_measurement_accuracy,
       (int)measurement.water_pressure_measurement_interval,
       (int)measurement.water_pressure_measurement_accuracy,
       (int)measurement.water_ph_measurement_interval,
       (int)measurement.conductivity);

       if (measurement.anomaly)
       {
        printf("Anomaly: True\n");
       }
       else { printf("Anomaly: False\n"); }
       

    }
    mutex_unlock(&buffer->mutex);
}


void reset_buffer(void) {
    mutex_lock(&buffer->mutex);

    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;

    mutex_unlock(&buffer->mutex);
}


Measurement* retrieve_all_measurements(void) {
    mutex_lock(&buffer->mutex);

    if (is_circular_buffer_empty())
    {
        printf("Buffer empty. Cannot retrieve elements");
        mutex_unlock(&buffer->mutex);
        return NULL;
    }

    Measurement* measurements = (Measurement*)malloc(sizeof(Measurement) * buffer->count);

    if (measurements == NULL)
    {
        printf("Measurement array allocation failed");
        mutex_unlock(&buffer->mutex);
        return NULL;
    }

    for (int i = 0; i < buffer->count; i++)
    {
        int real_index = (buffer->tail + i) % buffer->size;
        measurements[i] = buffer->data[real_index];
    }
    
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;

    mutex_unlock(&buffer->mutex);

    return measurements;
}


// Buffer initialisatio test
/*
int main()
{
    int bufferSize = 5; // Adjust the buffer size as needed
    CircularBuffer* buffer = initializeCircularBuffer();

    // Insert some measurements into the circular buffer
    Measurement m1 = {25.5, 50.0, 1013.2};
    Measurement m2 = {26.0, 49.5, 1012.8};
    Measurement m3 = {24.8, 51.2, 1014.0};

    insertMeasurement(buffer, m1);
    insertMeasurement(buffer, m2);
    insertMeasurement(buffer, m3);

    // Remove and print the oldest measurement
    Measurement oldest = removeOldestMeasurement(buffer);
    printf("Oldest Measurement: Temperature=%.2f, Humidity=%.2f, Air Pressure=%.2f\n", oldest.temperature, oldest.humidity, oldest.air_pressure);

    // Free the circular buffer when done
    freeCircularBuffer(buffer);

    return 0;
}*/

/*
int insert_measurement_new(Measurement new_measurement)
{
    mutex_lock(&buffer->mutex);

    if (is_circular_buffer_full())
    {
        int next_replaceable_index = buffer->tail;
        bool found_replaceable = false;

        // Find the next replaceable index with anomaly set to false
        for (int i = 0; i < buffer->size; i++)
        {
            if (!buffer->data[next_replaceable_index].anomaly)
            {
                found_replaceable = true;
                break;
            }
            next_replaceable_index = (next_replaceable_index + 1) % buffer->size;
        }

        // If a replaceable index with anomaly false is found, replace the measurement
        if (found_replaceable)
        {
            buffer->data[next_replaceable_index] = new_measurement;
        }
        // If all measurements have anomaly true, do not insert the new data
        else
        {
            mutex_unlock(&buffer->mutex);
            return -1; // Indicate that insertion failed
        }
    }
    else
    {
        // Insert the new_measurement at the head of the buffer
        buffer->data[buffer->head] = new_measurement;
        buffer->head = (buffer->head + 1) % buffer->size;
        buffer->count++;
    }

    mutex_unlock(&buffer->mutex);

    return 0; // Indicate successful insertion
}*/

/**
 * Disable automatic buffer allocation during this test
*//*
int test_buffer(int argc, char **argv) {
    (void) argc, (void) **argv;

    buffer = (CircularBuffer*)malloc(sizeof(CircularBuffer));

    if (buffer == NULL)
    {
        printf("Buffer allocation failed 1");
        return 0; // Return NULL to indicate failure
    }
    printf("Buffer allocated 1\n");

    buffer->data = (Measurement*)malloc(sizeof(Measurement) * BUFFER_SIZE);

    if (buffer->data == NULL)
    {
        printf("Buffer data allocation failed 1");
        return 0; // Return NULL to indicate failure
    }

    printf("Buffer data allocated 1\n");

    size_t total_memory = sizeof(CircularBuffer);
    printf("size of circular buffer: %u\n", total_memory);


    total_memory = sizeof(Measurement);
    printf("size of Measurement: %u\n", total_memory);

    total_memory = sizeof(Measurement) * BUFFER_SIZE;
    printf("size of Measurement * buffersize: %u\n", total_memory);
    
 
    free(buffer->data);
    free(buffer);


    buffer = (CircularBuffer*)malloc(sizeof(CircularBuffer));

    if (buffer == NULL)
    {
        printf("Buffer allocation failed 2");
        return 0; // Return NULL to indicate failure
    }

    printf("Buffer allocated 2\n");
    buffer->data = (Measurement*)malloc(sizeof(Measurement) * BUFFER_SIZE);

    if (buffer->data == NULL)
    {
        printf("Buffer data allocation failed 2");
        return 0; // Return NULL to indicate failure
    }

    printf("Buffer data allocated 2\n");

    free(buffer->data);
    free(buffer);

    return 1;
}*/

