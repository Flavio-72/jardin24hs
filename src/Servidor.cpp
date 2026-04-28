#include "Servidor.h"
#include "Configuracion.h"
#include "Control.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// ============================================================
// Microclima V2.0 — Servidor Web + WebSocket
//
// Endpoints:
//   GET /            → Dashboard HTML (desde LittleFS)
//   GET /api/estado  → JSON con estado actual
//   GET /api/config  → JSON con configuración
//   POST /api/config → Guardar nueva configuración
//   WS  /ws          → Stream en tiempo real
// ============================================================

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;
Calendario calendario;

static uint32_t ultimoEnvioWS = 0;

// --- Construir JSON de estado actual ---
static String construirJsonEstado() {
  JsonDocument doc;
  DateTime ahora = obtenerHoraActual();

  doc["tipo"] = "estado";
  doc["temp"] = round(obtenerTemperatura() * 10.0) / 10.0;
  doc["hum"] = round(obtenerHumedad() * 10.0) / 10.0;
  doc["luz"] = obtenerEstadoLuz();
  doc["ext"] = obtenerEstadoExtractor();
  doc["vent"] = obtenerEstadoVentilador();
  doc["modo"] = (config.modoActual == CRECIMIENTO) ? "VEGE" : (config.modoActual == FLORACION) ? "FLORA" : "PERSONALIZADO";
  doc["controlExt"] = nombreModoControl(obtenerControlExt());
  doc["controlVent"] = nombreModoControl(obtenerControlVent());

  // Día del ciclo
  int diaCiclo = 0;
  if (config.inicioCicloUnix > 0 &&
      ahora.unixtime() >= config.inicioCicloUnix) {
    diaCiclo = (ahora.unixtime() - config.inicioCicloUnix) / 86400;
  }
  doc["dia"] = diaCiclo;

  // Hora
  char bufHora[6];
  sprintf(bufHora, "%02d:%02d", ahora.hour(), ahora.minute());
  doc["hora"] = bufHora;

  // Perfil actual
  PerfilCultivo &p = obtenerPerfilActual();
  doc["tempMax"] = p.tempMax;
  doc["humMax"] = p.humMax;

  String json;
  serializeJson(doc, json);
  return json;
}

// --- Construir JSON de configuración ---
static String construirJsonConfig() {
  JsonDocument doc;
  DateTime ahora = obtenerHoraActual();

  doc["modo"] = (config.modoActual == CRECIMIENTO) ? "VEGE" : (config.modoActual == FLORACION) ? "FLORA" : "PERSONALIZADO";
  doc["inicioCicloUnix"] = config.inicioCicloUnix;

  // Timestamp actual para referencia
  doc["unixActual"] = ahora.unixtime();

  // Perfil Vegetativo
  JsonObject vege = doc["vege"].to<JsonObject>();
  vege["horaOn"] = config.vege.horaOn;
  vege["minOn"] = config.vege.minOn;
  vege["horaOff"] = config.vege.horaOff;
  vege["minOff"] = config.vege.minOff;
  vege["tempMax"] = config.vege.tempMax;
  vege["humMax"] = config.vege.humMax;

  // Perfil Floración
  JsonObject flora = doc["flora"].to<JsonObject>();
  flora["horaOn"] = config.flora.horaOn;
  flora["minOn"] = config.flora.minOn;
  flora["horaOff"] = config.flora.horaOff;
  flora["minOff"] = config.flora.minOff;
  flora["tempMax"] = config.flora.tempMax;
  flora["humMax"] = config.flora.humMax;

  // Perfil Personalizado
  JsonObject pers = doc["personalizado"].to<JsonObject>();
  pers["horaOn"] = config.personalizado.horaOn;
  pers["minOn"] = config.personalizado.minOn;
  pers["horaOff"] = config.personalizado.horaOff;
  pers["minOff"] = config.personalizado.minOff;
  pers["tempMax"] = config.personalizado.tempMax;
  pers["humMax"] = config.personalizado.humMax;

  String json;
  serializeJson(doc, json);
  return json;
}

