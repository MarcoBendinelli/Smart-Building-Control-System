#include "sensor.h"
#include <string.h>
#include <stdlib.h>

int generate_reading(int lower, int upper) {
    return (rand() %
            (upper - lower + 1)) + lower;
}

struct Sensor read_noise_value() {
    struct Sensor sensor;

    strncpy(sensor.id, "noise", 5);
    sensor.value = generate_reading(NOISE_LOWER_BOUND, NOISE_UPPER_BOUND);

    return sensor;
}

struct Sensor read_temperature() {
    struct Sensor sensor;

    strncpy(sensor.id, "temperature", 11);
    sensor.value = generate_reading(TEMP_LOWER_BOUND, TEMP_UPPER_BOUND);

    return sensor;
}

struct Sensor read_humidity() {
    struct Sensor sensor;

    strncpy(sensor.id, "humidity", 8);
    sensor.value = generate_reading(HUM_LOWER_BOUND, HUM_UPPER_BOUND);

    return sensor;
}
