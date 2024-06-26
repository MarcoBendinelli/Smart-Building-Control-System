/*---------------------------------------------------------------------------*/

/** 

 * IoT device

 */

/*---------------------------------------------------------------------------*/

#include "contiki.h"

#include "cfs/cfs.h" 

#include "mqtt.h"

#include "rpl.h"

#include "net/ipv6/uip.h"

#include "net/ipv6/sicslowpan.h"

#include "sys/etimer.h"

#include "sys/ctimer.h"

#include <time.h>

#include "leds.h"

#include "sensor.h"



#include "sys/log.h"

#define LOG_MODULE "MQTT-CLIENT"

#define LOG_LEVEL LOG_LEVEL_INFO



#include <string.h>

#include <stdio.h>

#include <stdlib.h>

/*---------------------------------------------------------------------------*/

/*

 * Publish to a local MQTT broker (e.g. mosquitto) running on

 * the node that hosts your border router

 */

static const char *broker_ip = MQTT_CLIENT_BROKER_IP_ADDR;

#define DEFAULT_ORG_ID              "mqtt-client"

/*---------------------------------------------------------------------------*/

/*

 * A timeout used when waiting for something to happen (e.g. to connect or to

 * disconnect)

 */

#define STATE_MACHINE_PERIODIC     (CLOCK_SECOND >> 1)

/*---------------------------------------------------------------------------*/

/* Provide visible feedback via LEDS during various states */

/* When connecting to broker */

#define CONNECTING_LED_DURATION    (CLOCK_SECOND >> 2)



/* Each time we try to publish */

#define PUBLISH_LED_ON_DURATION    (CLOCK_SECOND)

/*---------------------------------------------------------------------------*/

/* Connections and reconnections */

#define RETRY_FOREVER              0xFF

#define RECONNECT_INTERVAL         (CLOCK_SECOND * 2)

/*---------------------------------------------------------------------------*/

/*

 * Number of times to try reconnecting to the broker.

 * Can be a limited number (e.g. 3, 10 etc) or can be set to RETRY_FOREVER

 */

#define RECONNECT_ATTEMPTS         RETRY_FOREVER

#define CONNECTION_STABLE_TIME     (CLOCK_SECOND * 5)

static struct timer connection_life;

static uint8_t connect_attempt;

/*---------------------------------------------------------------------------*/

/* Various states */

static uint8_t state;

#define STATE_INIT            0

#define STATE_REGISTERED      1

#define STATE_CONNECTING      2

#define STATE_CONNECTED       3

#define STATE_PUBLISHING      4

#define STATE_DISCONNECTED    5

#define STATE_NEWCONFIG       6

#define STATE_CONFIG_ERROR 0xFE

#define STATE_ERROR        0xFF

/*---------------------------------------------------------------------------*/

#define CONFIG_ORG_ID_LEN        32

#define CONFIG_TYPE_ID_LEN       32

#define CONFIG_AUTH_TOKEN_LEN    32

#define CONFIG_CMD_TYPE_LEN       8

#define CONFIG_IP_ADDR_STR_LEN   64

/*---------------------------------------------------------------------------*/

/* A timeout used when waiting to connect to a network */

#define NET_CONNECT_PERIODIC        (CLOCK_SECOND >> 2)

#define NO_NET_LED_DURATION         (NET_CONNECT_PERIODIC >> 1)

/*---------------------------------------------------------------------------*/

/* Default configuration values */

#define DEFAULT_TYPE_ID             "native"

#define DEFAULT_AUTH_TOKEN          "AUTHZ"

#define DEFAULT_SUBSCRIBE_CMD_TYPE  "+"

#define DEFAULT_BROKER_PORT         1883

#define DEFAULT_PUBLISH_INTERVAL    (10 * CLOCK_SECOND)

#define DEFAULT_KEEP_ALIVE_TIMER    60

/*---------------------------------------------------------------------------*/

PROCESS_NAME(mqtt_client_process);

AUTOSTART_PROCESSES(&mqtt_client_process);

/*---------------------------------------------------------------------------*/

/**

 * \brief Data structure declaration for the MQTT client configuration

 */

typedef struct mqtt_client_config {

    char org_id[CONFIG_ORG_ID_LEN];

    char type_id[CONFIG_TYPE_ID_LEN];

    char auth_token[CONFIG_AUTH_TOKEN_LEN];

    char broker_ip[CONFIG_IP_ADDR_STR_LEN];

    char cmd_type[CONFIG_CMD_TYPE_LEN];

    clock_time_t pub_interval;

    uint16_t broker_port;

} mqtt_client_config_t;

/*---------------------------------------------------------------------------*/

/* Maximum TCP segment size for outgoing segments of our socket */

#define MAX_TCP_SEGMENT_SIZE    256

/*---------------------------------------------------------------------------*/

