#include <Arduino_LSM6DS3.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HTU21DF.h>
#include <WiFiNINA.h>
#include "config.h"

Adafruit_HTU21DF htu21df = Adafruit_HTU21DF(); // I2C
const char HOST_NAME[] = THEHOST;
const char ssid[] = WIFI_SSID;
const char psk[] = WIFI_PWD;

const char PATH_NAME[] = "/temperature";
const char getURL[] = "/temperature?loc=SRi/keh&lb=1&ub=10000";
const char queryTemplate[] = "?y=%ld&loc=%s&s=%c";

// constants for calculating the sensor values
const float LAM = 0.05;
const int lenWindow = 30;
const int plusThreshold = 30;
const int minusThreshold = -30;

// interval constants
const int intervalInfo = 500; // interval at which to update the board information
const int reconnectInterval = 5000;

// switches controlling program flow
boolean executePost = false;
boolean outputMemory = false;
boolean outputVerbose = false;

//flag for first run
boolean firstRun = true;

// transient states and counter
int status = WL_IDLE_STATUS; // the Wi-Fi radio's status
int ledState = LOW;          // ledState used to set the LED
int cntElem = 0;

// various timers
unsigned long previousMillisInfo = 0;      // will store last time Wi-Fi information was updated
unsigned long previousMillisLED = 0;       // will store the last time LED was updated
unsigned long previousMillisReconnect = 0; // will store the last time that HTTP-connection was reconnected;

// unsigned long previousMillisTestGet = 0;
typedef struct
{
    float temperature = 0;
    float humidity = 0;
    float sqTemperature = 0;
    float sqHumidiy = 0;

    float corrTemperature = 25;
    float corrHumidity = 50;
} SensorData;

SensorData currentSensorContainer;
SensorData transientSensorContainer;

//for memory measurement
extern "C" char* sbrk(int incr);

//function prototypes
void setup();
void initializeTemp();
int initializeWiFi(int statIn);
const char* translateWifiState(int state);
void println(const char* str);
void print(const char* str);
void println(double v);
void print(double v);
int freeRam();
void display_freeram();

void setup()
{
    // put your setup code here, to run once:
    // Initialize serial and wait for port to open:
    Serial1.begin(9600);
    Serial.begin(9600);
    delay(1000);
    // while (!Serial); 
    // while(!Serial1);

    // set the LED as output
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    // attempt to connect to Wi-Fi network:
    print("Attempting to connect to network: ");
    print(ssid);
    print(" ");

    status = initializeWiFi(status);

    println("\0");
    char msg[255];
    // you're connected now, so print out the data:
    snprintf(msg, 255, "You're connected to the network\nWifi - Firmware: %s latest: %s", WiFi.firmwareVersion(), WIFI_FIRMWARE_LATEST_VERSION);
    println(msg);
    println("---------------------------------------");

    if (!htu21df.begin())
    {
        println("Failed to initialize Temperature htu21df!");
        for (int i = 0; i < 10; i++)
        {
            delay(500);
            if (htu21df.begin())
            {
                print("finally succeeded after ");
                print(i);
                println(" tries!");
                i = 100;
            }
            else
                println("try again");
        }
    }
    initializeTemp();
}

void initializeTemp()
{
    //initialize the currentSensorContainer
    // currentSensorContainer.temperature = (htu21df.readTemperature() - currentSensorContainer.corrTemperature);
    // currentSensorContainer.humidity = (htu21df.readHumidity() - currentSensorContainer.corrHumidity);
    currentSensorContainer.temperature = 0;
    currentSensorContainer.humidity = 0;
    currentSensorContainer.sqTemperature = currentSensorContainer.temperature * currentSensorContainer.temperature;
    currentSensorContainer.sqHumidiy = currentSensorContainer.humidity * currentSensorContainer.humidity;
    //copy the transientSensorContainer by a deep copy
    memcpy(&transientSensorContainer, &currentSensorContainer, sizeof(SensorData));
}

void  println(const char* str) {
    Serial.println(str);
    Serial1.println(str);
}

void  print(const char* str) {
    Serial.print(str);
    Serial1.print(str);
}


void  println(double v) {
    Serial.print(v);
    Serial1.print(v);
}

void print(double v) {
    Serial.print(v);
    Serial1.print(v);
}

