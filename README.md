# MiddlewareProject2
## IoT device
### How to make it go
Move the IoT_implementation folder inside the contiki-ng-mw-2122 folder
### First Mosquitto setup
In your Client machine (VM - Ubuntu 16.04 32-bit Contiki NG Middleware Class AY 2021) modify the Mosquitto configuration file:
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
### Start your IoT device
Go in the correct folder:
```shell
cd IoT_stuff/IoT_stuff/IoT_device
```
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
### Simulation with Cooja
Use the Border-Router to communicate with the Broker outside Cooja
### More info
 - The IoT device will start to public to the public broker `mqtt.neslab.it:3200`, in order to receive the readings you must connect to such broker and you must subscribe to the correct topic. 
More about topics is well written here: http://www.steves-internet-guide.com/understanding-mqtt-topics/
- Thanks the bridge mode, the IoT devices publish to the local broker inside your machine and the local broker automatically forwards the messages to the public one.
