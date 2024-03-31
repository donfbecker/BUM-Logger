#include <SimpleSerialShell.h>

#define CHECK_ARGS(n, usage) { \
  if(argc < n) { \
    shell.println(usage); \
    return EXIT_FAILURE; \
  } \
}

#define SENSOR_FROM_ADDRESS(name, address) \
  SensorConfig *name = findSensorByAddress(address); \
  if(name == NULL) { \
    shell.print(address); \
    shell.println(F(" not found.")); \
    return EXIT_FAILURE; \
  }

#define INDEX_FROM_ADDRESS(name, address) \
  int name = findIndexByAddress(address); \
  if(name == -1) { \
    shell.print(address); \
    shell.println(F(" not found.")); \
    return EXIT_FAILURE; \
  }

#define COPY_ARGS(start, end, dest, len) \
  uint8_t pos = 0; \
  memset(dest, 0, len); \
  for(int i = start; i <= end; i++) { \
    if(i > start) dest[pos++] = ' '; \
    strncpy(&dest[pos], argv[i], len - pos); \
    pos += strlen(argv[i]); \
  }


int cmd_getTime(int argc, char **argv) {
  updateTimestamp();
  shell.println(timestamp);
  return EXIT_SUCCESS;
}

int cmd_getTimeSet(int argc, char **argv) {
  File timeFile = SD.open(F("TIME"), FILE_READ);
  if(timeFile) {
    while (timeFile.available()) {
      shell.write(timeFile.read());
    }
    timeFile.close();
  } else {
    shell.println(F(""));
  }
  return EXIT_SUCCESS;
};

int cmd_setTime(int argc, char **argv) {
  uint32_t year, month, day, hours, minutes, seconds;
  
  CHECK_ARGS(3, F("USAGE: settime [YYYY-MM-DD] [HH:MM:SS]"));

  sscanf(argv[1], "%04u-%02u-%02u", &year, &month, &day);
  sscanf(argv[2], "%02u:%02u:%02u", &hours, &minutes, &seconds);
  rtc.setDate(day, month, year - 2000);
  rtc.setTime(hours, minutes, seconds);

  updateTimestamp();

  File timeFile = SD.open(F("TIME"), FILE_WRITE | O_TRUNC);
  if (timeFile) {
    timeFile.println(timestamp);
    timeFile.close();
  } else {
    shell.println(F("Error writing time file."));
    return EXIT_FAILURE;
  }
  
  shell.print(F("OK "));
  shell.println(timestamp);
  return EXIT_SUCCESS;
};

int cmd_getName(int argc, char **argv) {
  shell.println(config.name);
  return EXIT_SUCCESS;
}

int cmd_setName(int argc, char **argv) {
  uint8_t pos = 0;
  memset(config.name, 0, MAX_VALUE_LEN);
  for(int i = 1; i < argc; i++) {
    if(i > 1) config.name[pos++] = ' ';
    strncpy(&config.name[pos], argv[i], MAX_VALUE_LEN - pos);
    pos += strlen(argv[i]);
  }

  shell.print(F("OK "));
  shell.println(config.name);
  return EXIT_SUCCESS;
}

int cmd_getInterval(int argc, char **argv) {
  shell.println(config.interval);
  return EXIT_SUCCESS;
}

int cmd_setInterval(int argc, char **argv) {
  CHECK_ARGS(2, F("USAGE: setinterval [MINUTES]"));

  config.interval = atoi(argv[1]);

  shell.print(F("OK "));
  shell.println(config.interval);
  return EXIT_SUCCESS;
}

int cmd_getSensors(int argc, char **argv) {
  for(int i = 0; i < config.sensors.size(); i++) {
    SensorConfig *sensor = config.sensors.get(i);
    if(sensor != NULL) {
      shell.print(i + 1);
      shell.print(F(": "));
      shell.print(sensor->address);
      shell.print(F(" "));
      shell.println(sensor->label);
    }
  }
  shell.println(F(""));
  return EXIT_SUCCESS;
}

int cmd_getLabel(int argc, char **argv) {
  CHECK_ARGS(2, F("USAGE: getlabel [SENSOR ADDRESS]"));
  SENSOR_FROM_ADDRESS(sensor, argv[1]);

  shell.println(sensor->label);
  return EXIT_SUCCESS;
}

