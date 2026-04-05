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

enum ModoManual { M_AUTO, M_ON, M_OFF };

void prepararControl();
void actualizarControl();

float obtenerTemperatura();
float obtenerHumedad();
bool obtenerEstadoLuz();
bool obtenerEstadoExtractor();

void establecerVentiladorManual(ModoManual modo);
void establecerExtractorManual(ModoManual modo);
ModoManual obtenerModoManualVent();
ModoManual obtenerModoManualExt();
uint32_t obtenerTiempoRestanteManualVent();
uint32_t obtenerTiempoRestanteManualExt();

#endif