/*

 * Buffers for Client ID and Topic.

 * Make sure they are large enough to hold the entire respective string

 *

 * We also need space for the null termination

 */

#define BUFFER_SIZE 64

static char client_id[BUFFER_SIZE];

static char device_id[BUFFER_SIZE];

static char pub_topic[BUFFER_SIZE];

static char sub_topic[BUFFER_SIZE];

/*---------------------------------------------------------------------------*/

/*

 * The main MQTT buffers.

 * We will need to increase if we start publishing more data.

 */

#define APP_BUFFER_SIZE 512

static struct mqtt_connection conn;

static char app_buffer[APP_BUFFER_SIZE];

/*---------------------------------------------------------------------------*/

static struct mqtt_message *msg_ptr = 0;

static struct etimer publish_periodic_timer;

static struct ctimer ct;

static char *buf_ptr;

static uint16_t seq_nr_value = 0;

/*---------------------------------------------------------------------------*/

static mqtt_client_config_t conf;

/*---------------------------------------------------------------------------*/

PROCESS(mqtt_client_process, "MQTT client");

/*---------------------------------------------------------------------------*/

int ipaddr_sprintf(char *buf, uint8_t buf_len, const uip_ipaddr_t *addr)

{

    uint16_t a;

    uint8_t len = 0;

    int i, f;

    for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {

        a = (addr->u8[i] << 8) + addr->u8[i + 1];

        if(a == 0 && f >= 0) {

            if(f++ == 0) {

                len += snprintf(&buf[len], buf_len - len, "::");

            }

        } else {

            if(f > 0) {

                f = -1;

            } else if(i > 0) {

                len += snprintf(&buf[len], buf_len - len, ":");

            }

            len += snprintf(&buf[len], buf_len - len, "%x", a);

        }

    }



    return len;

}

/*---------------------------------------------------------------------------*/

static void publish_led_off(void *d)

{

    leds_off(MQTT_CLIENT_STATUS_LED);

}

/*---------------------------------------------------------------------------*/

static void pub_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk,

            uint16_t chunk_len)

{

    LOG_INFO("Pub handler: topic='%s' (len=%u), chunk_len=%u\n", topic, topic_len,

             chunk_len);



    /* If the format != json, ignore */

    if(strncmp(&topic[topic_len - 4], "json", 4) != 0) {

        LOG_ERR("Incorrect format\n");

        return;

    }

}

/*---------------------------------------------------------------------------*/

static void mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data)

{

    switch(event) {

        case MQTT_EVENT_CONNECTED: {

            LOG_INFO("Application has a MQTT connection!\n");

            timer_set(&connection_life, CONNECTION_STABLE_TIME);

            state = STATE_CONNECTED;

            break;

        }

        case MQTT_EVENT_DISCONNECTED: {

            LOG_INFO("MQTT Disconnect: reason %u\n", *((mqtt_event_t *)data));



            state = STATE_DISCONNECTED;

            process_poll(&mqtt_client_process);

            break;

        }

        case MQTT_EVENT_PUBLISH: {

            msg_ptr = data;



            if(msg_ptr->first_chunk) {

                msg_ptr->first_chunk = 0;

                LOG_INFO("Application received a publish on topic '%s'; payload "

                         "size is %i bytes\n",

                         msg_ptr->topic, msg_ptr->payload_length);

            }



            pub_handler(msg_ptr->topic, strlen(msg_ptr->topic), msg_ptr->payload_chunk,

                        msg_ptr->payload_length);

            break;

        }

        case MQTT_EVENT_SUBACK: {

            LOG_INFO("Application is subscribed to topic successfully\n");

            break;

        }

        case MQTT_EVENT_UNSUBACK: {

            LOG_INFO("Application is unsubscribed to topic successfully\n");

            break;

        }

        case MQTT_EVENT_PUBACK: {

            LOG_INFO("Publishing complete\n");

            break;

        }

        default:

            LOG_WARN("Application got a unhandled MQTT event: %i\n", event);

            break;

    }

}

/*---------------------------------------------------------------------------*/

static int construct_pub_topic(void)

{

    int len = snprintf(pub_topic, BUFFER_SIZE, MQTT_CLIENT_PUBLISH_TOPIC);



    /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */

    if(len < 0 || len >= BUFFER_SIZE) {

        LOG_ERR("Pub topic: %d, buffer %d\n", len, BUFFER_SIZE);

        return 0;

    }



    return 1;

}

/*---------------------------------------------------------------------------*/

static int construct_sub_topic(void)

{

    int len = snprintf(sub_topic, BUFFER_SIZE, MQTT_CLIENT_SUB_TOPIC);



    /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */

    if(len < 0 || len >= BUFFER_SIZE) {

        LOG_INFO("Sub topic: %d, buffer %d\n", len, BUFFER_SIZE);

        return 0;

    }



    return 1;

}

