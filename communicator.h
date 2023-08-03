#pragma once
#include <WiFiNINA.h>
#include "config.h"
#include "util.h"



void fillQuery(char* queryBuffer, long tempMilli, const char loc[], char state);

int initializeWiFi(int statIn);
const char* translateWifiState(int state);
char* postValuesToServer(float T, float Hum, const char* location);
const char* getSSID();
int getWifiState();

const char* PATH_NAME = "/temperature";
const char* getURL = "/temperature?loc=SRi/keh&lb=1&ub=10000";
const char* queryTemplate = "?y=%ld&loc=%s&s=%c";

static const char* HOST_NAME = THEHOST;
static const char* ssid = WIFI_SSID;
static char* psk = WIFI_PWD;

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

void fillQuery(char* queryBuffer, long tempMilli, const char loc[], char state)
{
	sprintf(queryBuffer, queryTemplate, tempMilli, loc, state);
	trim(queryBuffer);
}

char* postValuesToServer(float T, float Hum, const char* location)
{
	// String str = fillQuery("2022-01-19%2011:30:00", T, location, 'g');
	char query[(255 + strlen(queryTemplate))];
	fillQuery(query, celsiusTomilliKelvin(T), location, 'g');

	char req[(255 + strlen(queryTemplate))];
	sprintf(req, "POST %s%s HTTP/1.1", PATH_NAME, query);
	println(req);
	char* outputBuffer = new char[256];
	int outputLen = 0;
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
		int availableBytes = client.available();
		boolean addNewLine = availableBytes;

		while (client.available())
		{
			char c = client.read();
			if (outputLen < 255) {
				outputBuffer[outputLen++] = c;
			}
		}
		if (addNewLine) {
			outputBuffer[outputLen++] = ('\n');
		}
		outputBuffer[outputLen] = '\0';
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
	return (outputLen > 0) ? outputBuffer : nullptr;
}

const char* getSSID()
{
	return ssid;
}

int getWifiState() {
	return WiFi.status();
}