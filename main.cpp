#include "mbed.h"
#include "phyphoxBle.h"
#include <chrono>
#include <cstdint>
#include "VL53L1X.h"
#include "USBSerial.h"
#include "flashUtility.h"
#include <string>
#include <sstream>
#include <iomanip>
//https://github.com/VRaktion/mbed-VL53L1X-ULD

PinName LED = P0_17; //DISTANZ


PinName SCL = P0_21;
PinName SDA = P0_23;

PinName SHUT = P0_13;
PinName GPIO = P0_14;

DigitalOut myLED(LED); 

InterruptIn VL53L1X_INT(GPIO);
LowPowerTicker blinkTicker;
uint32_t flashAddress = 0x0FE000;
FLASH myCONFIG(flashAddress);

volatile bool dataReady = true;

int ledCounter=0;

uint8_t configData[20]={0};

bool ENABLE = false;
uint32_t volatile InterMeasurement = 100;
VL53L1X::DistanceModes activeDistanceMode = VL53L1X::DistanceModes::Long;
VL53L1X::TimingBudget activeTimingBudget = VL53L1X::TimingBudget::_33ms;

I2C i2c(SDA,SCL);

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
uint16_t MapByteToTimingInMS[7]={
    15,
    20,
    33,
    50,
    100,
    200,
    500
};

void newConfiguration(){

    if(ENABLE){       
        sensor.SetDistanceMode(activeDistanceMode);
        sensor.SetTimingBudgetInMs(activeTimingBudget);
        sensor.SetInterMeasurementInMs(InterMeasurement+5);
        sensor.StartRanging();
    }else {
        //sensor.SetToSleep();
    }
}
void receivedData() {           // get data from phyphox app
    PhyphoxBLE::read(&configData[0],30);
    ENABLE = configData[0]; 
    activeDistanceMode = MapByteToDistanceMode[configData[1]];
    activeTimingBudget = MapByteToTimingBudget[configData[2]];
    InterMeasurement = MapByteToTimingInMS[configData[2]];
    newConfiguration();
  }
void receivedSN() {           // get data from phyphox app
    uint8_t mySNBufferArray[2];
    PhyphoxBLE::readHWConfig(&mySNBufferArray[0],20);
    uint16_t intSN[1];
    intSN[0] = mySNBufferArray[1] << 8 | mySNBufferArray[0];
    myCONFIG.writeSN(intSN);
    
  }
  void blinkNtimes(int times){
    for(int i=0;i<times;i++){
        myLED=!myLED;
        ThisThread::sleep_for(200ms);
        myLED=!myLED;
        ThisThread::sleep_for(200ms);
    }
 }

 void getDeviceName(char* myDeviceName){
    uint16_t mySN[1];
     
     myCONFIG.readSN(mySN);
     
     if(mySN[0] == 0xFFFF){
         mySN[0]=0;
     }
     
    
    std::string s = std::to_string(mySN[0]);
    if ( s.size() < 4 ){
        s = std::string(4 - s.size(), '0') + s;
    }
    std::string S;
    S.append("Distanz D");
    S.append(s);
    strcpy(myDeviceName, S.c_str());  
 }


int main() {

    myLED=1; //turn led off
    blinkTicker.attach(&powerOn,100ms);
    sensor.SensorInit();
    ThisThread::sleep_for(50ms);
    newConfiguration();

    char DEVICENAME[30];
    getDeviceName(DEVICENAME);
    PhyphoxBLE::start(DEVICENAME);              // start BLE

    PhyphoxBLE::configHandler = &receivedData;
    PhyphoxBLE::hwConfigHandler = &receivedSN;
    //VL53L1X sensor(&i2c, SHUT, GPIO);
    //sensor.EnableInterrupt();
    Timer t;
    t.reset();
    t.start();
    uint8_t sensorReady[1] = {false};

    while (true) {                            // start loop.
        if(ENABLE){
                sensor.CheckForDataReady(sensorReady);
                if(sensorReady[0]){
                    uint16_t distance;
                    sensor.GetDistance(&distance);
                    float distanceF = distance;
                    float timestampF = 0.001*duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count();
                    PhyphoxBLE::write(distanceF,timestampF); 
                    sensor.ClearInterrupt();
                    ThisThread::sleep_for((InterMeasurement-3)*1ms);
                }else {
                    ThisThread::sleep_for(1ms);
                }          
        }else {
            ThisThread::sleep_for(100ms);
        }
    }
}
