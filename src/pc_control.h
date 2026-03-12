#pragma once
#include <Arduino.h>

const int PWR_PIN = 8; // Пин управления

void setupHardware() {
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
}

void pressPowerButton(int ms = 500) {
    digitalWrite(PWR_PIN, HIGH);
    delay(ms);
    digitalWrite(PWR_PIN, LOW);
}
