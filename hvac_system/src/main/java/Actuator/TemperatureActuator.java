package Actuator;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.Properties;

public class TemperatureActuator extends Actuator {

    public TemperatureActuator() {
        this.parseArguments();
        this.setSystemOff();
        this.setHighThreshold(highThreshold);
        this.setLowThreshold(lowThreshold);
    }


    @Override
    public void manageSystem(float readValue) {
        if (readValue >= this.getHighThreshold()) {
            this.setSystemOff();
            System.out.println("High Temperature, turning Heating Off");
        } else if (readValue <= this.getLowThreshold()) {
            this.setSystemOn();
            System.out.println("Low Temperature, turning Heating On");
        }
    }


    private void parseArguments() {
        Properties config = new Properties();

        try {
            config.load(new FileInputStream(configPath));
            this.lowThreshold = Integer.parseInt(config.getProperty("temperatureLowThreshold"));
            this.highThreshold = Integer.parseInt(config.getProperty("temperatureHighThreshold"));

        } catch (IOException e) {
            throw new RuntimeException(e);
        }

    }
}