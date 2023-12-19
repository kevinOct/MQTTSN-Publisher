

//#include "sensordata_buffer.h"
///#include "mqttsn_publisher.h"


// Various Test Functions

/**
 * Test 1
 * Test whether the gateway controller can utilise the MQTT-SN
    protocol to connect and publish a data packet to a specified topic.
*//**
int test1(int argc, char **argv) {
  if (sim7020cmd_topic(argc, argv) != 0) {
    printf("Test 1: FAIL");
    return 1;
  }
  else {
    printf("Test 1: PASS");
    return 0;
  }
}*/


/**
 * Test 2
 * Test whether the data is stored successfully in the circular memory buffer
 * after the sensor data is created.
*//**
int test2(int argc, char **argv) {
    (void) argc; (void) argv;

    printf("Test 2: Add three data packets to the buffer\n");

    if (_start_data_simulation() != 0) {
        printf("Test 2: FAIL\n");
        return 1;
    }

    if (get_buffer_count() == 1) {
        printf("Buffer count: %d\n", get_buffer_count());
    }
    else {
        printf("Test 2: FAIL. One data unit added. Buffer count does equal 1.\n");
        return 1;
    }


    if (_start_data_simulation() != 0) {
        printf("Test 2: FAIL\n");
        return 1;
    }

    if (get_buffer_count() == 2) {
        printf("Buffer count: %d\n", get_buffer_count());
    }
    else {
        printf("Test 2: FAIL. Two data units added. Buffer count does equal 2.\n");
        return 1;
    }


    if (_start_data_simulation() != 0) {
        printf("Test 2: FAIL\n");
        return 1;
    }

    if (get_buffer_count() == 3) {
        printf("Buffer count: %d\n\n", get_buffer_count());
    }
    else {
        printf("Test 2: FAIL. Three data units added. Buffer count does equal 3.\n");
        return 1;
    }

    print_buffer_contents();

    printf("Test 2: PASS\n");
    return 0;

}*/

/**
 * Test 3
 * Test whether the oldest data in the circular memory buffer is
    successfully fetched for report generation.

 * NOTE: This test can be improved significantly. Possible automate the index checking. Maybe store
    buffer index in Measurement struct to cross-check. 

    Currently relies on the user to validate test results.
*//**
int test3(int argc, char **argv) {
    (void) argc; (void) argv;

    printf("Test 3: Fetch oldest(FIFO Policy) data packet. Three data packets are added to an empty buffer.\n");

    _start_data_simulation();
    _start_data_simulation();
    _start_data_simulation();

     if (get_buffer_tail() == 0) {
        printf("Current tail index: %d\n", get_buffer_tail());
    }
    else {
        printf("Test 3: FAIL. Tail index does not equal 0. Tail index 0 corresponds to oldest measurement in empty buffer.\n");
        return 1;
    }

    printf("\n");

    printf("Printing oldest data in buffer...\n");
    print_measurement_at_index(0);
    printf("Oldest data in buffer printed...\n");

    printf("\n");
    printf("Fetching oldest data...\n");
    Measurement measurement = fetch_oldest_measurement();
    printf("Oldest data fetched...\n\n");

    printf("Printing fetched data...\n");

    printf(
       "Rn measurement=%d Bq/l, "
       "Radiation Measurement Interval=%d sec, "
       "Detection Limit=%d Bq/l, "
       "Temperature Measurement Interval=%d sec, "
       "Temperature Measurement Accuracy=%d°C, "
       "Water Pressure Measurement Interval=%d sec, "
       "Water Pressure Measurement Accuracy=%d kPa, "
       "Water pH Measurement Interval=%d sec, "
       "Conductivity=%d µS/cm",
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

    printf("Test 3: PASS\n");
    return 0;

}*/

/**
 * Test 4
 * 
 * Test whether all the data at a given instance in the circular
     memory buffer is successfully fetched for report generation
*/