int initializeWiFi(int statIn)
{
    int stat = statIn;
    while (stat != WL_CONNECTED)
    {
        print("$");
        // Connect to WPA/WPA2 network:
        stat = WiFi.begin(ssid, psk);

        if (stat != WL_CONNECTED)
        {
            print("Reason code: ");
            print(WiFi.reasonCode());
            print(" ");
            println(translateWifiState(stat));
        }
        // wait 2 seconds for connection:
        delay(2000);
    }
    return stat;
}

const char* translateWifiState(int state)
{
    switch (state)
    {
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

void outputPressTempSensors()
{
    float tAvg = getActualTemperatureAvg();
    float hAvg = getActualHumidityAvg();
    print(" --- T2: ");
    print(htu21df.readTemperature());
    print("(");
    print(tAvg);
    print("=");
    print(celsiusTomilliKelvin(tAvg));
    print("mK) ---  Humi: ");
    print(htu21df.readHumidity());
    print("%(");
    print(hAvg);
    println(")");
}

void loop()
{

    unsigned long currentMillisInfo = millis();

    // check if the time after the last update is bigger the interval
    if (currentMillisInfo - previousMillisInfo >= intervalInfo)
    {
        previousMillisInfo = currentMillisInfo;
        // switchLed();
        if (outputVerbose)
        {
            outputPressTempSensors();
        }
        updateSensorValues(htu21df.readTemperature(), htu21df.readHumidity());
        postValuesToServer(getActualTemperatureAvg(), getActualHumidityAvg(), "test");
    }

    if (Serial.available())
    {
        char controlMsg = Serial.read();
        if (controlMsg == 'v')
        {
            outputVerbose = !outputVerbose;
        }
        else if (controlMsg == 'm') {
            outputMemory = !outputMemory;
        }
    }





    if ((currentMillisInfo - previousMillisReconnect) > reconnectInterval)
    {
        previousMillisReconnect = currentMillisInfo;
        // if the server's disconnected, stop the client:

        int wifiState = WiFi.status();
        if (wifiState != WL_CONNECTED)
        {
            print("wifi state: ");
            println(translateWifiState(wifiState));
            println("resetting wifi due to connection loss.");
            status = initializeWiFi(wifiState);
        }

        if (outputMemory) {
            display_freeram();
        }
    }
}

void updateSensorValues(float currentTemp, float currentHumi)
{
    // float tempMeas = currentTemp - currentSensorContainer.corrTemperature;
    // float humMeas = currentHumi - currentSensorContainer.corrHumidity;
    cntElem++;
    // int transElem=(cntElem+lenWindow/2);
    int transElem = cntElem;
    int curElem = (cntElem + lenWindow / 2);
    if (firstRun) {
        curElem = cntElem;
    }

    float delta = currentTemp - currentSensorContainer.temperature;
    currentSensorContainer.temperature += delta / curElem;
    //now using updated temperature
    float delta2 = currentTemp - currentSensorContainer.temperature;
    currentSensorContainer.sqTemperature += delta * delta2;

    delta = currentTemp - transientSensorContainer.temperature;
    transientSensorContainer.temperature += delta / transElem;
    delta2 = currentTemp - transientSensorContainer.temperature;
    transientSensorContainer.sqTemperature += delta * delta2;

    delta = currentHumi - currentSensorContainer.humidity;
    currentSensorContainer.humidity += delta / curElem;
    //now using updated temperature
    delta2 = currentHumi - currentSensorContainer.humidity;
    currentSensorContainer.sqHumidiy += delta * delta2;

    delta = currentHumi - transientSensorContainer.humidity;
    transientSensorContainer.humidity += delta / transElem;
    delta2 = currentHumi - transientSensorContainer.humidity;
    transientSensorContainer.sqHumidiy += delta * delta2;


    // currentSensorContainer.humidity += humMeas;
    // transientSensorContainer.humidity += humMeas;
    // currentSensorContainer.temperature += tempMeas;
    // transientSensorContainer.temperature += tempMeas;

    // currentSensorContainer.sqHumidiy += humMeas * humMeas;
    // transientSensorContainer.sqHumidiy += humMeas * humMeas;
    // currentSensorContainer.sqTemperature += tempMeas * tempMeas;
    // transientSensorContainer.sqTemperature += tempMeas * tempMeas;

    if (cntElem == lenWindow / 2 && !firstRun)
    {
        switchTransientToCurrent(currentTemp, currentHumi);
    }
    else if (cntElem == lenWindow)
    {
        firstRun = false;
        executePost = true;
        cntElem = 0;
        switchTransientToCurrent(currentTemp, currentHumi);
        printAccumulatedTempInfo();
    }
}

void switchTransientToCurrent(float tempMeas, float humMeas) {
    currentSensorContainer.temperature = transientSensorContainer.temperature;
    currentSensorContainer.humidity = transientSensorContainer.humidity;
    transientSensorContainer.temperature = tempMeas;
    transientSensorContainer.humidity = humMeas;
    currentSensorContainer.sqTemperature = transientSensorContainer.sqTemperature;
    currentSensorContainer.sqHumidiy = transientSensorContainer.sqHumidiy;
    transientSensorContainer.sqTemperature = 0;//tempMeas * tempMeas;
    transientSensorContainer.sqHumidiy = 0;//humMeas * humMeas;
}

float getActualTemperatureAvg() {
    // return currentSensorContainer.corrTemperature + currentSensorContainer.temperature / lenWindow;
    return currentSensorContainer.temperature;
}

float getActualHumidityAvg() {
    // return currentSensorContainer.corrHumidity + currentSensorContainer.humidity / lenWindow;
    return currentSensorContainer.humidity;
}

float getActualTemperatureStdev() {
    // return sqrt((currentSensorContainer.sqTemperature - currentSensorContainer.temperature * currentSensorContainer.temperature / lenWindow) / (lenWindow - 1));
    return sqrt(currentSensorContainer.sqTemperature / (lenWindow - 1));
}

float getActualHumidityStdev() {
    // return sqrt((currentSensorContainer.sqHumidiy - currentSensorContainer.humidity * currentSensorContainer.humidity / lenWindow) / (lenWindow - 1));
    return sqrt(currentSensorContainer.sqHumidiy / (lenWindow - 1));
}
void printAccumulatedTempInfo() {
    print("T: ");
    print(getActualTemperatureAvg());
    print(" +- ");
    print(getActualTemperatureStdev());

    print(" H: ");
    print(getActualHumidityAvg());
    print(" +- ");
    println(getActualHumidityStdev());

    println("------------------");
}

int celsiusTomilliKelvin(float T)
{
    // round to nearest milliKelvin (by addign 0.5)
    return 0.5 + 1000 * (T + 273.15);
}

void postValuesToServer(float T, float Hum, const char* location)
{
    if (executePost)
    {
        // String str = fillQuery("2022-01-19%2011:30:00", T, location, 'g');
        char query[(255 + strlen(queryTemplate))];
        fillQuery(query, celsiusTomilliKelvin(T), location, 'g');

        char req[(255 + strlen(queryTemplate))];
        sprintf(req, "POST %s%s HTTP/1.1", PATH_NAME, query);
        println(req);
        WiFiSSLClient client;
        if (client.connectSSL(HOST_NAME, 443))
        {
            // if connected:
            client.println(req);
            client.println("Host: " + String(HOST_NAME));
            client.print("Authorization: Basic ");
            client.println(BASICAUTH);
            client.println("Connection: close");
            client.println();


            delay(500);
            boolean addNewLine = outputVerbose && client.available();
            while (client.available())
            {
                char c = client.read();
                if (outputVerbose)
                    Serial.write(c);
            }
            if (addNewLine) {
                Serial.write('\n');
            }
        }
        else
        { // if not connected:
            char msg[255];
            snprintf(msg, 255, "connection to 'https://%s' failed", HOST_NAME);
            println(msg);
            //while (1)
            //  ;
        }
        client.stop();
        executePost = false;
    }
}

void switchLed()
{
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
    {
        ledState = HIGH;
    }
    else
    {
        ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(LED_BUILTIN, ledState);
}

void fillQuery(char* queryBuffer, long tempMilli, const char loc[], char state)
{
    sprintf(queryBuffer, queryTemplate, tempMilli, loc, state);
    trim(queryBuffer);
}

//ChatGPT :)
void trim(char* str)
{
    if (str == NULL) return;

    char* start = str;
    char* end = str + strlen(str) - 1;

    // Find the first non-whitespace character
    while (isspace(*start)) start++;

    // Find the last non-whitespace character
    while (isspace(*end) && end >= start) end--;

    // Shift characters to the left to remove leading whitespace
    int shift = start - str;
    if (shift > 0) {
        memmove(str, start, end - start + 2);  // Include the null terminator
    }
    else {
        shift = 0;
    }

    // Null-terminate the new string
    *(str + (end - start + 1 - shift)) = '\0';
}

void display_freeram() {
    Serial.print(F("- SRAM left: "));
    Serial.println(freeRam());
}

int freeRam() {
    char top;
    return &top - reinterpret_cast<char*>(sbrk(0));
}
