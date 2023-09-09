#include "communicator.h"
#include "config.h"
#include "util.h"


const char* Communicator::HOST_NAME = THEHOST;
const char* Communicator::ssid = WIFI_SSID;
const char* Communicator::psk = WIFI_PWD;

int Communicator::initializeWiFi(int statIn, unsigned long currentMillis) {
  int stat = statIn;
  while (stat != WL_CONNECTED) {
    tempUtil::print("$");
    WiFi.end();
    // Connect to WPA/WPA2 network:
    stat = WiFi.begin(ssid, psk);
    if (stat != WL_CONNECTED) {
      tempUtil::print("Reason code: ");
      tempUtil::print(WiFi.reasonCode());
      tempUtil::print(" ");
      tempUtil::println(translateWifiState(stat));
      WiFi.noLowPowerMode();
      tempUtil::println("WifI in highPowerMode");
      this->lastLowPowerMillis = currentMillis;
    }

    unsigned long fiveMinutes = 300000;
    if ((currentMillis - (this->lastLowPowerMillis)) > fiveMinutes) {
      this->lastLowPowerMillis = currentMillis;
      WiFi.lowPowerMode();
      tempUtil::println("WifI in lowPowerMode");
    }
    // wait 0.2 seconds for connection:
    delay(200);
  }
  return stat;
}

const char* Communicator::translateWifiState(int state) {
  switch (state) {
    case WL_CONNECTED:
      return "connected";
    case WL_IDLE_STATUS:
      return "idle";
    case WL_CONNECTION_LOST:
      return "connection lost";
    case WL_CONNECT_FAILED:
      return "connection failed";
    case WL_DISCONNECTED:
      return "disconnected";
    default:
      return "unknown state";
  }
}

void Communicator::fillQuery(char* queryBuffer, long tempMilli, long humiPPM, const char loc[], char state) {
  sprintf(queryBuffer, Communicator::queryTemplate, tempMilli, humiPPM, loc, state);
  tempUtil::trim(queryBuffer);
}

char* Communicator::postValuesToServer(float T, float Hum) {
  // String str = fillQuery("2022-01-19%2011:30:00", T, location, 'g');
  char queryTemperature[(255 + strlen(queryTemplate))];
  fillQuery(queryTemperature, tempUtil::celsiusTomilliKelvin(T), tempUtil::percentToPPM(Hum), LOCATION, 'g');

  char req[(255 + strlen(queryTemplate))];
  sprintf(req, "POST %s%s HTTP/1.1", Communicator::ENDPOINT_NAME, queryTemperature);
  tempUtil::println(req);

  char* outputBuffer = new char[256];
  int outputLen = 0;
  WiFiSSLClient client;
  if (client.connectSSL(Communicator::HOST_NAME, 443)) {
    // if connected:

    client.println(req);
    client.println("Host: " + String(Communicator::HOST_NAME));
    client.print("Authorization: Basic ");
    client.println(BASICAUTH);
    client.println("Connection: close");
    client.println();


    delay(500);
    int availableBytes = client.available();
    boolean addNewLine = availableBytes;

    while (client.available()) {
      char c = client.read();
      if (outputLen < 255) {
        outputBuffer[outputLen++] = c;
      }
    }
    if (addNewLine) {
      outputBuffer[outputLen++] = ('\n');
    }

    outputBuffer[outputLen] = '\0';
  } else {  // if not connected:
    char msg[255];
    snprintf(msg, 255, "connection to 'https://%s' failed", Communicator::HOST_NAME);
    tempUtil::println(msg);
    //while (1)
    //  ;
  }
  client.stop();
  return (outputLen > 0) ? outputBuffer : nullptr;
}

const char* Communicator::getSSID() {
  return Communicator::ssid;
}

int Communicator::getWifiState() {
  return WiFi.status();
}
