#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>
#include <DHT.h>
#include <RTClib.h>
#include "Configuracion.h"

// ============================================================
// Microclima V2.0 — Control de Actuadores y Sensores
// ============================================================

extern RTC_DS3231 rtc;

// Modo de control: AUTO (lógica VPD) o MANUAL (forzado ON/OFF)
enum ModoControl { C_AUTO, C_ON, C_OFF };

void prepararControl();
void actualizarControl();

// Lectura de sensores
float obtenerTemperatura();
float obtenerHumedad();

// Estado de actuadores
bool obtenerEstadoLuz();
bool obtenerEstadoExtractor();
bool obtenerEstadoVentilador();

// Control manual con nombre actualizado (controlExt/controlVent)
void establecerControlVent(ModoControl modo);
void establecerControlExt(ModoControl modo);
ModoControl obtenerControlVent();
ModoControl obtenerControlExt();
uint32_t obtenerTiempoRestanteControlVent();
uint32_t obtenerTiempoRestanteControlExt();

// Utilidad: nombre del modo para el JSON/Web
const char* nombreModoControl(ModoControl modo);

#endif