int cmd_setLabel(int argc, char **argv) {
  CHECK_ARGS(3, F("USAGE: setlabel [SENSOR ADDRESS] [LABEL]"));
  SENSOR_FROM_ADDRESS(sensor, argv[1]);

  uint8_t pos = 0;
  memset(sensor->label, 0, MAX_VALUE_LEN);
  for(int i = 2; i < argc; i++) {
    if(i > 2) sensor->label[pos++] = ' ';
    strncpy(&sensor->label[pos], argv[i], MAX_VALUE_LEN - pos);
    pos += strlen(argv[i]);
  }

  shell.print(F("OK "));
  shell.println(sensor->label);
  return EXIT_SUCCESS;
}

int cmd_getZeroPoint(int argc, char **argv) {
  CHECK_ARGS(2, F("USAGE: getzeropoint [SENSOR ADDRESS]"));
  SENSOR_FROM_ADDRESS(sensor, argv[1]);

  shell.println(sensor->zeroPoint);
  return EXIT_SUCCESS;
}

int cmd_setZeroPoint(int argc, char **argv) {
  CHECK_ARGS(3, F("USAGE: setzeropoint [SENSOR ADDRESS] [TEMP C]"));
  SENSOR_FROM_ADDRESS(sensor, argv[1]);

  sensor->zeroPoint = atof(argv[2]);

  shell.print(F("OK "));
  shell.println(sensor->zeroPoint);
  return EXIT_SUCCESS;
}

int cmd_getBoilPoint(int argc, char **argv) {
  CHECK_ARGS(2, F("USAGE: getboilpoint [SENSOR ADDRESS]"));
  SENSOR_FROM_ADDRESS(sensor, argv[1]);

  shell.println(sensor->boilPoint);
  return EXIT_SUCCESS;
}

int cmd_setBoilPoint(int argc, char **argv) {
  CHECK_ARGS(3, F("USAGE: setboilpoint [SENSOR ADDRESS] [TEMP C]"));
  SENSOR_FROM_ADDRESS(sensor, argv[1]);

  sensor->boilPoint = atof(argv[2]);

  shell.print(F("OK "));
  shell.println(sensor->boilPoint);
  return EXIT_SUCCESS;
}

int cmd_calibrateZeroPoints(int argc, char **argv) {
  shell.println(F("Reading temperatures..."));
  sensors.requestTemperatures();
  for(int i = 0; i < config.sensors.size(); i++) {
    SensorConfig *sensor = config.sensors.get(i);
    stringToAddress(sensor->address, addr);
    sensor->zeroPoint = sensors.getTempC(addr);
  }

 shell.println(F("Zero point temperatures saved."));
  return EXIT_SUCCESS;
}

int cmd_calibrateBoilPoints(int argc, char **argv) {
  shell.println(F("Reading temperatures..."));
  sensors.requestTemperatures();
  for(int i = 0; i < config.sensors.size(); i++) {
    SensorConfig *sensor = config.sensors.get(i);
    stringToAddress(sensor->address, addr);
    sensor->boilPoint = sensors.getTempC(addr);
  }

  shell.println(F("Boil point temperatures saved."));
  return EXIT_SUCCESS;
}

int cmd_findSensors(int argc, char **argv) {
  char buffer[MAX_VALUE_LEN];
  int count = sensors.getDeviceCount();
  for(int i = 0; i < count; i++) {
    if(sensors.getAddress(addr, i)) {
      addressToString(addr, buffer);
      SensorConfig *sensor = findSensorByAddress(buffer);
      if(sensor == NULL) {
        shell.println(buffer);
      }
    }
  }
  return EXIT_SUCCESS;
}

int cmd_addSensor(int argc, char **argv) {
  CHECK_ARGS(3, F("USAGE: addsensor [SENSOR ADDRESS] [LABEL]"));

  SensorConfig *sensor = findSensorByAddress(argv[1]);
  if(sensor != NULL) {
    shell.print(F("Sensor with address "));
    shell.print(argv[1]);
    shell.println(F(" is already configured."));
    return EXIT_FAILURE;
  }

  sensor = new struct SensorConfig;
  strncpy(sensor->address, argv[1], MAX_VALUE_LEN);
  sensor->zeroPoint = 0.0f;
  sensor->boilPoint = 100.0f;

  uint8_t pos = 0;
  memset(sensor->label, 0, MAX_VALUE_LEN);
  for(int i = 2; i < argc; i++) {
    if(i > 2) sensor->label[pos++] = ' ';
    strncpy(&sensor->label[pos], argv[i], MAX_VALUE_LEN - pos);
    pos += strlen(argv[i]);
  }

  config.sensors.add(sensor);
  config.sensorCount = config.sensors.size();

  shell.print(sensor->address);
  shell.println(F(" added to configuration."));
  return EXIT_SUCCESS;
}

