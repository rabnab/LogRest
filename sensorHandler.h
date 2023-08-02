#pragma once

#include <Arduino_LSM6DS3.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HTU21DF.h>

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

class SensorHandler {
private:
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
public:
	SensorHandler() {
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



	boolean init();

};