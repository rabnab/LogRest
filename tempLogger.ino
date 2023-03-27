#include <Arduino_LSM6DS3.h>
#include <Adafruit_Sensor.h>
// #include <Adafruit_BMP280.h>
//#include "Adafruit_Si7021.h"
#include <Adafruit_HTU21DF.h>
#include "config.h"

#include <WiFiNINA.h>


Adafruit_HTU21DF htu21df = Adafruit_HTU21DF();  // I2C

const float LAM = 0.05;

int plusThreshold = 30, minusThreshold = -30;

boolean outputVerboseTemp = false;

char ssid[] = WIFI_SSID;
char psk[] = WIFI_PWD;

WiFiClient client;

char HOST_NAME[] = THEHOST;
String PATH_NAME = "/temperature";  // change your EVENT-NAME and YOUR-KEY
const char getURL[] = "/temperature?loc=SRi/keh&lb=1&ub=10000";
const char queryTemplate[] = "?y=%ld&loc=%s&s=%c";

int status = WL_IDLE_STATUS;                // the Wi-Fi radio's status
int ledState = LOW;                         //ledState used to set the LED
unsigned long previousMillisInfo = 0;       //will store last time Wi-Fi information was updated
unsigned long previousMillisLED = 0;        // will store the last time LED was updated
unsigned long previousMillisReconnect = 0;  // will store the last time that HTTP-connection was reconnected;
const int intervalInfo = 500;               // interval at which to update the board information
const int lenWindow = 30;
// const int intervalTestGet = 5000;           // interval at which to send GET request
const int reconnectInterval = 5000;

// unsigned long previousMillisTestGet = 0;

float curTemp = 0;
float curHumi = 0;
float sqTemp = 0;
float sqHumi = 0;
float nextTemp = 0;
float nextHumi = 0;
float nextSqTemp = 0;
float nextSqHumi = 0;

float corrTemp = 25;
float corrHumi = 50;

int cnt = 0;
int cntElem = 0;
boolean executePost = false;

void setup() {
  // put your setup code here, to run once:
  //Initialize serial and wait for port to open:
  Serial1.begin(9600);

  Serial.begin(9600);
  delay(1000);
  //while (!Serial);
  //while(!Serial1);

  // set the LED as output
  pinMode(LED_BUILTIN, OUTPUT);
  // attempt to connect to Wi-Fi network:
  Serial.print("Attempting to connect to network: ");
  Serial.print(ssid);
  Serial.print(" ");

  Serial1.print("netzwerk verbunden: ");
  Serial1.print(ssid);
  Serial1.print(" ");





  while (status != WL_CONNECTED) {
    Serial.print(".");
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, psk);

    if (status != WL_CONNECTED) {
      Serial.print("Reason code: ");
      Serial.println(WiFi.reasonCode());
    }
    // wait 5 seconds for connection:
    delay(5000);
  }
  Serial.println();
  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");
  Serial1.println("You're connected to the network");

  Serial.println("Wifi - Firmware: " + String(WiFi.firmwareVersion()) + " latest: " + String(WIFI_FIRMWARE_LATEST_VERSION));

  Serial.println("---------------------------------------");

  if (client.connectSSL(HOST_NAME, 443)) {
    // if connected:
    Serial.println("HTTP SSL connection established to server");
  } else {  // if not connected:

    Serial.println("connection to 'https://" + String(HOST_NAME) + "' failed");
    while (1)
      ;
  }

  if (!htu21df.begin()) {
    Serial.println("Failed to initialize Temperature htu21df!");
    for (int i = 0; i < 10; i++) {
      delay(500);
      if (htu21df.begin()) {
        Serial.print("finally succes after ");
        Serial.print(i);
        Serial.println(" tries!");
        i = 100;
      } else
        Serial.println("try again");
    }
  }
  initializeTemp();
}

void initializeTemp() {
  nextTemp = curTemp = (htu21df.readTemperature() - corrTemp);
  nextHumi = curHumi = (htu21df.readHumidity() - corrHumi);
  nextSqTemp = sqTemp = curTemp * curTemp;
  nextSqHumi = sqHumi = curHumi * curHumi;
}


void outputPressTempSensors() {

  Serial.print(" --- T2: ");
  Serial.print(htu21df.readTemperature(), 2);
  Serial.print(" ---  Humi: ");
  Serial.print(htu21df.readHumidity(), 2);
  Serial.println("%");
}