/*---------------------------------------------------------------------------*/

static int construct_client_id(void)

{

    int len = snprintf(client_id, BUFFER_SIZE, "d:%s:%s:%02x%02x%02x%02x%02x%02x",

                       conf.org_id, conf.type_id,

                       linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],

                       linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],

                       linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);



    /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */

    if(len < 0 || len >= BUFFER_SIZE) {

        LOG_INFO("Client ID: %d, Buffer %d\n", len, BUFFER_SIZE);

        return 0;

    }

    

    len = snprintf(device_id, BUFFER_SIZE, "%02x%02x%02x%02x%02x%02x",

                   linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],

                   linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],

                   linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);



    return 1;

}

/*---------------------------------------------------------------------------*/

static void update_config(void)

{

    if(construct_client_id() == 0) {

        /* Fatal error. Client ID larger than the buffer */

        state = STATE_CONFIG_ERROR;

        return;

    }



    if(construct_sub_topic() == 0) {

        /* Fatal error. Topic larger than the buffer */

        state = STATE_CONFIG_ERROR;

        return;

    }



    if(construct_pub_topic() == 0) {

        /* Fatal error. Topic larger than the buffer */

        state = STATE_CONFIG_ERROR;

        return;

    }



    /* Reset the counter */

    seq_nr_value = 0;



    state = STATE_INIT;



    /*

     * Schedule next timer event ASAP

     *

     * If we entered an error state then we won't do anything when it fires

     *

     * Since the error at this stage is a config error, we will only exit this

     * error state if we get a new config

     */

    etimer_set(&publish_periodic_timer, 0);



    return;

}

/*---------------------------------------------------------------------------*/

static void init_config()

{

    srand(time(NULL));

    /* Populate configuration with default values */

    memset(&conf, 0, sizeof(mqtt_client_config_t));



    memcpy(conf.org_id, DEFAULT_ORG_ID, strlen(DEFAULT_ORG_ID));

    memcpy(conf.type_id, DEFAULT_TYPE_ID, strlen(DEFAULT_TYPE_ID));

    memcpy(conf.auth_token, DEFAULT_AUTH_TOKEN, strlen(DEFAULT_AUTH_TOKEN));

    memcpy(conf.broker_ip, broker_ip, strlen(broker_ip));

    memcpy(conf.cmd_type, DEFAULT_SUBSCRIBE_CMD_TYPE, 1);



    conf.broker_port = DEFAULT_BROKER_PORT;

    conf.pub_interval = DEFAULT_PUBLISH_INTERVAL;

}

/*---------------------------------------------------------------------------*/

static void subscribe(void)

{

    mqtt_status_t status;



    status = mqtt_subscribe(&conn, NULL, sub_topic, MQTT_QOS_LEVEL_0);



    LOG_INFO("Subscribing\n");

    if(status == MQTT_STATUS_OUT_QUEUE_FULL) {

        LOG_INFO("Tried to subscribe but command queue was full!\n");

    }

}

/* The function which is effectly in charge to publish the message*/

static void publish() {

    struct Sensor temperature_sensor = read_temperature();

    //struct Sensor humidity_sensor = read_humidity();

    

    int len;

    int remaining = APP_BUFFER_SIZE;

    time_t t = time(NULL);

    struct tm tm = *localtime(&t);



    buf_ptr = app_buffer;



    seq_nr_value++;

    

    len = snprintf(buf_ptr, remaining,

                   "{\"temp\":%d,",

                   temperature_sensor.value);

/*

    len = snprintf(buf_ptr, remaining,

                   "\"value\":%d",

                   humidity_sensor.value);

*/



    remaining -= len;

    buf_ptr += len;



    len = snprintf(buf_ptr, remaining,

                   "\"date_time\":\"%d-%02d-%02d %02d:%02d:%02d\"",

                   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);



    remaining -= len;

    buf_ptr += len;



    len = snprintf(buf_ptr, remaining, "}");



    if (len < 0 || len >= remaining) {

        LOG_ERR("Error!\n");

        return;

    }



    mqtt_publish(&conn, NULL, pub_topic, (uint8_t *)app_buffer, strlen(app_buffer),

                 MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);



    LOG_INFO("Just published: %s\n", app_buffer);

}

/*---------------------------------------------------------------------------*/

static void connect_to_broker(void)

{

    /* Connect to MQTT server */

    mqtt_connect(&conn, conf.broker_ip, conf.broker_port,

                 conf.pub_interval * 3);



    state = STATE_CONNECTING;

}

/*---------------------------------------------------------------------------*/

static void state_machine(void)

