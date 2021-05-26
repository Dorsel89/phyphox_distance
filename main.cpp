#include "mbed.h"
#include "phyphoxBle.h"
#include <chrono>
#include <cstdint>
#include "VL53L1X.h"
#include "USBSerial.h"

//https://github.com/VRaktion/mbed-VL53L1X-ULD

PinName LED = P0_17; //DISTANZ


PinName SCL = P0_21;
PinName SDA = P0_23;

PinName SHUT = P0_13;
PinName GPIO = P0_14;

DigitalOut myLED(LED); 
LowPowerTicker blinkTicker;
int ledCounter=0;

uint8_t configData[20]={0};

bool ENABLE = false;
VL53L1X::DistanceModes activeDistanceMode = VL53L1X::DistanceModes::Long;
VL53L1X::TimingBudget activeTimingBudget = VL53L1X::TimingBudget::_100ms;

I2C i2c(SDA,SCL);
uint8_t dataReady;
VL53L1X sensor(&i2c);

void powerOn() {
    if(ledCounter==9){
        myLED=0;
        ledCounter=0;
        return;
    }
    ledCounter+=1;
    myLED=1;
}

bool getNBit(uint8_t byte, int position){
    return byte & (1<<position);
}

VL53L1X::DistanceModes MapByteToDistanceMode[2] = {
    VL53L1X::DistanceModes::Short,
    VL53L1X::DistanceModes::Long
    };

VL53L1X::TimingBudget MapByteToTimingBudget[7]={
     VL53L1X::TimingBudget::_15ms,
    VL53L1X::TimingBudget::_20ms,
    VL53L1X::TimingBudget::_33ms,
    VL53L1X::TimingBudget::_50ms,
    VL53L1X::TimingBudget::_100ms,
    VL53L1X::TimingBudget::_200ms,
    VL53L1X::TimingBudget::_500ms
};


void newConfiguration(){

    if(ENABLE){       
        sensor.SetDistanceMode(activeDistanceMode);
        sensor.SetTimingBudgetInMs(activeTimingBudget);
        sensor.StartRanging();
    }else {
        //sensor.SetToSleep();
    }
}
void receivedData() {           // get data from phyphox app
    PhyphoxBLE::read(&configData[0],20);
    ENABLE = configData[0]; 
    activeDistanceMode = MapByteToDistanceMode[configData[1]];
    activeTimingBudget = MapByteToTimingBudget[configData[2]];
    newConfiguration();
  }

  void blinkNtimes(int times){
    for(int i=0;i<times;i++){
        myLED=!myLED;
        ThisThread::sleep_for(200ms);
        myLED=!myLED;
        ThisThread::sleep_for(200ms);
    }
 }
int main() {

    myLED=1; //turn led off
    blinkTicker.attach(&powerOn,100ms);
    sensor.SensorInit();
    ThisThread::sleep_for(50ms);
    newConfiguration();
    
    PhyphoxBLE::start("distanz");              // start BLE
    PhyphoxBLE::configHandler = &receivedData;
    //VL53L1X sensor(&i2c, SHUT, GPIO);
    //sensor.EnableInterrupt();
    
    while (true) {                            // start loop.
        if(ENABLE){
            
            sensor.CheckForDataReady(&dataReady);
            if(dataReady){
                uint16_t distance;
                sensor.GetDistance(&distance);
                float distanceF = distance;
                PhyphoxBLE::write(distanceF);    
            }
            ThisThread::sleep_for(5ms);
        }else {
            ThisThread::sleep_for(100ms);
        }
    }
}
