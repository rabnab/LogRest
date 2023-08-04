#pragma once

#include <Arduino_LSM6DS3.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HTU21DF.h>

#include "util.h"


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

Adafruit_HTU21DF htu21df;
// constants for calculating the sensor values
const float LAM = 0.05;
const int lenWindow = 30;
const int plusThreshold = 30;
const int minusThreshold = -30;
int cntElem = 0;
boolean firstRun = false;

SensorData currentSensorContainer;
SensorData transientSensorContainer;

void SensorHandlerInit() {
	htu21df = Adafruit_HTU21DF(); // I2C
};

boolean updateSensorValues();

void switchTransientToCurrent(float tempMeas, float humMeas);

float getActualTemperatureAvg();

float getActualHumidityAvg();

float getActualTemperatureStdev();

float getActualHumidityStdev();

void outputPressTempSensors();

void printAccumulatedTempInfo();

void initializeTemp();

boolean startSensors();

boolean updateSensorValues()
{
	float currentTemp = htu21df.readTemperature();
	float currentHumi = htu21df.readHumidity();
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

		cntElem = 0;
		switchTransientToCurrent(currentTemp, currentHumi);
		printAccumulatedTempInfo();
		return true;
	}
	return false;
}

void switchTransientToCurrent(float tempMeas, float humMeas)
{
	currentSensorContainer.temperature = transientSensorContainer.temperature;
	currentSensorContainer.humidity = transientSensorContainer.humidity;
	transientSensorContainer.temperature = tempMeas;
	transientSensorContainer.humidity = humMeas;
	currentSensorContainer.sqTemperature = transientSensorContainer.sqTemperature;
	currentSensorContainer.sqHumidiy = transientSensorContainer.sqHumidiy;
	transientSensorContainer.sqTemperature = 0;//tempMeas * tempMeas;
	transientSensorContainer.sqHumidiy = 0;//humMeas * humMeas;
}

float getActualTemperatureAvg()
{
	// return currentSensorContainer.corrTemperature + currentSensorContainer.temperature / lenWindow;
	return currentSensorContainer.temperature;
}

float getActualHumidityAvg()
{
	// return currentSensorContainer.corrHumidity + currentSensorContainer.humidity / lenWindow;
	return currentSensorContainer.humidity;
}

float getActualTemperatureStdev()
{
	// return sqrt((currentSensorContainer.sqTemperature - currentSensorContainer.temperature * currentSensorContainer.temperature / lenWindow) / (lenWindow - 1));
	return sqrt(currentSensorContainer.sqTemperature / (lenWindow - 1));
}

float getActualHumidityStdev()
{
	// return sqrt((currentSensorContainer.sqHumidiy - currentSensorContainer.humidity * currentSensorContainer.humidity / lenWindow) / (lenWindow - 1));
	return sqrt(currentSensorContainer.sqHumidiy / (lenWindow - 1));
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





boolean startSensors()
{
	if (htu21df.begin()) {
		initializeTemp();
		return true;
	}
	return false;
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