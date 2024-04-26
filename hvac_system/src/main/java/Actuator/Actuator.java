package Actuator;

public abstract class Actuator {

    public static final String configPath = "src/main/resources/config.cfg";

    public float highThreshold;
    public float lowThreshold;
    private boolean isSystemOn;

    public float getHighThreshold() {
        return highThreshold;
    }

    public void setHighThreshold(float highThreshold) {
        this.highThreshold = highThreshold;
    }

    public float getLowThreshold() {
        return lowThreshold;
    }

    public void setLowThreshold(float lowThreshold) {
        this.lowThreshold = lowThreshold;
    }

    public boolean isSystemOn() {
        return isSystemOn;
    }

    public void setSystemOn() {
        isSystemOn = true;
    }

    public void setSystemOff() {isSystemOn = false;}

    /**
     * Simple method to turn on/off actuators
     */
    public abstract void manageSystem(float readValue);
}
