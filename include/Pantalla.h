#ifndef PANTALLA_H
#define PANTALLA_H

#include <Arduino.h>
#include <U8g2lib.h>

// ============================================================
// Microclima V2.0 — Display OLED 1.3" SSD1306 (128x64, I2C)
// Modo: pantalla pasiva con rotación automática cada 3 segundos
// ============================================================

void inicializarPantalla();
void actualizarPantalla();

#endif
