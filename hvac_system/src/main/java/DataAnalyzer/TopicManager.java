package DataAnalyzer;

import org.apache.kafka.clients.admin.*;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.*;
import java.util.concurrent.ExecutionException;

import org.apache.zookeeper.ZooKeeper;


/**
 * Class used by the central consumer to create Kafka topics
 */
public class TopicManager  {

    private final AdminClient adminClient;



    public TopicManager(String serverAddr){

        Properties prop = new Properties();
        prop.put(AdminClientConfig.BOOTSTRAP_SERVERS_CONFIG, serverAddr);
        this.adminClient = AdminClient.create(prop);
    }

    /**
     * Retrieve topic list
     */
    public Set<String> getTopicList() throws ExecutionException, InterruptedException {
        ListTopicsResult listTopicsResult = this.adminClient.listTopics();
        return listTopicsResult.names().get();
    }

    /**
     * Add a new Kafka topic
     */
    public void addTopic(String topicName, int topicPartitions, short replicationFactor) throws ExecutionException, InterruptedException {
        List<NewTopic> topics = new ArrayList<>();
        if(this.getTopicList().contains(topicName)){
            return;
        }


        System.out.println("Adding topic " + topicName + " with " + topicPartitions + " partitions");
        topics.add(new NewTopic(topicName, topicPartitions, replicationFactor));
        CreateTopicsResult createResult = adminClient.createTopics(topics);
        createResult.all().get();
        System.out.println("Done!");
    }


}