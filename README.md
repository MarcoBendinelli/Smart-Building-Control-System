# Smart Building Control System :house:

The system is designed to manage **sensors** and **actuators** deployed in multiple buildings across different neighborhoods. It orchestrates simple control loops within each room of a building while collecting and analyzing long-term environmental data from available sensors. Inspiration for the data model is drawn from open datasets such as _sensor.community_.

Sensors installed in rooms monitor environmental data like temperature and humidity, using this information to activate commands on nearby actuators. For instance, if the temperature or humidity deviates from a comfortable range, an HVAC controller in the room is activated until the data returns to the desired range. Sensors are IoT devices, while actuators are more powerful devices capable of running a full-fledged operating system.

## Implementation

For inter-room communication, **Kafka** was chosen, while the sensor-actuator messaging system is implemented using **MQTT**. It is assumed that there is a single sensor for every actuator in the room, considering two possible types of actuators: one for temperature and one for humidity management.

The system functions as both a _producer_ and _consumer_: it consumes MQTT messages sent by sensors and produces Kafka events for the **Analyzer**. This system acts as a Kafka consumer, receiving every message from every room of the entire system and storing it for long-term analysis.

<p align="center">
  <img src="https://github.com/MarcoBendinelli/Smart-Building-Control-System/assets/79930488/3d517fb3-c8bb-4d98-817b-94b4f46e5694" alt="Architecture Diagram" width="800">
</p>

## Design Choices

Due to sensors being unable to run a full-fledged operating system, **Contiki-NG** and **MQTT** were chosen for communication between sensors and actuators. Conversely, **Kafka** was selected for communication between actuators and a central server collecting all data. In the proposed version, only one broker and one partition are used, serving as a central processing unit for every building. In case of massive data, the project can be easily scaled to include more brokers and computing units.

## Guide

### IoT device

#### First Mosquitto setup
In your Client machine (VM - Ubuntu) modify the Mosquitto configuration file:
```shell
cd /etc/mosquitto
```
```shell
sudo nano mosquitto.conf
```
And add the following lines in the bottom of the file:
```shell
  connection bridge-01
  address mqtt.neslab.it:3200
  topic # out 0
  topic # in 0
```
Save, Exit and Restart your machine.

#### Start your IoT device
Go in the correct folder
Compile your code:
```shell
make TARGET=native
```
Run your code:
```shell
sudo ./IoT-device.native
```
- Note1: The sudo command is necessary to run the code inside your network.
- Note2: The mote publishes a reading every 10 seconds, if you want to change such behaviour, change the number at line 86: `#define DEFAULT_PUBLISH_INTERVAL    (10 * CLOCK_SECOND)`
- Note3: Look at the project-conf.h file for changing the publish topic
  
#### Simulation with Cooja
Use the Border-Router to communicate with the Broker outside Cooja

### More info
 - The IoT device will start to publish to a public broker, in order to receive the readings you must connect to such broker and you must subscribe to the correct topic. 
More about topics is well written here: http://www.steves-internet-guide.com/understanding-mqtt-topics/
- Thanks the bridge mode, the IoT devices publish to the local broker inside your machine and the local broker automatically forwards the messages to the public one.