/**
int test4(int argc, char **argv) {
    (void) argc; (void) argv;

    printf("Test 4: Test whether the entire buffer contents are fetched. Three data packets are added to an empty buffer.\n");
    _start_data_simulation();
    _start_data_simulation();
    _start_data_simulation();


    printf(("Size of one data entry: 904 bytes. Size of 3 data entries: 2712 bytes\n"));

    if (get_buffer_count() == 3) 
    {
        printf("Current buffer count = %d\n", get_buffer_count());
    }
    else {
        printf("Test 4: FAIL. Buffer count does not equal 3. Error inserting 3 data packets.\n");
        return 1;
    }

    
    printf("\n");
    printf("Creating sensor report...\n");

    uint8_t _buffer[MQTTSN_BUFFER_SIZE];
    static uint8_t pub_finished;
    int byte_count = 0;
    do
    {
        char *topicstr = default_topicstr;
        char *basename = default_basename;
        int publen;
        publen = sensor_report(_buffer, sizeof(_buffer), &pub_finished, &topicstr, &basename);
        byte_count += publen;
    } while (!pub_finished);
    
    printf("Sensor report created...\n");

    printf("\n");

    if (byte_count == 3 * 904) {
        printf("Number of bytes read for report generation: 2712 \n");
        printf("All buffer contents fetched for report generation...\n");
        printf("Test 4: PASS\n");    
        return 0;
    }
    else {
        printf("Error: Byte count does not equal 2712 for 3 data entries. All data not fetched for report generation.\n");
        printf("Test 4: FAIL\n");
        return 1;
    }
}*/


/**
 * Test 5
 * 
 * Test whether all the data in the memory buffer is removed
 * once it has been successfully fetched for report generation
*/
/*
int test5(int argc, char **argv) {
    (void) argc; (void) argv;

    printf("Test 5: Check buffer count after report generation. Three data packets are added to an empty buffer.\n");

    _start_data_simulation();
    _start_data_simulation();
    _start_data_simulation();

    if (get_buffer_count() == 3) 
    {
        printf("Current buffer count = %d\n", get_buffer_count());
    }
    else {
        printf("Test 5: FAIL. Buffer count does not equal 3. Error inserting 3 data packets.\n");
        return 1;
    }
    
    printf("\n");
    printf("Creating sensor report...\n");

    uint8_t _buffer[MQTTSN_BUFFER_SIZE];
    static uint8_t pub_finished;
    int byte_count;
    do
    {
        char *topicstr = default_topicstr;
        char *basename = default_basename;
        int publen;
        publen = sensor_report(_buffer, sizeof(_buffer), &pub_finished, &topicstr, &basename);
        byte_count += publen;
    } while (!pub_finished);
    
    printf("Sensor report created...\n");


    printf("\n");
    if (get_buffer_count() == 0) {
        printf("Current buffer count = %d\n", get_buffer_count());
        printf("Buffer empty. All data fetched for report generation.\n");
        printf("Test 5: PASS \n");
        return 0;
    }
    else {
        printf("Error: Buffer count does not equal 0. All data not fetched for report generation. %d\n", get_buffer_count());
        printf("Test 5: FAIL\n");
        return 1;
    }
}

int test6(int argc, char **argv) {
    (void) argc, (void) **argv;

    printf("Test 6: Test data generation during connection loss.\n");

    int count = get_buffer_count();

    printf("Buffer count before connection loss = %d\n", count);
    printf("Stopping SIM7020 connection...\n");

    if (sim7020_stop() == 0) {
        printf("Waiting 15 seconds...\n");
        xtimer_usleep(15 * US_PER_SEC);
        printf("DONE WAITING\n");
    }
    else {
        printf("Error: Not able to stop SIM7020 connection...\n");
        return 1;
    }

    int new_count = get_buffer_count();

    if (new_count > count) {
        printf("Buffer count 15 seconds after connection loss = %d\n", new_count);
        printf("TEST 6: PASS\n");
        return 0;
    }
    else {
        printf("No new data added to buffer during connection loss...\n");
        printf("TEST 6: FAIL\n");
        return 1;
    }


    return 0;
    
}*/

