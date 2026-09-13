#pragma once
#include <Arduino.h>

// User verified repaired GPIO4 -> display LED control; high enables backlight.
// This reports commanded PWM, not measured light output or wiring correctness.
static constexpr uint8_t BACKLIGHT_PIN = 4;
static bool backlightReady = false;
static uint32_t backlightStarted = 0;
static uint16_t backlightDuty = 0;
static const char *backlightPhase = "init";

void startBacklightTest() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, LOW);
  backlightReady = ledcAttach(BACKLIGHT_PIN, 5000, 10);
  if (backlightReady) backlightReady = ledcWrite(BACKLIGHT_PIN, 1023);
  backlightDuty = backlightReady ? 1023 : 0;
  backlightPhase = backlightReady ? "full" : "error";
  backlightStarted = millis();
}

String backlightStatus() {
  return String("{\"gpio\":4,\"pwm_hz\":5000,\"ready\":") + (backlightReady ? "true" : "false") +
    ",\"duty\":" + String(backlightDuty) + ",\"max_duty\":1023,\"phase\":\"" + backlightPhase + "\"}";
}