{

    switch(state) {

        case STATE_INIT:

            /* If we have just been configured register MQTT connection */

            mqtt_register(&conn, &mqtt_client_process, client_id, mqtt_event,

                          MAX_TCP_SEGMENT_SIZE);



            mqtt_set_username_password(&conn, "use-token-auth",

                                       conf.auth_token);



            /* _register() will set auto_reconnect; we don't want that */

            conn.auto_reconnect = 0;

            connect_attempt = 1;



            state = STATE_REGISTERED;

            LOG_INFO("Init\n");



            /* Continue */



        case STATE_REGISTERED:

            if(uip_ds6_get_global(ADDR_PREFERRED) != NULL) {

                /* Registered and with a global IPv6 address, connect! */

                LOG_INFO("Joined network! Connect attempt %u\n", connect_attempt);

                connect_to_broker();

            } else {

                leds_on(MQTT_CLIENT_STATUS_LED);

                ctimer_set(&ct, NO_NET_LED_DURATION, publish_led_off, NULL);

            }

            etimer_set(&publish_periodic_timer, NET_CONNECT_PERIODIC);

            return;

            break;



        case STATE_CONNECTING:

            leds_on(MQTT_CLIENT_STATUS_LED);

            ctimer_set(&ct, CONNECTING_LED_DURATION, publish_led_off, NULL);

            /* Not connected yet. Wait */

            LOG_INFO("Connecting: retry %u...\n", connect_attempt);

            break;



        case STATE_CONNECTED:



        case STATE_PUBLISHING:

            /* If the timer expired, the connection is stable */

            if(timer_expired(&connection_life)) {

                /*

                 * Intentionally using 0 here instead of 1: We want RECONNECT_ATTEMPTS

                 * attempts if we disconnect after a successful connect

                 */

                connect_attempt = 0;

            }



            if(mqtt_ready(&conn) && conn.out_buffer_sent) {

                /* Connected; publish */

                if(state == STATE_CONNECTED) {

                    subscribe();

                    state = STATE_PUBLISHING;

                } else {

                    leds_on(MQTT_CLIENT_STATUS_LED);

                    ctimer_set(&ct, PUBLISH_LED_ON_DURATION, publish_led_off, NULL);

                    publish();

                }

                etimer_set(&publish_periodic_timer, conf.pub_interval);



                //LOG_INFO("Publishing\n");

                /* Return here so we don't end up rescheduling the timer */

                return;

            } else {

                /*

                 * Our publish timer fired, but some MQTT packet is already in flight

                 * (either not sent at all, or sent but not fully ACKd)

                 *

                 * This can mean that we have lost connectivity to our broker or that

                 * simply there is some network delay. In both cases, we refuse to

                 * trigger a new message and we wait for TCP to either ACK the entire

                 * packet after retries, or to timeout and notify us

                 */

                LOG_INFO("Publishing... (MQTT state=%d, q=%u)\n", conn.state,

                         conn.out_queue_full);

            }

            break;



        case STATE_DISCONNECTED:

            LOG_INFO("Disconnected\n");

            if(connect_attempt < RECONNECT_ATTEMPTS ||

               RECONNECT_ATTEMPTS == RETRY_FOREVER) {

                /* Disconnect and backoff */

                clock_time_t interval;

                mqtt_disconnect(&conn);

                connect_attempt++;



                interval = connect_attempt < 3 ? RECONNECT_INTERVAL << connect_attempt :

                           RECONNECT_INTERVAL << 3;



                LOG_INFO("Disconnected: attempt %u in %lu ticks\n", connect_attempt, interval);



                etimer_set(&publish_periodic_timer, interval);



                state = STATE_REGISTERED;

                return;

            } else {

                /* Max reconnect attempts reached; enter error state */

                state = STATE_ERROR;

                LOG_ERR("Aborting connection after %u attempts\n", connect_attempt - 1);

            }

            break;



        case STATE_CONFIG_ERROR:

            /* Idle away. The only way out is a new config */

            LOG_ERR("Bad configuration.\n");

            return;



        case STATE_ERROR:

        default:

            leds_on(MQTT_CLIENT_STATUS_LED);

            /*

             * 'default' should never happen

             *

             * If we enter here it's because of some error. Stop timers. The only thing

             * that can bring us out is a new config event

             */

            LOG_INFO("Default case: State=0x%02x\n", state);

            return;

    }



    /* If we didn't return so far, reschedule ourselves */

    etimer_set(&publish_periodic_timer, STATE_MACHINE_PERIODIC);

}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(mqtt_client_process, ev, data)

{



    PROCESS_BEGIN();



    LOG_INFO("MQTT client Process\n");



    init_config();

    update_config();



    /* Main loop */

    while(1) {



        PROCESS_YIELD();



        if (ev == PROCESS_EVENT_TIMER && data == &publish_periodic_timer) {

            state_machine();

        }

    }



    PROCESS_END();

}

/*---------------------------------------------------------------------------*/