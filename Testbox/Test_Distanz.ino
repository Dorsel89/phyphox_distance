#include <SD.h>

#define CONFIGMODE 0
#define SHORT_INIT 1
#define SHORT 2
#define LONG_INIT 3
#define LONG 4


float targetShort = 1384;
float targetLong = 1570;
float limit = 10;
float limitSTD = 10; //+-10mm rauschen

const int nDatapoints = 50;


float target[10];
float limitArray[10];

uint16_t SERIALNUMBER[1];

File root;
int storedDataFiles = 0;

String DEFAULTDEVICENAME = "Distance D0000";

String Address = "";
char* serialNumberChar = "";
String SerialNumberString = "";
char buffer[21];

volatile bool INIT=false;

#include "BLEDevice.h"
// The remote service we wish to connect to.
static BLEUUID serviceUUID("cddf1001-30f7-4671-8b43-5e40ba53514a");
static BLEUUID serviceHWUUID("cddf9011-30f7-4671-8b43-5e40ba53514a");

// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("cddf1002-30f7-4671-8b43-5e40ba53514a");
static BLEUUID    configUUID("cddf1003-30f7-4671-8b43-5e40ba53514a");
static BLEUUID    configHWUUID("cddf9012-30f7-4671-8b43-5e40ba53514a");


bool printData = true;

bool shortInit=false;
bool longInit=false;

uint8_t configData[20] = {0};

int volatile dataPointsRequired =0;


bool deviceWorks[2] = {false};

int countFiles();
long volatile modeDataPoint = 0;
volatile bool measuring = false;
volatile bool snOnSDCard = false;

int volatile currentMode = 0;
bool doSomeStatistics(int dataPoints, float* median, float* varianz);
void checkLimits(float* median, float* varianz);
void saveData();

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = true;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pHWRemoteCharacteristic;
static BLERemoteCharacteristic* configCharacteristic;
static BLERemoteCharacteristic* configHWCharacteristic;

static BLEAdvertisedDevice* myDevice;
uint8_t dataReceived[20] = {0};


float D[nDatapoints]={0};
float T[nDatapoints]={0};


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  float myfloats[2];
  memcpy(&myfloats[0], pData, 8);
  if (measuring && modeDataPoint < dataPointsRequired) { 
    D[modeDataPoint] = myfloats[0];
    if(printData){
      Serial.print("D: ");
      Serial.print(D[modeDataPoint],5);
      }
    T[modeDataPoint] = myfloats[1];
    if(printData){
      Serial.print(" T: ");
      Serial.print(T[modeDataPoint],5);
      }
                     
      if(printData){
        Serial.println("");
      }
    modeDataPoint += 1;
    
  }
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      currentMode = 0;
      snOnSDCard=false;
      shortInit = false;
      longInit=false;
    }

    void onDisconnect(BLEClient* pclient) {
      connected = false;
      currentMode = 0;
      Serial.println("disconnected");
    }
};


bool connectToServer() {
  Serial.print("Forming a connection to ");
  Address = myDevice->getAddress().toString().c_str();
  Serial.println(Address);
  Address.replace(":", "");

  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  BLERemoteService* pHWRemoteService = pClient->getService(serviceHWUUID);
  if (pHWRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceHWUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");


  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  pHWRemoteCharacteristic = pHWRemoteService->getCharacteristic(configHWUUID);
  configCharacteristic = pRemoteService->getCharacteristic(configUUID);
  if (pHWRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
}
/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      //Serial.print("BLE Advertised Device found: ");
      //Serial.println(advertisedDevice.toString().c_str());
      String currentName = advertisedDevice.toString().c_str();
      //Serial.println(currentName);
      // We have found a device, let us now see if it contains the service we are looking for.
      //if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID) && currentName == DEFAULTDEVICENAME) {
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceHWUUID)) {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;

      } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  target[SHORT]=targetShort;
  target[LONG]=targetLong;
  limitArray[SHORT]=limit;
  limitArray[LONG]=limit;
  
  Serial.begin(115200);
  delay(1000);
  Serial.println("");
  Serial.println("");
  Serial.println("STARTE DISTANZ TESTCENTER");
  BLEDevice::init("");
  //pinMode(LED_BUILTIN,OUTPUT);
  pinMode(32,  OUTPUT );
  pinMode(33, OUTPUT);
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);

  digitalWrite(32, 1);
  digitalWrite(33, 1);
  digitalWrite(25, 1);
  digitalWrite(26, 1);

  if (!SD.begin(4)) {
    Serial.println("sd initialization failed!");
    return;
  }


  storedDataFiles = countFiles();

  Serial.print(storedDataFiles);
  Serial.println(" txt files found");
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    Serial.println("");
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    if(currentMode==CONFIGMODE){
        configData[0]=0x01;
        configData[1]=0x01;
        configData[2]=0x04;
        configCharacteristic->writeValue(&configData[0], 20);
        currentMode = SHORT_INIT;
        getSN();
        return;
      }
    if(currentMode==SHORT_INIT){
      measuring= true;
      modeDataPoint = 0;
      dataPointsRequired = nDatapoints;
      currentMode = SHORT;
      return;
      }      
    if(currentMode==SHORT && modeDataPoint == nDatapoints){
      //do statistics
      float median[1]={0};
      float varianz[1]={0};
      doSomeStatistics(nDatapoints,median,varianz );
      checkLimits(median, varianz);
      //store data
      measuring=false;
      modeDataPoint = 0;
      saveData();
      currentMode=LONG_INIT;
      return;
      }
    if(currentMode==LONG_INIT){
      long currentTime = millis();
      Serial.println("");
      Serial.println("change position!");
      while((millis()-currentTime) <5000){
        Serial.print("- ");
        delay(500);
        }
        Serial.println("");
        Serial.println("");
      measuring= true;
      modeDataPoint = 0;
      dataPointsRequired = nDatapoints;
      currentMode = LONG;
      return;
      }
    if(currentMode==LONG && modeDataPoint == nDatapoints){
      //do statistics
      float median[1]={0};
      float varianz[1]={0};
      doSomeStatistics(nDatapoints,median,varianz );
      checkLimits(median, varianz);
      //store data
      saveData();
      measuring=false;
      modeDataPoint = 0;
      currentMode+=1;
      return;
      }
    if(currentMode==5){
      Serial.println("");
      
      if(deviceWorks[0] && deviceWorks[1]){
        Serial.println("TEST PASSED");
        }else{
        Serial.println("NOT PASSED  - Check protocol for detailed information");
      }
      currentMode+=1;
      return;
      } 
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(100); // Delay a second between loops.
} // End of loop


