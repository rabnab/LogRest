#include "communicator.h"
#include "config.h"
#include "util.h"


const char* Communicator::HOST_NAME = THEHOST;
const char* Communicator::ssid = WIFI_SSID;
const char* Communicator::psk = WIFI_PWD;

int Communicator::initializeWiFi(int statIn)
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
const char* Communicator::translateWifiState(int state)
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

void Communicator::fillQuery(char* queryBuffer, long tempMilli, const char loc[], char state)
{
    sprintf(queryBuffer, Communicator::queryTemplate, tempMilli, loc, state);
    trim(queryBuffer);
}

char* Communicator::postValuesToServer(float T, float Hum, const char* location)
{
        // String str = fillQuery("2022-01-19%2011:30:00", T, location, 'g');
        char query[(255 + strlen(queryTemplate))];
        fillQuery(query, celsiusTomilliKelvin(T), location, 'g');

        char req[(255 + strlen(queryTemplate))];
        sprintf(req, "POST %s%s HTTP/1.1", Communicator::PATH_NAME, query);
        println(req);
        char* outputBuffer = new char[256];
        int outputLen = 0;
        WiFiSSLClient client;
        if (client.connectSSL(Communicator::HOST_NAME, 443))
        {
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
            
            while (client.available())
            {
                char c = client.read();
                if (outputLen<255) {
                    outputBuffer[outputLen++] = c;
                }
            }
            if (addNewLine) {
                outputBuffer[outputLen++]=('\n');
            }
            outputBuffer[outputLen] = '\0';
        }
        else
        { // if not connected:
            char msg[255];
            snprintf(msg, 255, "connection to 'https://%s' failed", Communicator::HOST_NAME);
            println(msg);
            //while (1)
            //  ;
        }
        client.stop();
        return (outputLen>0)?outputBuffer:nullptr;
}

const char* Communicator::getSSID()
{
    return Communicator::ssid;
}

int Communicator::getWifiState() {
    return WiFi.status();
}

