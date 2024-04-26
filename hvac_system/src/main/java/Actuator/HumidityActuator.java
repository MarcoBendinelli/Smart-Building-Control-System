package Actuator;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.Properties;

public class HumidityActuator extends Actuator{

    public HumidityActuator(){
        this.parseArguments();
        this.setSystemOff();
        this.setHighThreshold(highThreshold);
        this.setLowThreshold(lowThreshold);
    }


    @Override
    public void manageSystem(float readValue){
        if (readValue >= this.getHighThreshold()){
            this.setSystemOff();
            System.out.println("High humidity, Setting Humidifier Off");
        }
        else if (readValue <= this.getLowThreshold()){
            this.setSystemOn();
            System.out.println("Low humidity, Setting Humidifier On");
        }
    }


    private void parseArguments() {
        Properties config = new Properties();

        try {
            config.load(new FileInputStream(configPath));
            this.lowThreshold = Integer.parseInt(config.getProperty("humidityLowThreshold"));
            this.highThreshold = Integer.parseInt(config.getProperty("humidityHighThreshold"));

        } catch (IOException e) {
            throw new RuntimeException(e);
        }

    }


}
