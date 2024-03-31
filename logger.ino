const char *DATA_FILE = "data.log";

#define LUX_ADDRESS 0x23

void logger_setup() {
}

void logger_loop(void) {
  // Default to 5 minutes in case SD card is not inserted.
  uint16_t cfgInterval = 5;

  // Buffer for reading config file
  const size_t bufferLen = 64;
  char buffer[bufferLen];
  uint16_t cfgSensorCount = 0;

  digitalWrite(LED_BUILTIN, HIGH);

  // Get timestamp immediately
  updateTimestamp();

  //if(config.remoteLogging) turnModemOn();

  String dataString = "";

  // Do we need to attach a header?
  bool needHeader = !SD.exists(DATA_FILE);
  if(needHeader) {
    dataString += "Time\tBattery 1\tBattery 2\tLux";
    for(int i = 0; i < config.sensorCount; i++) {
      SensorConfig *sensor = config.sensors.get(i);
      dataString += "\t";
      dataString += sensor->label;
    }
    dataString += "\n";
  }

  dataString += timestamp;

  // Read values into an array in case we log remote
  // We need enough room for sensors plus battery
  float values[config.sensorCount + 1];

  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 4.3V):
  int sensorValue = analogRead(ADC_BATTERY);
  values[0] = sensorValue * (4.3 / 1023.0);
  dataString += "\t";
  dataString += String(values[0]);

  sensorValue = analogRead(battery2Pin);
  values[1] = sensorValue * (4.3 / 1023.0);
  dataString += "\t";
  dataString += String(values[1]);

  values[2] = readLuxSensor();
  dataString += "\t";
  dataString += String(values[2]);

  // Read sensors
  sensors.requestTemperatures();
  for (int i = 0; i < config.sensorCount; i++) {
    SensorConfig *sensor = config.sensors.get(i);

    stringToAddress(sensor->address, addr);
    float t = sensors.getTempF(addr);

    // Scale values based on calibration points
    t = ((t - sensor->zeroPoint) / (sensor->boilPoint - sensor->zeroPoint)) * 100.0f;
    values[i + 3] = t;

    dataString += "\t";
    dataString += String(t, 4);
  }

  writeData(dataString);
  if(config.remoteLogging) {
    sendDataToRemote(values);
    //sendData(values);
    //turnModemOff();
  }


loop:
  digitalWrite(LED_BUILTIN, LOW);
  
  time_t time = rtc.getEpoch();
  uint32_t d = ((cfgInterval * 60) - (time % (cfgInterval * 60))) * 1000;

  if(debugPinValue > 900) {
    Serial.println(dataString);
    Serial.flush();
    delay(d);
  } else {
    // Set all D pins to pullup to save power
    // Don't change pins 11, 12, 13 or 14
    for (int i=0; i < 11; i++) pinMode(i, INPUT_PULLUP);
    pinMode(15, INPUT_PULLUP);
    USBDevice.detach();
    //USBDevice.standby();
    LowPower.deepSleep(d);
  }
}

void writeData(String dataString) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File logFile = SD.open(DATA_FILE, FILE_WRITE);
  if (logFile) {
    logFile.println(dataString);
    logFile.close();
  } else {
    Serial.println("Error opening data log file.");
  }
}

float readLuxSensor() {
  uint8_t reg = 0x10;
  uint8_t buf[4] = {0};
  uint16_t data;

  Wire.beginTransmission(LUX_ADDRESS);
  Wire.write(&reg, 1);
  if (Wire.endTransmission() != 0) {
    return -1.0f;
  }
  delay(20);

  Wire.requestFrom(LUX_ADDRESS, (uint8_t)2);
  for (uint16_t i = 0; i < 2; i++) {
    buf[i] = Wire.read();
  }
  
  data = buf[0] << 8 | buf[1];
  return (((float)data) / 1.2f);
}
