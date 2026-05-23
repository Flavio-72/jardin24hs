#include "Pantalla.h"
#include "Configuracion.h"
#include "Control.h"
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
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

static uint8_t pantallaActual = 0;
static const uint8_t TOTAL_PANTALLAS = 3;
static uint32_t ultimoCambioPantalla = 0;
static const uint32_t INTERVALO_CLIMA = 5000;
static const uint32_t INTERVALO_ESTADO = 4000;
static const uint32_t INTERVALO_WIFI = 3000;

// --- Indicador de página (3 puntos en esquina superior derecha) ---
static void dibujarIndicador(uint8_t activa) {
  for (uint8_t i = 0; i < TOTAL_PANTALLAS; i++) {
    if (i == activa)
      oled.drawDisc(110 + i * 7, 8, 2);
    else
      oled.drawCircle(110 + i * 7, 8, 1);
  }
}

// --- Pantalla 0: Temperatura y Humedad ---
static void dibujarClima() {
  float t = obtenerTemperatura();
  float h = obtenerHumedad();
  PerfilCultivo &p = obtenerPerfilActual();

  char bufTemp[10];
  char bufHum[10];
  dtostrf(t, 1, 1, bufTemp); // Ancho 1 para evitar espacios a la izquierda
  dtostrf(h, 1, 1, bufHum);

  // Etiqueta TEMP (y=10)
  oled.setFont(u8g2_font_6x10_tr);
  oled.drawStr(0, 10, "TEMP");

  // Temperatura — fuente grande (baseline y=32)
  oled.setFont(u8g2_font_logisoso16_tn);
  uint8_t tw = oled.getStrWidth(bufTemp);
  oled.drawStr((128 - tw) / 2 - 8, 32, bufTemp);

  // °C al costado
  oled.setFont(u8g2_font_7x14B_tr);
  oled.drawStr((128 + tw) / 2 - 4, 26,
               "\xb0"
               "C");

  // Línea separadora (y=36)
  oled.drawHLine(10, 36, 108);

  // Humedad abajo (y=48)
  oled.setFont(u8g2_font_6x10_tr);
  oled.drawStr(0, 48, "HUMEDAD");

  // Humedad — fuente grande (baseline y=64)
  oled.setFont(
      u8g2_font_logisoso16_tn); // Cambiado a logisoso16 para uniformidad
  uint8_t hw = oled.getStrWidth(bufHum);
  oled.drawStr((128 - hw) / 2 - 8, 64, bufHum);

  oled.setFont(u8g2_font_7x14B_tr);
  oled.drawStr((128 + hw) / 2 - 4, 64, "%");

  dibujarIndicador(0);
}

// --- Pantalla 1: Estado de relés, Modo, Día ---
static void dibujarEstado() {
  DateTime ahora = obtenerHoraActual();

  // Hora con fuente legible pero compacta
  char bufHora[12];
  int h_rtc = ahora.hour();
  int m_rtc = ahora.minute();
  
  // Seguridad: si el RTC devuelve basura, mostrar --
  if (h_rtc > 23 || m_rtc > 59) {
    sprintf(bufHora, "--:--");
  } else {
    sprintf(bufHora, "%02d:%02d", h_rtc, m_rtc);
  }
  oled.setFont(u8g2_font_7x14B_tr);
  uint8_t tw = oled.getStrWidth(bufHora);
  oled.drawStr((128 - tw) / 2, 12, bufHora);

  // Relés — fuente compacta 6x10
  oled.setFont(u8g2_font_6x10_tr);
  oled.drawStr(0, 23, obtenerEstadoLuz() ? "LUZ: ON" : "LUZ: OFF");

  char bufExt[32];
  sprintf(bufExt, "EXT: %s [%s]", obtenerEstadoExtractor() ? "ON" : "OFF",
          nombreModoControl(obtenerControlExt()));
  oled.drawStr(0, 33, bufExt);

  char bufVent[32];
  sprintf(bufVent, "VEN: %s [%s]", obtenerEstadoVentilador() ? "ON" : "OFF",
          nombreModoControl(obtenerControlVent()));
  oled.drawStr(0, 43, bufVent);

  // Modo (izquierda) + Día del ciclo (derecha)
  const char *modo = (config.modoActual == CRECIMIENTO) ? "VEGE"
                     : (config.modoActual == FLORACION) ? "FLORA"
                                                        : "PERS";
  int diaCiclo = 0;
  if (config.inicioCicloUnix > 0 &&
      ahora.unixtime() >= config.inicioCicloUnix) {
    diaCiclo = (ahora.unixtime() - config.inicioCicloUnix) / 86400;
  }

  oled.setFont(u8g2_font_7x14B_tr);
  oled.drawStr(0, 64, modo);

  char bufDia[12];
  sprintf(bufDia, "Dia: %d", diaCiclo);
  uint8_t dw = oled.getStrWidth(bufDia);
  oled.drawStr(128 - dw, 64, bufDia);

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
    Serial.println(
        "[ERROR] Bus I2C en corto o sin resistencias Pull-Up. Abortando OLED.");
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
  if (!oledConectado)
    return;

  uint32_t intervalo = INTERVALO_CLIMA;
  if (pantallaActual == 1)
    intervalo = INTERVALO_ESTADO;
  if (pantallaActual == 2)
    intervalo = INTERVALO_WIFI;

  // Rotación automática
  if (millis() - ultimoCambioPantalla >= intervalo) {
    pantallaActual = (pantallaActual + 1) % TOTAL_PANTALLAS;

    // Si la siguiente pantalla es WiFi (2) pero hay alguien conectado al AP,
    // saltar a la 0
    if (pantallaActual == 2 && WiFi.softAPgetStationNum() > 0) {
      pantallaActual = 0;
    }

    ultimoCambioPantalla = millis();
  }

  oled.clearBuffer();

  switch (pantallaActual) {
  case 0:
    dibujarClima();
    break;
  case 1:
    dibujarEstado();
    break;
  case 2:
    dibujarWifi();
    break;
  }

  oled.sendBuffer();
}
