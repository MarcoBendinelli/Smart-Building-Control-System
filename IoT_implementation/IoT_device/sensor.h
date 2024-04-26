#ifndef SENSOR_H
#define SENSOR_H

#define NOISE_LOWER_BOUND -10        // Lower bound of the noise value
#define NOISE_UPPER_BOUND 60         // Upper bound of the noise value

#define TEMP_LOWER_BOUND 19          // Lower bound of the temperature value
#define TEMP_UPPER_BOUND 24          // Upper bound of the temperature value

#define HUM_LOWER_BOUND 50           // Lower bound of the humidity value
#define HUM_UPPER_BOUND 60           // Upper bound of the humidity value

struct Sensor {
    char id[32];
    int value;
};

struct Sensor read_noise_value();
struct Sensor read_temperature();
struct Sensor read_humidity();

#endif
