#pragma once
#include <WiFiNINA.h>
#include "util.h"

class Communicator {
private:
  const char* ENDPOINT_NAME = "/meas";
  //const char* getURL = "/temperature?loc=SRi/keh&lb=1&ub=10000";
  const char* queryTemplate = "?t=%ld&h=%ld&loc=%s&s=%c";
  long lastLowPowerMillis=0;

  void fillQuery(char* queryBuffer, long tempMilli, long humiPPM, const char loc[], char state);
public:
  static const char* HOST_NAME;
  static const char* ssid;
  static const char* psk;



  int initializeWiFi(int statIn, unsigned long currentMillis);

  const char* translateWifiState(int state);
  char* postValuesToServer(float T, float Hum);
  const char* getSSID();
  int getWifiState();

  Communicator(){};
};