// --- WebSocket: recepción de comandos ---
static void onWebSocketEvent(AsyncWebSocket *server,
                             AsyncWebSocketClient *client, AwsEventType type,
                             void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len &&
        info->opcode == WS_TEXT) {
      data[len] = 0; // Null-terminate

      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, (char *)data);
      if (err)
        return;

      const char *cmd = doc["cmd"];
      if (!cmd)
        return;

      if (strcmp(cmd, "setControlExt") == 0) {
        const char *valor = doc["valor"];
        if (strcmp(valor, "ON") == 0)
          establecerControlExt(C_ON);
        else if (strcmp(valor, "OFF") == 0)
          establecerControlExt(C_OFF);
        else
          establecerControlExt(C_AUTO);
      } else if (strcmp(cmd, "setControlVent") == 0) {
        const char *valor = doc["valor"];
        if (strcmp(valor, "ON") == 0)
          establecerControlVent(C_ON);
        else if (strcmp(valor, "OFF") == 0)
          establecerControlVent(C_OFF);
        else
          establecerControlVent(C_AUTO);
      } else if (strcmp(cmd, "setConfig") == 0) {
        // Cambiar modo
        if (doc["modo"].is<const char*>()) {
          const char *modo = doc["modo"];
          if (strcmp(modo, "FLORA") == 0) config.modoActual = FLORACION;
          else if (strcmp(modo, "PERSONALIZADO") == 0) config.modoActual = PERSONALIZADO;
          else config.modoActual = CRECIMIENTO;
        }

        // Cambiar perfil actual
        PerfilCultivo &p = obtenerPerfilActual();
        if (doc["horaOn"].is<int>())
          p.horaOn = doc["horaOn"];
        if (doc["tempMax"].is<float>())
          p.tempMax = doc["tempMax"];
        if (doc["humMax"].is<float>())
          p.humMax = doc["humMax"];

        // Inicio del ciclo
        if (doc["inicioCicloUnix"].is<uint32_t>()) {
          config.inicioCicloUnix = doc["inicioCicloUnix"];
        }

        guardarConfiguracion();
      }

      // Responder con estado actualizado
      client->text(construirJsonEstado());
    }
  } else if (type == WS_EVT_CONNECT) {
    Serial.printf("[WS] Cliente #%u conectado desde %s\n", client->id(),
                  client->remoteIP().toString().c_str());
    client->text(construirJsonEstado());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("[WS] Cliente #%u desconectado\n", client->id());
  }
}

