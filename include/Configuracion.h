#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include <Arduino.h>
#include <Preferences.h>
#include <RTClib.h>

// ============================================================
// Microclima V2.0 — Configuración del Sistema
// Placa: Waveshare ESP32-S3-Touch-LCD-2
// ============================================================

// --- Pines GPIO ---
// Como no usamos la pantalla integrada (ST7789) ni la cámara,
// muchos GPIOs quedan libres. Elegimos pines seguros (no strapping).
// Strapping pins a EVITAR: GPIO 0, 3, 45, 46

#define PIN_DHT       1    // DHT22 Data
#define PIN_LUZ       40   // Relé Luz
#define PIN_EXTRACTOR 41   // Relé Extractor
#define PIN_VENTILADOR 42  // Relé Ventilador Interno

// I2C — Bus compartido: OLED SSD1306 (0x3C) + RTC DS3231 (0x68)
#define PIN_SDA       10
#define PIN_SCL       11

// --- WiFi AP ---
#define WIFI_AP_SSID     "Microclima"
#define WIFI_AP_PASSWORD "micro2025"

// --- Modos de Cultivo ---
enum ModoCultivo {
  CRECIMIENTO,
  FLORACION,
  PERSONALIZADO
};

// --- Perfil de Cultivo ---
struct PerfilCultivo {
  uint8_t horaOn;
  uint8_t minOn;
  uint8_t horaOff;
  uint8_t minOff;
  float tempMax;
  float humMax;
};

// --- Configuración Persistente (NVS) ---
struct ConfiguracionApp {
  PerfilCultivo vege;
  PerfilCultivo flora;
  PerfilCultivo personalizado;
  ModoCultivo modoActual;
  uint32_t inicioCicloUnix;
};

// Perfiles por defecto (biológicamente optimizados)
const PerfilCultivo PERFIL_VEGE_DEFAULT  = { 6, 0, 0, 0, 28.0, 70.0 };    // 18hs luz (06:00-00:00)
const PerfilCultivo PERFIL_FLORA_DEFAULT = { 7, 0, 19, 0, 26.0, 55.0 };   // 12hs luz (07:00-19:00)
const PerfilCultivo PERFIL_PERS_DEFAULT  = { 8, 0, 20, 0, 27.0, 60.0 };   // 12hs luz (08:00-20:00)

extern ConfiguracionApp config;
extern bool rtcConectado;

void cargarConfiguracion();
void guardarConfiguracion();
PerfilCultivo& obtenerPerfilActual();
DateTime obtenerHoraActual();

#endif
