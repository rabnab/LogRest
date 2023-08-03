#pragma once
#ifndef TEMPLOGGER_INO
#define TEMPLOGGER_INO

#include "tempLogger.h"
#include "sensorHandler.h"
#include "communicator.h"
#include "util.h"







// interval constants
const int intervalInfo = 500; // interval at which to update the board information
const int reconnectInterval = 5000;

// switches controlling program flow

boolean outputMemory = false;
boolean outputVerbose = false;


// transient states and counter
int status = WL_IDLE_STATUS; // the Wi-Fi radio's status
int ledState = LOW;          // ledState used to set the LED


// various timers
unsigned long previousMillisInfo = 0;      // will store last time Wi-Fi information was updated
unsigned long previousMillisLED = 0;       // will store the last time LED was updated
unsigned long previousMillisReconnect = 0; // will store the last time that HTTP-connection was reconnected;

//for memory measurement
extern "C" char* sbrk(int incr);




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

	print(getSSID());
	print(" ");

	status = initializeWiFi(status);

	println("\0");
	char msg[255];
	// you're connected now, so print out the data:
	snprintf(msg, 255, "You're connected to the network\nWifi - Firmware: %s latest: %s", WiFi.firmwareVersion(), WIFI_FIRMWARE_LATEST_VERSION);
	println(msg);
	println("---------------------------------------");

	SensorHandlerInit();
	if (!startSensors())
	{
		println("Failed to initialize Temperature htu21df!");
		for (int i = 0; i < 10; i++)
		{
			delay(500);
			if (startSensors())
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
		if (updateSensorValues()) {
			char* serialOutput = postValuesToServer(
				getActualTemperatureAvg(),
				getActualHumidityAvg(),
				"test");
			if (serialOutput != nullptr) {
				if (outputVerbose) Serial.print(serialOutput);
				free(serialOutput);
			}
		}
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

		int wifiState = getWifiState();
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



void display_freeram() {
	Serial.print(F("- SRAM left: "));
	Serial.println(freeRam());
}

int freeRam() {
	char top = ' ';
	return &top - reinterpret_cast<char*>(sbrk(0));
}
#endif