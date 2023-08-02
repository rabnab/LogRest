#include "sensorHandler.h"
#include "util.h"

boolean SensorHandler::updateSensorValues()
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

    if (this->cntElem == lenWindow / 2 && !firstRun)
    {
        switchTransientToCurrent(currentTemp, currentHumi);
    }
    else if (this->cntElem == lenWindow)
    {
        this->firstRun = false;
        
        this->cntElem = 0;
        switchTransientToCurrent(currentTemp, currentHumi);
        printAccumulatedTempInfo();
        return true;
    }
    return false;
}

void SensorHandler::switchTransientToCurrent(float tempMeas, float humMeas)
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

float SensorHandler::getActualTemperatureAvg()
{
    // return currentSensorContainer.corrTemperature + currentSensorContainer.temperature / lenWindow;
    return currentSensorContainer.temperature;
}

float SensorHandler::getActualHumidityAvg()
{
    // return currentSensorContainer.corrHumidity + currentSensorContainer.humidity / lenWindow;
    return currentSensorContainer.humidity;
}

float SensorHandler::getActualTemperatureStdev()
{
    // return sqrt((currentSensorContainer.sqTemperature - currentSensorContainer.temperature * currentSensorContainer.temperature / lenWindow) / (lenWindow - 1));
    return sqrt(currentSensorContainer.sqTemperature / (lenWindow - 1));
}

float SensorHandler::getActualHumidityStdev()
{
    // return sqrt((currentSensorContainer.sqHumidiy - currentSensorContainer.humidity * currentSensorContainer.humidity / lenWindow) / (lenWindow - 1));
    return sqrt(currentSensorContainer.sqHumidiy / (lenWindow - 1));
}

void SensorHandler::initializeTemp()
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





boolean SensorHandler::init()
{
	if (htu21df.begin()) {
		this->initializeTemp();
		return true;
	}
	return false;
}




void SensorHandler::outputPressTempSensors()
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

void SensorHandler::printAccumulatedTempInfo() {
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