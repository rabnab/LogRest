#pragma once
#ifndef RK_UTIL_H
#define RK_UTIL_H

void  println(const char* str) {
    Serial.println(str);
    Serial1.println(str);
}

void  print(const char* str) {
    Serial.print(str);
    Serial1.print(str);
}


void  println(double v) {
    Serial.print(v);
    Serial1.print(v);
}

void print(double v) {
    Serial.print(v);
    Serial1.print(v);
}

//ChatGPT :)
void trim(char* str)
{
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
    }
    else {
        shift = 0;
    }

    // Null-terminate the new string
    *(str + (end - start + 1 - shift)) = '\0';
}

int celsiusTomilliKelvin(float T)
{
    // round to nearest milliKelvin (by addign 0.5)
    return 0.5 + 1000 * (T + 273.15);
}

#endif