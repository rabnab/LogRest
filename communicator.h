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
	boolean outputVerbose = false;

public:
	Communicator() {
		PATH_NAME = "/temperature";
		getURL = "/temperature?loc=SRi/keh&lb=1&ub=10000";

		queryTemplate = "?y=%ld&loc=%s&s=%c";
	};
	void fillQuery(char* queryBuffer, long tempMilli, const char loc[], char state);

	int initializeWiFi(int statIn);
	const char* translateWifiState(int state);
	char* postValuesToServer(float T, float Hum, const char* location);
	const char* getSSID();
	int getWifiState();
};


