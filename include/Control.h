#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>
#include <DHT.h>
#include "Configuracion.h"
#include <RTClib.h>

#define DHTPIN 2
#define DHTTYPE DHT22

#define PIN_LUZ 13
#define PIN_EXTRACTOR 11
#define PIN_VENTILADOR_INT 12

extern RTC_DS3231 rtc;

void inicializarControl();
void actualizarControl();

float getTemperatura();
float getHumedad();
bool getEstadoLuz();

#endif
