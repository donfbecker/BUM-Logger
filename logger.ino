const char *DATA_FILE = "data.log";

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

  String dataString = "";

  // Do we need to attach a header?
  bool needHeader = !SD.exists(DATA_FILE);
  if(needHeader) {
    dataString += "Time\tBattery V";
    for(int i = 0; i < config.sensorCount; i++) {
      SensorConfig *sensor = config.sensors.get(i);
      dataString += "\t";
      dataString += sensor->label;
    }
    dataString += "\n";
  }

  dataString += timestamp;

  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 4.3V):
  int sensorValue = analogRead(ADC_BATTERY);
  float voltage = sensorValue * (4.3 / 1023.0);
  dataString += "\t";
  dataString += String(voltage);

  // Read sensors
  sensors.requestTemperatures();
  for (int i = 0; i < config.sensorCount; i++) {
    SensorConfig *sensor = config.sensors.get(i);

    stringToAddress(sensor->address, addr);
    float t = sensors.getTempF(addr);
    dataString += "\t";
    dataString += String(t, 4);
  }

  writeData(dataString);

loop:
  digitalWrite(LED_BUILTIN, LOW);
  
  time_t time = rtc.getEpoch();
  uint32_t d = ((cfgInterval * 60) - (time % (cfgInterval * 60))) * 1000;

  if(debugPinValue > 900) {
    Serial.println(dataString);
    Serial.flush();
    delay(d);
  } else {
   //USBDevice.detach();
   USBDevice.standby();
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
