#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <Arduino.h>
#include "Calendario.h"

// ============================================================
// Microclima V2.0 — Servidor Web Embebido
// WiFi AP + HTTP + WebSocket + Captive Portal DNS
// ============================================================

extern Calendario calendario;

void inicializarServidor();
void enviarEstadoWebSocket();  // Llamar cada ~2 seg desde loop
void procesarDNS();            // Llamar en cada loop() para captive portal

#endif