int cmd_removeSensor(int argc, char **argv) {
  CHECK_ARGS(2, F("USAGE: removesensor [SENSOR ADDRESS]"));
  INDEX_FROM_ADDRESS(index, argv[1]);

  config.sensors.remove(index);
  config.sensorCount = config.sensors.size();

  shell.print(argv[1]);
  shell.println(F(" removed from configuration."));
  return EXIT_SUCCESS;
}

int cmd_moveSensor(int argc, char **argv) {
  CHECK_ARGS(3, F("USAGE: movesensor [SENSOR ADDRESS] [POSITION]"));
  INDEX_FROM_ADDRESS(index, argv[1]);

  int pos = atoi(argv[2]) - 1;
  SensorConfig *sensor = config.sensors.remove(index);
  config.sensors.add(pos, sensor);

  shell.print(argv[1]);
  shell.print(F(" moved to position "));
  shell.print(argv[2]);
  shell.println(F("."));
  return EXIT_SUCCESS;
}

int cmd_getRemote(int argc, char **argv) {
  if(config.remoteLogging) {
    shell.println(F("Remote loggin is enabled."));
  } else {
    shell.println(F("Remote logging is disabled."));
  }
  return EXIT_SUCCESS;
}

int cmd_enableRemote(int argc, char **argv) {
  config.remoteLogging = true;
  shell.println(F("Remote logging enabled."));
  return EXIT_SUCCESS;
}

int cmd_disableRemote(int argc, char **argv) {
  config.remoteLogging = false;
  shell.println(F("Remote logging disabled."));
  return EXIT_SUCCESS;
}

int cmd_getGSMTimeout(int argc, char **argv) {
  shell.println(config.gsmTimeout);
  return EXIT_SUCCESS;
}

int cmd_setGSMTimeout(int argc, char **argv) {
  CHECK_ARGS(2, F("USAGE: setgsmtimeout [SECONDS]"));

  config.gsmTimeout = atoi(argv[1]);

  shell.print(F("OK "));
  shell.println(config.gsmTimeout);
  return EXIT_SUCCESS;
}

int cmd_getLoggingUrl(int argc, char **argv) {
  shell.println(config.loggingUrl);
  return EXIT_SUCCESS;
}

int cmd_setLoggingUrl(int argc, char **argv) {
  CHECK_ARGS(2, F("USAGE: setloggingurl [URL]"));
  COPY_ARGS(1, argc - 1, config.loggingUrl, MAX_URL_LEN);
  shell.println(config.loggingUrl);
  return EXIT_SUCCESS;
}

int cmd_getGprsApn(int argc, char **argv) {
  shell.println(config.gprsApn);
  return EXIT_SUCCESS;
}

int cmd_setGprsApn(int argc, char **argv) {
  CHECK_ARGS(2, F("USAGE: setgprsapn [GPRS APN]"));
  COPY_ARGS(1, argc - 1, config.gprsApn, MAX_VALUE_LEN);
  shell.println(config.gprsApn);
  return EXIT_SUCCESS;
}

int cmd_getGprsUser(int argc, char **argv) {
  shell.println(config.gprsUser);
  return EXIT_SUCCESS;
}

int cmd_setGprsUser(int argc, char **argv) {
  CHECK_ARGS(2, F("USAGE: setgprsuser [GPRS USERNAME]"));
  COPY_ARGS(1, argc - 1, config.gprsUser, MAX_VALUE_LEN);
  shell.println(config.gprsUser);
  return EXIT_SUCCESS;
}

int cmd_getGprsPass(int argc, char **argv) {
  shell.println(config.gprsPass);
  return EXIT_SUCCESS;
}

int cmd_setGprsPass(int argc, char **argv) {
  CHECK_ARGS(2, F("USAGE: setgprspass [GPRS PASSWORD]"));
  COPY_ARGS(1, argc - 1, config.gprsPass, MAX_VALUE_LEN);
  shell.println(config.gprsPass);
  return EXIT_SUCCESS;
}

int cmd_config(int argc, char **argv) {
  shell.println(F("\n--------------------\n"));
  dumpConfig(shell);
  shell.println(F("\n--------------------\n"));
  return EXIT_SUCCESS;
}

int cmd_save(int argc, char **argv) {
  if(!saveConfig()) {
    shell.println(F("Could not save configuration."));
  } else {
    shell.println(F("Configuration saved."));
  }
  return EXIT_SUCCESS;
}