int countFiles() {
  root = SD.open("/distanz");
  root.rewindDirectory();
  int nFiles = 0;
  
  while (true) {
    File entry =  root.openNextFile();
    if (! entry) {
      // no more files
      
      break;
    }
    nFiles++;
    entry.close();
  }
  root.close();
  return nFiles;
}

String split(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void getSN(){
  root = SD.open("/distanz");
  root.rewindDirectory();       
  //search for file with address in name
  while(true){       
    File entry = root.openNextFile();
    if (!entry) {
          // no more files
          break;
        }
        
    char* filenameP;
    char filename[50];
    filenameP = (char*)entry.name();
    memcpy(&filename[0],filenameP,50);
    String filnameS = filename;
    String txtRemoved = split(filnameS,'.',0);
    String directoryRemoved = split(txtRemoved,'/',2);
    String SN = split(directoryRemoved,'_',0);
    String mac = split(directoryRemoved,'_',1);
          
    entry.close();
    if(Address == mac){
      Serial.println("");
      Serial.print("Data found on SD-Card for this Distance-Box: \nSN: ");
      Serial.print(SN);
      Serial.print(" MAC: ");
      Serial.println(mac);
      Serial.println("");
      SerialNumberString = SN;
      snOnSDCard = true;
      break;
      }  
  }
  root.close();

  if(!snOnSDCard){
    Serial.println("no data on sd card for this Box available... creating a file..");
    Serial.print("New SN: ");
    sprintf(buffer, "D%04d", storedDataFiles+1);
    SerialNumberString = buffer;  
    root = SD.open("/distanz/" +SerialNumberString+"_"+ Address + ".txt", FILE_WRITE);
    root.close();
    Serial.println(SerialNumberString);
    }
  //send SERIALNUMBER to device
  String myStringBuffer = SerialNumberString;
  myStringBuffer.remove(0,1);
  SERIALNUMBER[0] = myStringBuffer.toInt();
  
  uint8_t mySNArray[2];
  memcpy(&mySNArray[0],&SERIALNUMBER[0],2);
  pHWRemoteCharacteristic->writeValue(&mySNArray[0], 2);
    
}

bool doSomeStatistics(int dataPoints, float* median, float* varianz){
  
  for(int i=0;i<dataPoints;i++){
    median[0] += D[i]/dataPoints;
  }          

  for(int i=0;i<dataPoints;i++){
    varianz[0]+=(D[i]-median[0])*(D[i]-median[0])/(dataPoints-1);
  }
  
  Serial.println();
  Serial.printf("Median\t%f ± %f (%f ± %f)\n",median[0],sqrt(varianz[0])/sqrt(dataPoints),target[currentMode],limitArray[currentMode]);
  Serial.printf("standard deviation %f (%f)\n",sqrt(varianz[0]),limitSTD);

  return 0;
}

void checkLimits(float* median, float* varianz){
  float myLimit = limitArray[currentMode];
  float mytarget = target[currentMode];

  if(sqrt(varianz[0])<limitSTD && (median[0]<mytarget+myLimit) && (median[0]>mytarget-myLimit)){
    Serial.print("Distance measurement looks good");
    if(currentMode==SHORT){
        deviceWorks[0]=true;
      }else if(currentMode==LONG){
        deviceWorks[1]=true;  
      }
      
  }else{
     Serial.print("problem with distance measurement");
     if(currentMode==SHORT){
      deviceWorks[0]=false;
      }else if(currentMode==LONG){
        deviceWorks[1]=false;  
      }
  }     
    Serial.println("");
}

void saveData(){
  root = SD.open("/distanz/"  +SerialNumberString+"_"+ Address +  ".txt", FILE_APPEND);
  int saveLength =nDatapoints;
  if(currentMode == SHORT){
      root.println("SHORT MODE");
  }else if(currentMode == LONG){      
      root.println("LONG MODE");
  }
  for(int i = 0;i<saveLength;i++){
    root.print(D[i],5);
    root.print('\t');
    root.println(T[i],5);
  }
  root.close(); 
}