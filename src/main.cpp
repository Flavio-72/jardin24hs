#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include "Configuracion.h"
#include "Control.h"
#include "Menu.h"

RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  
  if (!rtc.begin()) {
    // Si no hay RTC, mostramos error en Serial y LCD
    // El menu intentará manejarlo
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  cargarConfig();
  inicializarControl();
  inicializarMenu();

  Serial.println("Sistema Microclima Modular v0.4 Iniciado");
}

void loop() {
  actualizarControl();
  actualizarMenu();
  delay(50);
}