// --- Inicialización ---
void inicializarServidor() {
  // Montar LittleFS para servir archivos web
  if (!LittleFS.begin(true)) {
    Serial.println("[FS] Error montando LittleFS");
  }

  // WiFi Access Point
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  
  if (!WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD)) {
    Serial.println("[WiFi] ERROR crítico: No se pudo iniciar el AP");
  } else {
    Serial.print("[WiFi] AP iniciado: ");
    Serial.println(WIFI_AP_SSID);
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.softAPIP());
  }

  // DNS Captive Portal — resuelve CUALQUIER dominio a nuestra IP
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println("[DNS] Captive portal DNS iniciado");

  // --- Rutas HTTP ---

  // Catch-all absoluto (Portal Cautivo Definitivo sin redirecciones)
  // Cualquier URL que el celular pida (sea la IP, google.com, o generate_204)
  // recibe directamente la página web o el error si no está subida.
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
      return;
    }

    // Redirección del Portal Cautivo:
    // Si Android/iOS pide una URL de validación (ej. www.googleapis.cn),
    // lo redireccionamos explícitamente a nuestra IP. Así el teléfono
    // actualiza la cabecera y muestra 192.168.4.1 en lugar del dominio raro.
    if (request->host() != "192.168.4.1") {
      request->redirect("http://192.168.4.1/");
      return;
    }

    if (LittleFS.exists("/index.html")) {
      request->send(LittleFS, "/index.html", "text/html");
    } else {
      String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'></head>";
      html += "<body style='font-family:sans-serif; text-align:center; padding:20px; background:#111; color:#eee;'>";
      html += "<h2 style='color:#f59e0b;'>Falta el Dashboard</h2>";
      html += "<p>El Firmware se instal&oacute; bien, pero la pantalla web no est&aacute; en la memoria de la placa.</p>";
      html += "<p>En PlatformIO, ve al panel del Alien -> Project Tasks -> esp32s3 -> Platform -> <b>Upload Filesystem Image</b></p>";
      html += "</body></html>";
      request->send(200, "text/html", html);
    }
  });

  // Archivos estáticos del Calendario
  server.on("/calendario.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/calendario.css", "text/css");
  });
  server.on("/calendario.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/calendario.js", "application/javascript");
  });

  // API: Estado actual
  server.on("/api/estado", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", construirJsonEstado());
  });

  // API: Configuración actual
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", construirJsonConfig());
  });

  // API: Guardar configuración
  server.on(
      "/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, (char *)data);
        if (err) {
          request->send(400, "application/json",
                        "{\"error\":\"JSON inválido\"}");
          return;
        }

        if (doc["modo"].is<const char*>()) {
          const char *modo = doc["modo"];
          if (strcmp(modo, "FLORA") == 0) config.modoActual = FLORACION;
          else if (strcmp(modo, "PERSONALIZADO") == 0) config.modoActual = PERSONALIZADO;
          else config.modoActual = CRECIMIENTO;
        }

        // Perfiles completos
        if (doc["vege"].is<JsonObject>()) {
          JsonObject v = doc["vege"];
          if (v["horaOn"].is<int>()) config.vege.horaOn = v["horaOn"];
          if (v["minOn"].is<int>()) config.vege.minOn = v["minOn"];
          // Auto-cálculo para VEGE: 18 horas de luz
          config.vege.horaOff = (config.vege.horaOn + 18) % 24;
          config.vege.minOff = config.vege.minOn;
          if (v["tempMax"].is<float>()) config.vege.tempMax = v["tempMax"];
          if (v["humMax"].is<float>()) config.vege.humMax = v["humMax"];
        }
        if (doc["flora"].is<JsonObject>()) {
          JsonObject f = doc["flora"];
          if (f["horaOn"].is<int>()) config.flora.horaOn = f["horaOn"];
          if (f["minOn"].is<int>()) config.flora.minOn = f["minOn"];
          // Auto-cálculo para FLORA: 12 horas de luz
          config.flora.horaOff = (config.flora.horaOn + 12) % 24;
          config.flora.minOff = config.flora.minOn;
          if (f["tempMax"].is<float>()) config.flora.tempMax = f["tempMax"];
          if (f["humMax"].is<float>()) config.flora.humMax = f["humMax"];
        }
        if (doc["personalizado"].is<JsonObject>()) {
          JsonObject p = doc["personalizado"];
          if (p["horaOn"].is<int>()) config.personalizado.horaOn = p["horaOn"];
          if (p["minOn"].is<int>()) config.personalizado.minOn = p["minOn"];
          if (p["horaOff"].is<int>()) config.personalizado.horaOff = p["horaOff"];
          if (p["minOff"].is<int>()) config.personalizado.minOff = p["minOff"];
          if (p["tempMax"].is<float>()) config.personalizado.tempMax = p["tempMax"];
          if (p["humMax"].is<float>()) config.personalizado.humMax = p["humMax"];
        }

        if (doc["inicioCicloUnix"].is<uint32_t>()) {
          config.inicioCicloUnix = doc["inicioCicloUnix"];
        }

        guardarConfiguracion();
        request->send(200, "application/json", construirJsonConfig());
      });

  // API: Registrar en Calendario
  server.on(
      "/api/calendario", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, (char *)data);
        if (err) {
          request->send(400, "application/json", "{\"error\":\"JSON inválido\"}");
          return;
        }

        String tipo = doc["tipo"] | "nota";
        int ml = doc["ml"] | 0;
        String nota = doc["nota"] | "";

        if (calendario.registrarEvento(tipo, ml, nota)) {
          request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
          request->send(500, "application/json", "{\"error\":\"Error al guardar\"}");
        }
      });

  // WebSocket
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.begin();
  Serial.println("[HTTP] Servidor iniciado en puerto 80");
}

// --- Enviar estado por WebSocket (llamar desde loop cada ~2 seg) ---
void enviarEstadoWebSocket() {
  if (millis() - ultimoEnvioWS < 2000)
    return;
  ultimoEnvioWS = millis();

  if (ws.count() > 0) {
    ws.textAll(construirJsonEstado());
  }

  // Limpiar clientes desconectados
  ws.cleanupClients();
}

// --- Procesar DNS captive portal (llamar en cada loop) ---
void procesarDNS() {
  dnsServer.processNextRequest();
}
