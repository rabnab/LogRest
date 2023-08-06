#pragma once
class tempUtil {
public:
  static void println(const char* str);
  static void print(const char* str);
  static void println(double v);
  static void print(double v);
  static int celsiusTomilliKelvin(float T);
  static int percentToPPM(float H);
  static void trim(char* str);
};