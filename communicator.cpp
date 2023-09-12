#include "communicator.h"
#include "config.h"
#include "util.h"

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
  //we are connected to WiFi  now connect to mqtt
  //connectToMqtt();

  char id[255];
  char topic[255];

  snprintf(id, 255, "mqtt%s%ld", LOCATION, currentMillis);
  snprintf(topic, 255, "home/%s/will", LOCATION);
  

  mqtt.beginWill(topic, true, 1);
  mqtt.print("conn lost");
  mqtt.endWill();
  mqtt.setId(id);
  mqtt.setUsernamePassword(MQTT_USR, MQTT_PWD);
  if (!mqtt.connect(MQTT_HOST)) {
    tempUtil::print("mqtt connection failed with error code: ");
    tempUtil::println(mqtt.connectError());
    while (1)
      ;
  }

  return stat;
}

void Communicator::keepAlive() {
  mqtt.poll();
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

// void Communicator::fillQuery(char* queryBuffer, long tempMilli, long humiPPM, const char loc[], char state) {
//   sprintf(queryBuffer, Communicator::queryTemplate, tempMilli, humiPPM, loc, state);
//   tempUtil::trim(queryBuffer);
// }

char* Communicator::postValuesToServer(float T, float Hum) {
  // String str = fillQuery("2022-01-19%2011:30:00", T, location, 'g');
  // char queryTemperature[(255 + strlen(queryTemplate))];
  // fillQuery(queryTemperature, tempUtil::celsiusTomilliKelvin(T), tempUtil::percentToPPM(Hum), LOCATION, 'g');

  char req[255];

  int qos = 1;
  boolean retain = false;
  boolean dup = false;

  float dew = -1;
  char outputBuffer[255];

  snprintf(req, 255, "home/%s/meas", LOCATION);
  snprintf(outputBuffer, 255, "{'HTU21': { 'temperature': %e, 'humidity': %e, 'dewpoint': %e } }", T, Hum, dew);
  mqtt.beginMessage(req, strlen(outputBuffer), retain, qos, dup);
  mqtt.print(outputBuffer);
  mqtt.endMessage();


  tempUtil::print(req);
  tempUtil::print(":");
  tempUtil::println(outputBuffer);
  return nullptr;
}

const char* Communicator::getSSID() {
  return Communicator::ssid;
}

int Communicator::getWifiState() {
  return WiFi.status();
}
