#include "Configuracion.h"
#include "Control.h"
#include "Pantalla.h"
#include "Servidor.h"
#include <Arduino.h>
#include <RTClib.h>
#include <WiFi.h>
#include <Wire.h>
#include <LittleFS.h>
#include "Datalogger.h"

// ============================================================
// Microclima V2.0 — ESP32-S3 + OLED + Web Dashboard
//
// Arquitectura:
//   - Control.cpp   → Lógica VPD / Luz / Extracción (del V1.0)
//   - Pantalla.cpp  → OLED 1.3" con pantallas rotativas
//   - Servidor.cpp  → WiFi AP + HTTP + WebSocket
//   - Configuracion → Persistencia NVS (reemplaza EEPROM)
// ============================================================

RTC_DS3231 rtc;
bool rtcConectado = false;
uint32_t baseUnix = 1767225600; // Por defecto: 2026-01-01
uint32_t millisSinc = 0;

void ajustarHora(uint32_t ahoraUnix) {
  if (rtcConectado) {
    rtc.adjust(DateTime(ahoraUnix));
  }
  baseUnix = ahoraUnix;
  millisSinc = millis();
  Serial.printf("[TIME] Hora ajustada a Unix: %u\n", ahoraUnix);
}

DateTime obtenerHoraActual() {
  if (rtcConectado) {
    DateTime now = rtc.now();
    // Validación de seguridad: si el RTC devuelve basura (ej. horas > 23),
    // es que hay ruido en el bus I2C o se desconectó.
    if (now.hour() > 23 || now.minute() > 59) {
      Serial.println("[TIME] ERROR: RTC devolvió basura. Usando reloj interno.");
      uint32_t segundosTranscurridos = (millis() - millisSinc) / 1000;
      return DateTime(baseUnix + segundosTranscurridos);
    }
    return now;
  } else {
    // Si no hay RTC, sumamos el tiempo transcurrido desde el último ajuste (o arranque)
    uint32_t segundosTranscurridos = (millis() - millisSinc) / 1000;
    return DateTime(baseUnix + segundosTranscurridos);
  }
}

// Reporte JSON por Serial (debug / datalog USB)
void reporteSerial() {
  static uint32_t ultimoReporte = 0;
  if (millis() - ultimoReporte < 15000)
    return; // Cada 15 seg
  ultimoReporte = millis();

  DateTime ahora = obtenerHoraActual();
  float t = obtenerTemperatura();
  float h = obtenerHumedad();

  PerfilCultivo &p = obtenerPerfilActual();
  int diaCiclo = 0;
  if (config.inicioCicloUnix > 0 &&
      ahora.unixtime() >= config.inicioCicloUnix) {
    diaCiclo = (ahora.unixtime() - config.inicioCicloUnix) / 86400;
  }

  const char *modo = (config.modoActual == CRECIMIENTO)   ? "VEGE"
                     : (config.modoActual == FLORACION) ? "FLORA"
                                                       : "PERS";

  // JSON compacto por serial
  Serial.printf("{\"time\":\"%04d-%02d-%02d %02d:%02d:%02d\","
                "\"temp\":%.1f,\"hum\":%.1f,"
                "\"tMax\":%.1f,\"hMax\":%.1f,"
                "\"luz\":\"%s\",\"ext\":\"%s\",\"vent\":\"%s\","
                "\"controlExt\":\"%s\",\"controlVent\":\"%s\","
                "\"modo\":\"%s\",\"horario\":\"%02d-%02d\",\"dia\":%d}\n",
                ahora.year(), ahora.month(), ahora.day(), ahora.hour(),
                ahora.minute(), ahora.second(), t, h, p.tempMax, p.humMax,
                obtenerEstadoLuz() ? "on" : "off",
                obtenerEstadoExtractor() ? "on" : "off",
                obtenerEstadoVentilador() ? "on" : "off",
                nombreModoControl(obtenerControlExt()),
                nombreModoControl(obtenerControlVent()), modo, p.horaOn,
                p.horaOff, diaCiclo);
}

void setup() {
  Serial.begin(115200);
  delay(500); // Esperar estabilización USB-CDC

  // I2C en pines definidos (compartido: OLED + RTC)
  pinMode(PIN_SDA, INPUT_PULLUP);
  pinMode(PIN_SCL, INPUT_PULLUP);
  delay(10);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setTimeOut(
      20); // Timeout corto de 20ms para evitar bloqueos si falta un componente

  // Hardware Check para I2C
  if (digitalRead(PIN_SDA) == LOW || digitalRead(PIN_SCL) == LOW) {
    Serial.println(
        "[ERROR] I2C Bus en corto o sin pull-ups (SDA o SCL en LOW)!");
    rtcConectado = false;
  } else {
    // RTC
    if (!rtc.begin()) {
      Serial.println("[ERROR] RTC DS3231 no encontrado!");
      rtcConectado = false;
    } else {
      rtcConectado = true;
      if (rtc.lostPower()) {
        Serial.println("[RTC] Ajustando hora de compilación");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      }
    }
  }

  // Módulos
  if (!LittleFS.begin(true)) {
    Serial.println("[FS] ERROR crítico: No se pudo montar LittleFS");
  }
  
  cargarConfiguracion();
  calendario.begin();
  datalogger.begin();
  prepararControl();
  inicializarPantalla();
  inicializarServidor();

  Serial.println("========================================");
  Serial.println("  Microclima V2.0 — ESP32-S3 Iniciado  ");
  Serial.println("========================================");
  Serial.printf("  WiFi AP: %s\n", WIFI_AP_SSID);
  Serial.printf("  IP: %s\n", WiFi.softAPIP().toString().c_str());
  Serial.println("========================================");
}

void loop() {
  procesarDNS(); // Captive portal DNS (rápido, sin bloqueos)

  static uint32_t ultimoTick = 0;
  // Actualizamos hardware solo 1 vez por segundo
  // Esto evita bloquear el WiFi si un sensor I2C (como el RTC) se desconecta
  if (millis() - ultimoTick >= 1000) {
    ultimoTick = millis();
    actualizarControl();
    actualizarPantalla();
  }

  enviarEstadoWebSocket(); // Ya tiene su propio timer interno de 2s
  reporteSerial();         // Ya tiene su propio timer de 15s
  delay(10);               // Yield para WiFi/async
}