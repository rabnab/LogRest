#include "util.h"

void tempUtil::println(const char* str) {
  Serial.println(str);
  Serial1.println(str);
}

void tempUtil::print(const char* str) {
  Serial.print(str);
  Serial1.print(str);
}


void tempUtil::println(double v) {
  Serial.print(v);
  Serial1.print(v);
}

void tempUtil::print(double v) {
  Serial.print(v);
  Serial1.print(v);
}

//ChatGPT :)
void tempUtil::trim(char* str) {
  if (str == NULL) return;

  char* start = str;
  char* end = str + strlen(str) - 1;

  // Find the first non-whitespace character
  while (isspace(*start)) start++;

  // Find the last non-whitespace character
  while (isspace(*end) && end >= start) end--;

  // Shift characters to the left to remove leading whitespace
  int shift = start - str;
  if (shift > 0) {
    memmove(str, start, end - start + 2);  // Include the null terminator
  } else {
    shift = 0;
  }

  // Null-terminate the new string
  *(str + (end - start + 1 - shift)) = '\0';
}

int tempUtil::celsiusTomilliKelvin(float T) {
  // round to nearest milliKelvin (by addign 0.5)
  return 0.5 + 1000 * (T + 273.15);
}

int tempUtil::percentToPPM(float H) {
  // return integer ppm
  return H*10000;
}
