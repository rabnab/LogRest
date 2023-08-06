#module for general (physical) computations
import math

def dewPoint(temperatureCelsius, humidityPerc):
    return magnusDewPoint(temperatureCelsius, humidityPerc, 17.625, 243.04)

def magnusDewPoint(temperatureCelsius, humidityPerc, a, b):
    alpha = math.log(humidityPerc/100)+ a * temperatureCelsius/(b + temperatureCelsius)
    return b * alpha /(a-alpha)