/**
 * Test 7
 * Pre-requisites: set buffer size to 3
*//***
int test7(int argc, char **argv) {
    (void) argc, (void) **argv;

    printf("Test 7: Test FIFO replacement policy.\n");

    printf("BUFFER CAPACITY = 3\n");
    printf("Adding 3 data entries to buffer...\n");

    _start_data_simulation();
    _start_data_simulation();
    _start_data_simulation();

    int count = get_buffer_count();

    if (count == 3) {
        printf("Buffer count = %d. Buffer FULL\n", count);
    }
    else {
        printf("Error: Buffer count does not equal 3.\n");
        printf("TEST 7: FAIL\n");
        return 1;
    }

    int tail = get_buffer_tail();
    printf("Buffer tail index = %d\n", tail);

    printf("\n");
    printf("Printing oldest measurement at index 0...\n");
    print_measurement_at_index(0);

    printf("\n");
    printf("Adding new data entry...\n");
    _start_data_simulation();
    
    int new_count = get_buffer_count();
    
    if (new_count == 3) {
        printf("Current buffer count = %d\n", new_count);
    }
    else {
        printf("Error adding new data entry. Buffer count does not equal 3\n");
        printf("TEST 7: FAIL\n");
        return 1;
    }


    tail = get_buffer_tail();

    if (tail == 1) {
        printf("New buffer tail index = %d\n", tail);
    }
    else {
        printf("Error: Buffer tail not incremented.\n");
        printf("TEST 7: FAIL\n");
        return 1;
    }

    printf("\n");
    printf("Printing current measurement at index 0...\n");
    print_measurement_at_index(0);
    printf("\n");

    printf("TEST 7: PASS\n");

    return 0;
} */

/**
 * Test 8
 * Set mean, standard deviation, and z-threshold to suit your test parameters.
*/
/**
int test8(int argc, char **argv) {
    (void) argc, (void) **argv;

    printf("Test 8: Test anomaly detection.\n");

    printf("BUFFER CAPACITY = 3\n");
    printf("Mean = 150.0, Standard deviation = 22.75, Z-threshold = 2.0\n");
    
    printf("Adding 3 data entries with no anomalies to buffer...\n");

    
    Measurement measurement1;
    measurement1.rn_measurement = 125;
    insert_measurement(measurement1);

    Measurement measurement2;
    measurement2.rn_measurement = 145;
    insert_measurement(measurement2);

    Measurement measurement3;
    measurement3.rn_measurement = 180;
    insert_measurement(measurement3);



    int count = get_buffer_count();

    if (count == 3) {
        printf("Buffer count = %d. Buffer FULL\n", count);
    }
    else {
        printf("Error: Buffer count does not equal 3.\n");
        printf("TEST 8: FAIL\n");
        return 1;
    }

    printf("\n");
    printf("Adding new data entry with anomaly...\n");
    Measurement measurement4;
    measurement4.rn_measurement = 200;
    insert_measurement(measurement4);

    bool check = is_anomaly(0);
    if (check) {
        printf("Anomaly detected!\n");
    }
    else {
        printf("Error: No anomaly detected in new data entry.\n");
        printf("TEST 8: FAIL\n");
        return 1;
    }

    printf("\n");
    printf("New measurement at index 0\n");
    print_anomaly_at_index(0);

    printf("\n");
    printf("TEST 8: PASS\n");

    return 0;

}


int test9(int argc, char **argv) {
    (void) argc, (void) **argv;

    printf("Test 9: Bi-directional communication\n\n");

    printf("Use parameter values different to the current values in order to ensure a successful test\n");
    printf("Use cmnd 'anmodify sd_placeholder mean_placeholder zthreshold_placeholder\n\n");

    printf("Current anomaly detection parameters: \n");

    print_params();

    float sd = get_sd();
    float mean = get_mean();
    float z_threshold = get_z();

    printf("Publishing message...\n");
    _publish_all(1);

    if (sd != get_sd())
    {
        printf("Parameters modified!\n");
        printf("Current anomaly detection paramters: \n");
        print_params();
        printf("TEST 9: PASS\n");
        return 0;
    }

    if (mean != get_mean())
    {
        printf("Parameters modified!\n");
        printf("Current anomaly detection paramters: \n");
        print_params();
        printf("TEST 9: PASS\n");
        return 0;
    }

    if (z_threshold != get_z())
    {
        printf("Parameters modified!\n");
        printf("Current anomaly detection paramters: \n");
        print_params();
        printf("TEST 9: PASS\n");
        return 0;
    }
    

    printf("Parameters not modified!\n");
    printf("TEST 9: FAIL\n");
    return 1;

} */