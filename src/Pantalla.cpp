#include "Pantalla.h"
#include "Control.h"
#include "Configuracion.h"
#include <WiFi.h>

// ============================================================
// Microclima V2.0 — OLED 1.3" SSD1306 128x64
//
// Pantallas alternadas cada 3 segundos con caracteres grandes:
//   Pantalla 0: Temperatura (número grande) + Humedad
//   Pantalla 1: Estado relés + Modo + Día del ciclo
//   Pantalla 2: WiFi SSID + IP de conexión
// ============================================================

// SSD1306 128x64 I2C — Si resulta ser SH1106, cambiar esta línea:
// U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

static uint8_t pantallaActual = 0;
static const uint8_t TOTAL_PANTALLAS = 3;
static uint32_t ultimoCambioPantalla = 0;
static const uint32_t INTERVALO_ROTACION = 3000; // 3 segundos

// --- Indicador de página (3 puntos en esquina inferior derecha) ---
static void dibujarIndicador(uint8_t activa) {
  for (uint8_t i = 0; i < TOTAL_PANTALLAS; i++) {
    if (i == activa) oled.drawDisc(110 + i * 7, 62, 2);
    else             oled.drawCircle(110 + i * 7, 62, 1);
  }
}

// --- Pantalla 0: Temperatura y Humedad ---
static void dibujarClima() {
  float t = obtenerTemperatura();
  float h = obtenerHumedad();

  char bufTemp[8];
  char bufHum[8];
  dtostrf(t, 4, 1, bufTemp);
  dtostrf(h, 4, 1, bufHum);

  // Etiqueta
  oled.setFont(u8g2_font_6x10_tr);
  oled.drawStr(0, 8, "TEMPERATURA");

  // Temperatura — fuente mediana-grande (18px, cabe bien)
  oled.setFont(u8g2_font_logisoso18_tn);
  uint8_t tw = oled.getStrWidth(bufTemp);
  oled.drawStr((128 - tw) / 2 - 8, 30, bufTemp);

  // °C al costado
  oled.setFont(u8g2_font_7x14B_tr);
  oled.drawStr((128 + tw) / 2 - 4, 24, "\xb0""C");

  // Línea separadora
  oled.drawHLine(10, 35, 108);

  // Humedad abajo, más compacta
  oled.setFont(u8g2_font_6x10_tr);
  oled.drawStr(0, 48, "HUMEDAD");

  oled.setFont(u8g2_font_logisoso16_tn);
  uint8_t hw = oled.getStrWidth(bufHum);
  oled.drawStr(52, 60, bufHum);

  oled.setFont(u8g2_font_6x10_tr);
  oled.drawStr(52 + hw + 2, 60, "%");

  dibujarIndicador(0);
}

// --- Pantalla 1: Estado de relés, Modo, Día ---
static void dibujarEstado() {
  DateTime ahora = obtenerHoraActual();

  // Hora con fuente legible pero compacta
  char bufHora[6];
  sprintf(bufHora, "%02d:%02d", ahora.hour(), ahora.minute());
  oled.setFont(u8g2_font_7x14B_tr);
  uint8_t tw = oled.getStrWidth(bufHora);
  oled.drawStr((128 - tw) / 2, 12, bufHora);

  // Línea divisoria
  oled.drawHLine(0, 15, 128);

  // Relés — fuente compacta 6x10
  oled.setFont(u8g2_font_6x10_tr);
  oled.drawStr(0, 28, obtenerEstadoLuz() ? "LUZ: ON" : "LUZ: OFF");

  char bufExt[20];
  sprintf(bufExt, "EXT: %s [%s]",
    obtenerEstadoExtractor() ? "ON" : "OFF",
    nombreModoControl(obtenerControlExt()));
  oled.drawStr(0, 40, bufExt);

  char bufVent[20];
  sprintf(bufVent, "VEN: %s [%s]",
    obtenerEstadoVentilador() ? "ON" : "OFF",
    nombreModoControl(obtenerControlVent()));
  oled.drawStr(0, 52, bufVent);

  // Modo + Día del ciclo
  const char* modo = (config.modoActual == CRECIMIENTO) ? "VEGE" : "FLORA";
  int diaCiclo = 0;
  if (config.inicioCicloUnix > 0 && ahora.unixtime() >= config.inicioCicloUnix) {
    diaCiclo = (ahora.unixtime() - config.inicioCicloUnix) / 86400;
  }

  char bufModo[24];
  sprintf(bufModo, "%s D%d", modo, diaCiclo);
  oled.setFont(u8g2_font_7x14B_tr);
  oled.drawStr(0, 64, bufModo);

  dibujarIndicador(1);
}

// --- Pantalla 2: WiFi + IP ---
static void dibujarWifi() {
  oled.setFont(u8g2_font_6x10_tr);
  oled.drawStr(0, 10, "WIFI AP:");

  oled.setFont(u8g2_font_7x14B_tr);
  oled.drawStr(0, 28, WIFI_AP_SSID);

  oled.drawHLine(0, 32, 128);

  oled.setFont(u8g2_font_6x10_tr);
  oled.drawStr(0, 45, "Conectar y abrir:");

  // IP del AP
  IPAddress ip = WiFi.softAPIP();
  char bufIP[20];
  sprintf(bufIP, "%s", ip.toString().c_str());
  oled.setFont(u8g2_font_7x14B_tr);
  oled.drawStr(0, 62, bufIP);

  dibujarIndicador(2);
}

// --- API Pública ---
bool oledConectado = false;

void inicializarPantalla() {
  if (digitalRead(PIN_SDA) == LOW || digitalRead(PIN_SCL) == LOW) {
    Serial.println("[ERROR] Bus I2C en corto o sin resistencias Pull-Up. Abortando OLED.");
    oledConectado = false;
    return;
  }

  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0) {
    oledConectado = true;
  }
  if (!oledConectado) {
    Wire.beginTransmission(0x3D);
    if (Wire.endTransmission() == 0) {
      oledConectado = true;
    }
  }

  if (oledConectado) {
    oled.begin();
    oled.setContrast(180); // Brillo moderado
    oled.enableUTF8Print();
  } else {
    Serial.println("[ERROR] Pantalla OLED no encontrada en I2C!");
  }
}

void actualizarPantalla() {
  if (!oledConectado) return;

  // Rotación automática
  if (millis() - ultimoCambioPantalla >= INTERVALO_ROTACION) {
    pantallaActual = (pantallaActual + 1) % TOTAL_PANTALLAS;
    ultimoCambioPantalla = millis();
  }

  oled.clearBuffer();

  switch (pantallaActual) {
    case 0: dibujarClima();  break;
    case 1: dibujarEstado(); break;
    case 2: dibujarWifi();   break;
  }

  oled.sendBuffer();
}
