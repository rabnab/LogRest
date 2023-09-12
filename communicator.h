#include "WiFiClient.h"
#pragma once
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include "util.h"

class Communicator {
private:

  long lastLowPowerMillis=0;
  WiFiClient wifiClient;
  MqttClient mqtt = new MqttClient(wifiClient);

public:
  static const char* ssid;
  static const char* psk;



  int initializeWiFi(int statIn, unsigned long currentMillis);
  const char* translateWifiState(int state);
  char* postValuesToServer(float T, float Hum);
  const char* getSSID();
  int getWifiState();
  void keepAlive();

  Communicator(){};
};