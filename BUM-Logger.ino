#include <SPI.h>
#include <SD.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <IniFile.h>
#include <LinkedList.h>

#include <ArduinoLowPower.h>
#include <RTCZero.h>

#define MAX_VALUE_LEN 64
#define MAX_URL_LEN 256

struct SensorConfig {
  char label[MAX_VALUE_LEN];
  char address[MAX_VALUE_LEN];
  float zeroPoint;
  float boilPoint;
};

struct StationConfig {
  char name[MAX_VALUE_LEN];
  uint32_t sensorCount;
  uint32_t interval;
  LinkedList<SensorConfig *> sensors;

  bool remoteLogging;
  char loggingUrl[MAX_URL_LEN];
  char gprsApn[MAX_VALUE_LEN];
  char gprsUser[MAX_VALUE_LEN];
  char gprsPass[MAX_VALUE_LEN];
};

StationConfig config;

const char *CONFIG_FILE = "station.cfg";

// SD card settings
const int chipSelect = SDCARD_SS_PIN;

const int configPin   = A0;
const int debugPin    = A1;
const int battery2Pin = A2;

int configPinValue  = 0;
int debugPinValue   = 0;

// Variables used in logger and configuration
char timestamp[20];
DeviceAddress addr;

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);  
DallasTemperature sensors(&oneWire);

// Modem Serial
HardwareSerial &serialGSM = Serial1;

RTCZero rtc;

void setup(void) {
  debugPinValue = analogRead(debugPin);
  configPinValue = analogRead(configPin);

  USBDevice.attach();
  Serial.begin(115200);
  if(debugPinValue > 900 || configPinValue > 900) {
    while(!Serial) {}
  }

  // Set GSM module baud rate
  serialGSM.begin(115200);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card initialization failed.  Is a card inserted?");
    while (1);
  }

  // Start real time clock
  rtc.begin();

  // Start temperature sensors
  sensors.begin();

  //Read configuration
  readConfig();
  
  if(configPinValue > 900) configuration_setup();
  else logger_setup();
}

void loop() {
  debugPinValue = analogRead(debugPin);
  if(configPinValue > 900) configuration_loop();
  else logger_loop();
}

void updateTimestamp() {
  sprintf(timestamp, "%04i-%02i-%02iT%02i:%02i:%02i", 2000 + rtc.getYear(), rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
}

void addressToString(DeviceAddress addr, char *out) {
  sprintf(out, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]); 
}

void stringToAddress(char *str, DeviceAddress dst) {
  uint32_t addr[8];
  sscanf(str, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5], &addr[6], &addr[7]);
  for(int i = 0; i < 8; i++) dst[i] = addr[i];
}

void readConfig() {
  const size_t bufferLen = MAX_VALUE_LEN;
  char buffer[bufferLen];

  if(!SD.exists(CONFIG_FILE)) return;
  
  // Load config file
  IniFile ini(CONFIG_FILE);
  if (!ini.open()) {
    Serial.print("Error opening ");
    Serial.print(CONFIG_FILE);
    Serial.print(": ");
    Serial.println(ini.getError());
    return;
  }
    
  if (!ini.validate(buffer, bufferLen)) {
    Serial.println("Buffer length invalid.");
    return;
  }

  // Load station name
  if(ini.getValue("station", "name", buffer, bufferLen)) {
    strncpy(config.name, buffer, MAX_VALUE_LEN);
  }

  // Load sensor count and interval
  ini.getValue("station", "interval", buffer, bufferLen, config.interval);
  ini.getValue("station", "sensors", buffer, bufferLen, config.sensorCount);

  for(int i = 0; i < config.sensorCount; i++) {
    char key[32];
    SensorConfig *sensor = new struct SensorConfig;
    sensor->zeroPoint = 0.0f;
    sensor->boilPoint = 100.0f;

    sprintf(key, "sensor%d", i + 1);
    if(ini.getValue("sensors", key, sensor->address, MAX_VALUE_LEN)) {
      if(ini.getValue(sensor->address, "label", buffer, bufferLen)) {
        strncpy(sensor->label, buffer, MAX_VALUE_LEN);
      }

      ini.getValue(sensor->address, "zeropoint", buffer, bufferLen, sensor->zeroPoint);
      ini.getValue(sensor->address, "boilpoint", buffer, bufferLen, sensor->boilPoint);
    }

    config.sensors.add(sensor);
  }

  // Remote logging configuration
  ini.getValue("station", "remote-logging", buffer, bufferLen, config.remoteLogging);
  ini.getValue("remote", "url", config.loggingUrl, MAX_URL_LEN);
  ini.getValue("remote", "gprs-apn", config.gprsApn, MAX_VALUE_LEN);
  ini.getValue("remote", "gprs-user", config.gprsUser, MAX_VALUE_LEN);
  ini.getValue("remote", "gprs-pass", config.gprsPass, MAX_VALUE_LEN);
}

void dumpConfig(Stream &out) {
  out.println(F("[station]"));
  out.print(F("name="));
  out.println(config.name);
  out.print(F("interval="));
  out.println(config.interval);
  out.print(F("sensors="));
  out.println(config.sensors.size());
  out.print(F("remote-logging="));
  out.println(config.remoteLogging);

  out.println(F("\n[remote]"));
  out.print(F("url="));
  out.println(config.loggingUrl);
  out.print(F("gprs-apn="));
  out.println(config.gprsApn);
  out.print(F("gprs-user="));
  out.println(config.gprsUser);
  out.print(F("gprs-pass="));
  out.println(config.gprsPass);

  out.println(F("\n[sensors]"));
  for(int i = 0; i < config.sensors.size(); i++) {
    SensorConfig *s = config.sensors.get(i);
    out.print(F("sensor"));
    out.print(i + 1);
    out.print(F("="));
    out.println(s->address);
  }

  // Have to loop twice to output things in the right order
  for(int i = 0; i < config.sensors.size(); i++) {
    SensorConfig *s = config.sensors.get(i);

    out.print(F("\n["));
    out.print(s->address);
    out.println(F("]"));
    out.print("label=");
    out.println(s->label);
    out.print("zeropoint=");
    out.println(s->zeroPoint);
    out.print("boilpoint=");
    out.println(s->boilPoint);
  }
}

int saveConfig() {
  File configFile = SD.open(CONFIG_FILE, FILE_WRITE | O_TRUNC);
  if (configFile) {
    dumpConfig(configFile);
    configFile.close();
  } else {
    return 0;
  }

  return 1;
}

int findIndexByAddress(char *address) {
  int size = config.sensors.size();
  for(int i = 0; i < size; i++) {
    SensorConfig *s = config.sensors.get(i);
    if(s != NULL && strcmp(s->address, address) == 0) {
      return i;
    }
  }

  return -1;
}

SensorConfig *findSensorByAddress(char *address) {
  int size = config.sensors.size();
  for(int i = 0; i < size; i++) {
    SensorConfig *s = config.sensors.get(i);
    if(s != NULL && strcmp(s->address, address) == 0) {
      return s;
    }
  }

  return NULL;
}
