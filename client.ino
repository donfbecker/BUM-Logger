// Modem type
#define TINY_GSM_MODEM_BG96
#include <TinyGsmClient.h>

#include <ArduinoJson.h>

// Modem HW Pins
#define MODEM_PWR_ON_PIN    20
#define MODEM_ON_PIN        6

// Initialize GSM modem
TinyGsm modem(serialGSM);

// Initialize GSM client
TinyGsmClient client(modem);

#define URL_PROTOCOL_LEN 8
#define URL_HOST_LEN 128
#define URL_PATH_LEN 128
struct URL {
  char protocol[URL_PROTOCOL_LEN];
  char host[URL_HOST_LEN];
  int port;
  char path[URL_PATH_LEN];
} url;

void sendDataToRemote(float *values) {
  turnModemOn();
  sendData(values);
  turnModemOff();
}

void turnModemOn() {
  // Turn modem on
  pinMode(MODEM_PWR_ON_PIN, OUTPUT);
  pinMode(MODEM_ON_PIN, OUTPUT);
  digitalWrite(MODEM_PWR_ON_PIN, HIGH);
  digitalWrite(MODEM_ON_PIN, HIGH);

  delay(100);
  digitalWrite(MODEM_ON_PIN, LOW);
}

void turnModemOff() {
  if (client.connected()) client.stop(500);
  if (modem.isGprsConnected()) modem.gprsDisconnect();
  modem.poweroff();

   // turn modem off
  digitalWrite(MODEM_PWR_ON_PIN, LOW);
  digitalWrite(MODEM_ON_PIN, LOW);
}

void sendData(float *values) {
  // Initialize modem
  if (!modem.restart()) {
    Serial.println("Modem not found.");
    return;
  }

  // Conntect to GSM Network
   if (!modem.isNetworkConnected()) {
    if (!modem.waitForNetwork(10000)) {
      Serial.println("Could not connect to GSM network.");
      return;
    }
   }

   // Connect to GPRS
   if (!modem.isGprsConnected()) {
    if (!modem.gprsConnect(config.gprsApn, config.gprsUser, config.gprsPass)) {
      Serial.println("Could not connect to GPRS.");
      return;
    }
  }

  // Set time from GSM
  set_time_from_gsm();

  // Parse config url
  parseUrl(config.loggingUrl);

  // Make connection
  if (!client.connect(url.host, url.port)) {
    Serial.println("Connection to logging url failed.");
  }

  // Build request
  DynamicJsonDocument jsonbuf(1024);
  jsonbuf["Station"] = config.name;
  jsonbuf["Battery V"] = values[0];
  for(int i = 0; i < config.sensorCount; i++) {
    SensorConfig *sensor = config.sensors.get(i);
    jsonbuf[sensor->label] = values[i + 1];
  }

  String jsonstr = "";
  serializeJson(jsonbuf, jsonstr);
  int jsonlen = jsonstr.length();

  String reqstr = "POST /" + String(url.path) + " HTTP/1.1\r\nHost: " + String(url.host) + "\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: " + jsonlen + "\r\n\r\n" + jsonstr + "\r\n\r\n";
  client.print(reqstr);
} 

void set_time_from_gsm() {
  int gsm_year = 0;
  int gsm_month = 0;
  int gsm_day = 0;
  int gsm_hour = 0;
  int gsm_minute = 0;
  int gsm_second = 0;
  float gsm_timezone = 0;

  if (modem.getNetworkTime(&gsm_year, &gsm_month, &gsm_day, &gsm_hour, &gsm_minute, &gsm_second, &gsm_timezone)) {
    rtc.setDate(gsm_day, gsm_month, gsm_year - 2000);
    rtc.setTime(gsm_hour, gsm_minute, gsm_second);
  }
}

void parseUrl(char *str) {
  // Assume a valid URL
  enum URLParseState {ST_PROTOCOL, ST_SEPERATOR, ST_HOST, ST_PORT, ST_PATH} state = ST_PROTOCOL;

  memset(&url, 0, sizeof(url));
  url.port = 80;
  url.path[0] = '/';

  int pos = 0;
  for (int i = 0; str[i] != 0; i++) {
    switch(state) {
      case ST_PROTOCOL:
        if (str[i] == ':') {
          state = ST_SEPERATOR;
          pos = 0;
        }
        else url.protocol[pos++] = str[i];
        break;

      case ST_SEPERATOR:
        if (str[i] != '/') {
          state = ST_HOST;
          pos = 0;
          url.host[pos++] = str[i];
        }
        break;

      case ST_HOST:
        if (str[i] == ':') {
          url.port = atoi(&str[i + 1]);
          state = ST_PORT;
          pos = 0;
        } if (str[i] == '/') {
          state = ST_PATH;
          pos = 0;
        } else {
          url.host[pos++] = str[i];
        }
        break;

      case ST_PORT:
        if (str[i] == '/') {
          state = ST_PATH;
          pos = 0;
        }
        break;

      case ST_PATH:
        url.path[pos++] = str[i];
    }
  }
}