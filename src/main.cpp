#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include "Configuracion.h"
#include "Control.h"
#include "Menu.h"

RTC_DS3231 rtc;

void reporteSerial() {
  static uint32_t lastLog = 0;
  if (millis() - lastLog >= 15000) { // 15 segundos
    lastLog = millis();
    
    DateTime ahora = rtc.now();
    float t = getTemperatura();
    float h = getHumedad();
    bool luz = getEstadoLuz();
    const char* modo = (config.modoActual == CRECIMIENTO) ? "VEGE" : "FLORA";
    
    // Formato semi-JSON para facilitar log de base de datos futura
    char logMsg[128];
    int t_int = (int)t; int t_dec = abs((int)(t*10)%10);
    int h_int = (int)h; int h_dec = abs((int)(h*10)%10);
    
    sprintf(logMsg, "{\"time\":\"20%02d-%02d-%02d %02d:%02d:%02d\",\"temp\":%d.%d,\"hum\":%d.%d,\"luz\":%d,\"modo\":\"%s\"}",
            ahora.year() % 100, ahora.month(), ahora.day(), ahora.hour(), ahora.minute(), ahora.second(),
            t_int, t_dec, h_int, h_dec, (luz ? 1 : 0), modo);
            
    Serial.println(logMsg);
  }
}

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
  reporteSerial();
  delay(50);
}