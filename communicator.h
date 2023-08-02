#pragma once
#include <WiFiNINA.h>

class Communicator {
private:
	static const char* HOST_NAME;
	static const char* ssid;
	static const char* psk;

	static const char* PATH_NAME;
	static const char* getURL;
	static const char* queryTemplate;

public:
	Communicator() {};
	void fillQuery(char* queryBuffer, long tempMilli, const char loc[], char state);

	int initializeWiFi(int statIn);
	const char* translateWifiState(int state);
	char* postValuesToServer(float T, float Hum, const char* location);
	const char* getSSID();
	int getWifiState();
};

const char* Communicator::PATH_NAME = "/temperature";
const char* Communicator::getURL = "/temperature?loc=SRi/keh&lb=1&ub=10000";
const char* Communicator::queryTemplate = "?y=%ld&loc=%s&s=%c";