void configuration_setup() {
  shell.attach(Serial);
  shell.addCommand(F("gettime \n\tReturns current time according to the RTC."), cmd_getTime);
  shell.addCommand(F("settime [YYYY-MM-DD] [HH:MM:SS]\n\tSets the RTC time."), cmd_setTime);
  shell.addCommand(F("gettimeset \n\tReturns the time when the RTC time was set."), cmd_getTimeSet);

  shell.addCommand(F("getname \n\tReturns the station name."), cmd_getName);
  shell.addCommand(F("setname [NAME]\n\tSets the station name."), cmd_setName);

  shell.addCommand(F("getinterval \n\tReturns the current logging interval in minutes."), cmd_getInterval);
  shell.addCommand(F("setinterval [MINUTES]\n\tSets the logging interval."), cmd_setInterval);

  shell.addCommand(F("getsensors \n\tList all currently configured sensors."), cmd_getSensors);

  shell.addCommand(F("getlabel [SENSOR ADDRESS]\n\tReturns the label for the sensor with the provided address."), cmd_getLabel);
  shell.addCommand(F("setlabel [SENSOR ADDRESS] [LABEL]\n\tSets the label for the sensor with the provided address."), cmd_setLabel);

  shell.addCommand(F("getzeropoint [SENSOR ADDRESS]\n\tReturns the configured zero point for the sensor with the provided address."), cmd_getZeroPoint);
  shell.addCommand(F("setzeropoint [SENSOR ADDRESS] [TEMP C]\n\tSets the zero point for the sensor with the provided address."), cmd_setZeroPoint);

  shell.addCommand(F("getboilpoint [SENSOR ADDRESS]\n\tReturns the configured boil point for the sensor with the provided address."), cmd_getBoilPoint);
  shell.addCommand(F("setboilpoint [SENSOR ADDRESS] [TEMP C]\n\tSets the boil point for the sensor with the provided address."), cmd_setBoilPoint);

  shell.addCommand(F("calibratezeropoints \n\tReads current temperature from all sensors and sets as zero point."), cmd_calibrateZeroPoints);
  shell.addCommand(F("calibrateboilpoints \n\tReads current temperature from all sensors and sets as boil point."), cmd_calibrateBoilPoints);

  shell.addCommand(F("findsensors \n\tList all connected sensors that are not configured."), cmd_findSensors);
  shell.addCommand(F("addsensor [SENSOR ADDRESS] [LABEL]\n\tAdd sensor with given address to configuration."), cmd_addSensor);
  shell.addCommand(F("removesensor [SENSOR ADDRESS]\n\tRemove sensor with given address from configuration."), cmd_removeSensor);
  shell.addCommand(F("movesensor [SENSOR ADDRESS] [POSITION]\n\tChange position of sensor with given address in configuration."), cmd_moveSensor);

  shell.addCommand(F("getremote \n\tReturns whether or not remote logging is enabled."), cmd_getRemote);
  shell.addCommand(F("enableremote \n\tEnable remote logging."), cmd_enableRemote);
  shell.addCommand(F("disableremote \n\tDisable remote logging."), cmd_disableRemote);

  shell.addCommand(F("getloggingurl \n\tReturns the remote logging url."), cmd_getLoggingUrl);
  shell.addCommand(F("setloggingurl [URL]\n\tSets the remote logging url."), cmd_setLoggingUrl);

  shell.addCommand(F("getgsmtimeout \n\tReturns the GSM timeout value."), cmd_getGSMTimeout);
  shell.addCommand(F("setgsmtimeout [SECONDS]\n\tSets the GSM timeout value in seconds."), cmd_setGSMTimeout);

  shell.addCommand(F("getgprsapn \n\tReturns the GPRS APN."), cmd_getGprsApn);
  shell.addCommand(F("setgprsapn [GPRS APN]\n\tSets GPRS APN."), cmd_setGprsApn);
  shell.addCommand(F("getgprsuser \n\tReturns the GPRS username."), cmd_getGprsUser);
  shell.addCommand(F("setgprsuser [GPRS USERNAME]\n\tSets GPRS username."), cmd_setGprsUser);
  shell.addCommand(F("getgprspass \n\tReturns the GPRS password."), cmd_getGprsPass);
  shell.addCommand(F("setgprspass [GPRS PASSWORD]\n\tSets GPRS password."), cmd_setGprsPass);

  shell.addCommand(F("config \n\tDump current configuration to shell."), cmd_config);
  shell.addCommand(F("save \n\tSaves the current configuration."), cmd_save);
  
  shell.println(F("\nBUM Logger Configuation Shell"));
  shell.println(F("Type 'help' for a list of commands"));
}

void configuration_loop() {
  if (Serial.available()) {
    char line[128];
    size_t lineLength = Serial.readBytesUntil('\n', line, 127);
    line[lineLength] = '\0';

    shell.execute(line);
  }
}
