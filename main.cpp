#include "mbed.h"
#include "phyphoxBle.h"
#include <chrono>
#include <cstdint>
#include "VL53L1X.h"
//https://github.com/VRaktion/mbed-VL53L1X-ULD

PinName LED = P0_17; //DISTANZ
//PinName LED = P0_4; //ELEHRE 


PinName SCL = P0_21;
PinName SDA = P0_23;

PinName SHUT = P0_13;
PinName GPIO = P0_14;

DigitalOut myLED(LED); 
LowPowerTicker blinkTicker;
int ledCounter=0;

I2C i2c(SDA,SCL);
uint8_t dataReady;


void powerOn() {
    if(ledCounter==9){
        myLED=0;
        ledCounter=0;
        return;
    }
    ledCounter+=1;
    myLED=1;
}

int main() {

    myLED=1; //turn led off
    blinkTicker.attach(&powerOn,100ms);

    PhyphoxBLE::start("distanz");              // start BLE

    //VL53L1X sensor(&i2c, SHUT, GPIO);
    VL53L1X sensor(&i2c);
    sensor.SensorInit();
    //sensor.SetToSleep();
    //sensor.EnableInterrupt();
    sensor.SetDistanceMode(VL53L1X::DistanceModes::Long);
    sensor.SetTimingBudgetInMs(VL53L1X::TimingBudget::_200ms);
    sensor.StartRanging();
    

    while (true) {                            // start loop.
        sensor.CheckForDataReady(&dataReady);
        if(dataReady){
            uint16_t distance;
            sensor.GetDistance(&distance);
            float distanceF = distance;
            PhyphoxBLE::write(distanceF);
        }
    }
}
