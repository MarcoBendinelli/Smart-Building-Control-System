package DataAnalyzer;

import com.opencsv.CSVWriter;
import org.apache.commons.lang.StringUtils;
import org.apache.kafka.clients.consumer.ConsumerConfig;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;
import org.apache.kafka.common.serialization.StringDeserializer;

import java.io.*;
import java.time.Duration;
import java.time.temporal.ChronoUnit;
import java.util.Properties;
import java.util.concurrent.ExecutionException;

public class AnalyzerConsumer {
    private static final String configPath = "src/main/resources/config.cfg";

    private static String groupID;
    private static final String temperatureTopic = "temperature";

    private static final String humidityTopic = "humidity";

    private static File file;
    private static FileWriter outfile;
    private static CSVWriter writer;
    private static final int numPartitions = 1;

    private static final short replicationFactor = 1;

    private static String KafkaServerAddr ;

    private static final String filepath = "target/generated-sources/out.csv";
    private static final boolean autoCommit = true;
    private static final int autoCommitIntervalMs = 15000;

    private static final String offsetResetStrategy = "earliest";


    /**
     * Writes each record in the result.csv file
     */
    public static void writeOnCsv(ConsumerRecord<String, String> record) throws IOException {

        writer = new CSVWriter(outfile);
        file = new File(filepath);

        outfile = new FileWriter(file, true);
        writer = new CSVWriter(outfile);

        String key = record.key().replace("iot/", "");
        String message = record.value();

        String building = StringUtils.substringBefore(key, "/");
        key = key.replace(building + "/", "");
        String room = StringUtils.substringBefore(key, "/");
        key = key.replace(room + "/" , "");
        String type = key;

        message = message.substring(message.indexOf(":") + 1);
        String value = StringUtils.substringBefore(message, ",");
        message = message.substring(message.indexOf(":" ) + 1);
        String dateTime = message;

        dateTime = StringUtils.substringBetween(dateTime, String.valueOf('"'), String.valueOf('"'));
        String[] data = {type, building, room, dateTime, value};
        writer.writeNext(data);

        writer.close();
    }

    /**
     * Creates the result.csv file and writes the header on it
     */
    public static void initializeCSV() throws IOException {

        file = new File(filepath);

        outfile = new FileWriter(file);
        writer = new CSVWriter(outfile);

        String[] header = {"Type", "Building", "Room", "Date_Time", "Value"};

        writer.writeNext(header);
        System.out.println("Header written on CSV");

        writer.close();

    }

    /**
     * Simple parsing from config file
     */
    private static void parseArguments() {
        Properties config = new Properties();

        try {
            config.load(new FileInputStream(configPath));
            KafkaServerAddr = config.getProperty("KafkaServerAddr");
            groupID = config.getProperty("groupID");

        } catch (FileNotFoundException e) {
            throw new RuntimeException(e);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

    }


    public static void main(String[] args) throws ExecutionException, InterruptedException, IOException {

        parseArguments();

        TopicManager topicManager = new TopicManager(KafkaServerAddr);
        topicManager.addTopic(temperatureTopic, numPartitions, replicationFactor);
        topicManager.addTopic(humidityTopic, numPartitions, replicationFactor);

        System.out.println("Topics Name List: " + topicManager.getTopicList());

        initializeCSV();


        final Properties props = new Properties();
        props.put(ConsumerConfig.BOOTSTRAP_SERVERS_CONFIG, KafkaServerAddr);
        props.put(ConsumerConfig.GROUP_ID_CONFIG, groupID);
        props.put(ConsumerConfig.ENABLE_AUTO_COMMIT_CONFIG, String.valueOf(autoCommit));
        props.put(ConsumerConfig.AUTO_COMMIT_INTERVAL_MS_CONFIG, String.valueOf(autoCommitIntervalMs));
        props.put(ConsumerConfig.AUTO_OFFSET_RESET_CONFIG, offsetResetStrategy);
        props.put(ConsumerConfig.KEY_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class.getName());
        props.put(ConsumerConfig.VALUE_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class.getName());


        KafkaConsumer<String, String> consumer = new KafkaConsumer<String, String>(props);
        consumer.subscribe(topicManager.getTopicList());

        while (true) {
            final ConsumerRecords<String, String> records = consumer.poll(Duration.of(5, ChronoUnit.MINUTES));
            for (final ConsumerRecord<String, String> record : records) {
                System.out.println("Partition: " + record.partition() +
                        "\tOffset: " + record.offset() +
                        "\tKey: " + record.key() +
                        "\tValue: " + record.value()
                );
                writeOnCsv(record);
            }
        }


    }
}