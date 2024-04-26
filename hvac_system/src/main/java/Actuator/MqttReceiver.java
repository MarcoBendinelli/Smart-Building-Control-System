package Actuator;

import org.apache.commons.lang.StringUtils;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;


import java.io.FileInputStream;
import java.io.IOException;
import java.util.*;


import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.ProducerConfig;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.apache.kafka.common.serialization.StringSerializer;


public class MqttReceiver implements MqttCallback {

    private static final String configPath = "src/main/resources/config.cfg";
    MqttClient client;
    KafkaProducer<String, String> producer;
    private static Actuator actuator;
    private static String kafkaTopic;
    private static String MQTT_topic; //To parse from config
    private static String MqttServerAddr;
    private static String KafkaServerAddr ;


    public MqttReceiver(KafkaProducer<String, String> producer) {
        this.producer = producer;
    }

    /**
     * Simple parsing from config file
     */
    private static void parseArguments() {
        Properties config = new Properties();

        try {
            config.load(new FileInputStream(configPath));
            kafkaTopic = config.getProperty("generalTopic");
            MQTT_topic = config.getProperty("topic");
            MqttServerAddr = config.getProperty("MqttServerAddr");
            KafkaServerAddr = config.getProperty("KafkaServerAddr");

        } catch (IOException e) {
            throw new RuntimeException(e);
        }

    }


    public static void main(String[] args) {

        parseArguments();

        //Instantiate an actuator basing on the topic
        if (MQTT_topic.contains("temperature")){
            actuator = new TemperatureActuator();
            kafkaTopic = "temperature";
        }
        else{
            actuator = new HumidityActuator();
            kafkaTopic = "humidity";
        }

        final Properties props = new Properties();
        props.put(ProducerConfig.BOOTSTRAP_SERVERS_CONFIG, KafkaServerAddr);
        props.put(ProducerConfig.KEY_SERIALIZER_CLASS_CONFIG, StringSerializer.class.getName());
        props.put(ProducerConfig.VALUE_SERIALIZER_CLASS_CONFIG, StringSerializer.class.getName());

        //Instantiate the Kafka producer
        final KafkaProducer<String, String> producer = new KafkaProducer<>(props);
        new MqttReceiver(producer).run();


    }



    public void run() {
        try {

            client = new MqttClient(MqttServerAddr, MqttClient.generateClientId());  //instantiate MQTT Client
            client.connect();       //Connect to MQTT broker
            System.out.println("Client connected");
            client.setCallback(this);
            client.subscribe(MQTT_topic);        //Subscribe to topic
            System.out.println("Client subscribed to topic " + MQTT_topic);

        } catch (MqttException e) {
            e.printStackTrace();
        }
    }

    /**
     * Build and publish a kafka event
     */
    public void produceKafkaEvent(String message, String topic){
        final ProducerRecord<String, String> record = new ProducerRecord<>(kafkaTopic, topic, message);
        producer.send(record);
    }

    @Override
    public void connectionLost(Throwable cause) {
        System.out.println("connection lost because:" + cause);
    }

    /**
     * Called when an MQTT message arrives. This method calls the method on the actuator to manage it, and
     * produces a Kafka event to be read by the central consumer.
     */
    @Override
    public void messageArrived(String topic, MqttMessage message) {

        System.out.println(message);

        String stringMessage = message.toString();
        produceKafkaEvent(stringMessage, topic);

        try{
            float value = this.getValueFromMessage(stringMessage);
            System.out.println("a message arrived");
            System.out.println(value);
            actuator.manageSystem(value);
        }

        catch(Exception e){
            System.out.print("not a valid message");
        }


    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken token) {
        //Not used
    }

    /**
     * Retrieve the temp/humidity value from the message
     */
    public float getValueFromMessage(String message){
        return Float.parseFloat(StringUtils.substringBetween(message, ":", ","));
    }

}