void loop() {

  unsigned long currentMillisInfo = millis();

  // check if the time after the last update is bigger the interval
  if (currentMillisInfo - previousMillisInfo >= intervalInfo) {
    previousMillisInfo = currentMillisInfo;
    switchLed();
    if (outputVerboseTemp) {
      outputPressTempSensors();
    }
    updateSensorValues();
    postValuesToServer(corrTemp + curTemp / lenWindow, corrHumi + curHumi / lenWindow, "SZ");
  }

  if (Serial.available()) {
    char controlMsg = Serial.read();
    if (controlMsg == 'v') {
      outputVerboseTemp = !outputVerboseTemp;
    }
  }

  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  if ((currentMillisInfo - previousMillisReconnect) > reconnectInterval) {
    previousMillisReconnect = currentMillisInfo;
    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnecting from server.");
      client.flush();
      client.stop();

      // try to reconnect
      while (!reconnect()) {
        delay(2000);
        Serial.print(".");
      };
      Serial.println("connection reestablished");
    }
  }
}

void updateSensorValues() {
  float tempMeas = htu21df.readTemperature() - corrTemp;
  float humMeas = htu21df.readHumidity() - corrHumi;
  curHumi += humMeas;
  nextHumi += humMeas;
  curTemp += tempMeas;
  nextTemp += tempMeas;

  sqHumi += humMeas * humMeas;
  nextSqHumi += humMeas * humMeas;
  sqTemp += tempMeas * tempMeas;
  nextSqTemp += tempMeas * tempMeas;
  cntElem++;
  if (cntElem == lenWindow / 2) {
    curTemp = nextTemp;
    curHumi = nextHumi;
    nextTemp = tempMeas;
    nextHumi = humMeas;
    sqTemp = nextSqTemp;
    sqHumi = nextSqHumi;
    nextSqTemp = tempMeas * tempMeas;
    nextSqHumi = humMeas * humMeas;
  } else if (cntElem == lenWindow) {
    executePost = true;
    Serial.print("T: ");
    Serial.print(corrTemp + curTemp / lenWindow);
    Serial.print(" +- ");
    Serial.print(sqrt((sqTemp - curTemp * curTemp / lenWindow) / (lenWindow - 1)));

    Serial.print(" H: ");
    Serial.print(corrHumi + curHumi / lenWindow);
    Serial.print(" +- ");
    Serial.println(sqrt((sqHumi - curHumi * curHumi / lenWindow) / (lenWindow - 1)));


    Serial.println("------------------");

    cntElem = 1;
    curTemp = nextTemp;
    curHumi = nextHumi;
    nextTemp = tempMeas;
    nextHumi = tempMeas;
    sqTemp = nextSqTemp;
    sqHumi = nextSqHumi;
    nextSqTemp = tempMeas * tempMeas;
    nextSqHumi = humMeas * humMeas;
  }
}

int celsiusTomilliKelvin(float T) {
  //round to nearest milliKelvin (by addign 0.5)
  return 0.5+1000*(T+273.15);
}

void postValuesToServer(float T, float Hum, const char* location) {
  if (executePost) {
    // String str = fillQuery("2022-01-19%2011:30:00", T, location, 'g');
    
    
    String str = fillQuery(celsiusTomilliKelvin(T), location, 'g');
    String req = "POST " + PATH_NAME + str + " HTTP/1.1";
    Serial.println(req);
    client.println(req);
    closeRESTrequest();
    executePost = false;
  }
}

boolean reconnect() {
  return client.connectSSL(HOST_NAME, 443);
}

void closeRESTrequest() {
  client.println("Host: " + String(HOST_NAME));
  client.print("Authorization: Basic ");
  client.println(BASICAUTH);
  client.println("Connection: close");
  client.println();
}

void switchLed() {
  // if the LED is off turn it on and vice-versa:
  if (ledState == LOW) {
    ledState = HIGH;
  } else {
    ledState = LOW;
  }

  // set the LED with the ledState of the variable:
  digitalWrite(LED_BUILTIN, ledState);
}

String fillQuery(long tempMilli, const char loc[], char state) {
  char* retVal = new char[(255 + strlen(queryTemplate))];
  sprintf(retVal, queryTemplate, tempMilli, loc, state);
  String retStr = String(retVal);
  retStr.trim();
  return retStr;
}
