#include "Control.h"
#include "Datalogger.h"
#include <math.h>

// ============================================================
// Microclima V2.0 — Lógica de Control (ESP32-S3)
// Adaptado de V1.0: misma lógica VPD + Pulso Respiro + Winter Pulse
// Cambios: pines GPIO nuevos, renombrado a ModoControl, sin LCD
// ============================================================

DHT dht(PIN_DHT, DHT22);
float temperatura = 0.0;
float humedad = 0.0;
bool estadoLuz = false;
uint32_t ultimaLectura = 0;

// Variables de Control (antes "Manual")
ModoControl controlVent = C_AUTO;
ModoControl controlExt = C_AUTO;
uint32_t inicioControlVent = 0;
uint32_t inicioControlExt = 0;
const uint32_t DURACION_CONTROL_MANUAL = 1800000; // 30 minutos

void prepararControl() {
  dht.begin();
  pinMode(PIN_LUZ, OUTPUT);
  pinMode(PIN_EXTRACTOR, OUTPUT);
  pinMode(PIN_VENTILADOR, OUTPUT);

  // Condición Inicial Segura (Relés lógica inversa)
  digitalWrite(PIN_LUZ, HIGH);       // Apagado
  digitalWrite(PIN_EXTRACTOR, HIGH); // Apagado
  digitalWrite(PIN_VENTILADOR, LOW); // Encendido 24/7
}

void actualizarControl() {
  // Leer sensores cada 2 segundos
  if (millis() - ultimaLectura > 2000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t))
      temperatura = t;
    if (!isnan(h))
      humedad = h;

    // Registrar en el historial cada 10 min (manejado internamente por
    // datalogger)
    datalogger.registrar(temperatura, humedad, obtenerVPD());

    ultimaLectura = millis();
  }

  PerfilCultivo &p = obtenerPerfilActual();
  DateTime ahora = obtenerHoraActual();
  int minActuales = ahora.hour() * 60 + ahora.minute();
  int minOn = p.horaOn * 60 + p.minOn;
  int minOff = p.horaOff * 60 + p.minOff;

  // --- Lógica de Luz ---
  if (minOn < minOff) {
    estadoLuz = (minActuales >= minOn && minActuales < minOff);
  } else { // Cruce de medianoche
    estadoLuz = (minActuales >= minOn || minActuales < minOff);
  }
  digitalWrite(PIN_LUZ, estadoLuz ? LOW : HIGH); // Lógica inversa

  // --- Retorno automático de control manual (30 min anti-olvidos) ---
  if (controlExt != C_AUTO &&
      millis() - inicioControlExt > DURACION_CONTROL_MANUAL) {
    controlExt = C_AUTO;
  }
  if (controlVent != C_AUTO &&
      millis() - inicioControlVent > DURACION_CONTROL_MANUAL) {
    controlVent = C_AUTO;
  }

  // --- Lógica de Ventilador Interno ---
  if (controlVent == C_ON) {
    digitalWrite(PIN_VENTILADOR, LOW); // Forzado ON
  } else if (controlVent == C_OFF) {
    digitalWrite(PIN_VENTILADOR, HIGH); // Forzado OFF
  } else {
    digitalWrite(PIN_VENTILADOR, LOW); // AUTO (24/7 ON por diseño biológico)
  }

  // --- Lógica de Extracción Inteligente ---
  if (controlExt == C_ON) {
    digitalWrite(PIN_EXTRACTOR, LOW); // Forzado ON
  } else if (controlExt == C_OFF) {
    digitalWrite(PIN_EXTRACTOR, HIGH); // Forzado OFF
  } else {
    // Lógica Automática: Humedad Crítica + Pulso de Respiro + Protección Invierno
    bool tempAlta = (temperatura > p.tempMax);
    bool humAlta = (humedad > p.humMax);
    bool pulsoRespiro = (ahora.hour() % 3 == 0 && ahora.minute() < 5);
    
    // Histéresis: Evitar que el relé prenda y apague por variaciones de 0.1% cuando hace frío
    bool extraccionActiva = (digitalRead(PIN_EXTRACTOR) == LOW);
    bool prioridadHumedad = humAlta || (extraccionActiva && humedad > (p.humMax - 5.0));

    if (temperatura < 18.0 && !prioridadHumedad) {
      // Winter Pulse: 2 min cada hora (protección térmica)
      if (ahora.minute() < 2) {
        digitalWrite(PIN_EXTRACTOR, LOW);
      } else {
        digitalWrite(PIN_EXTRACTOR, HIGH);
      }
    } else {
      if (tempAlta || humAlta || pulsoRespiro) {
        digitalWrite(PIN_EXTRACTOR, LOW);
      } else if (!pulsoRespiro && temperatura < (p.tempMax - 2.0) &&
                 humedad < (p.humMax - 5.0)) {
        digitalWrite(PIN_EXTRACTOR, HIGH);
      }
    }
  }
}

// --- Getters ---
float obtenerTemperatura() { return temperatura; }
float obtenerHumedad() { return humedad; }

float obtenerVPD() {
  if (temperatura < 5.0 || humedad < 5.0)
    return 0.0;
  // VPsat (kPa) = 0.61078 * exp((17.27 * T) / (T + 237.3))
  float vpsat = 0.61078 * exp((17.27 * temperatura) / (temperatura + 237.3));
  // VPD = VPsat * (1 - RH / 100)
  float vpd = vpsat * (1.0 - (humedad / 100.0));
  return vpd;
}
bool obtenerEstadoLuz() { return estadoLuz; }
bool obtenerEstadoExtractor() { return digitalRead(PIN_EXTRACTOR) == LOW; }
bool obtenerEstadoVentilador() { return digitalRead(PIN_VENTILADOR) == LOW; }

// --- Control Ext/Vent ---
void establecerControlVent(ModoControl modo) {
  controlVent = modo;
  if (modo != C_AUTO)
    inicioControlVent = millis();
}

void establecerControlExt(ModoControl modo) {
  controlExt = modo;
  if (modo != C_AUTO)
    inicioControlExt = millis();
}

ModoControl obtenerControlVent() { return controlVent; }
ModoControl obtenerControlExt() { return controlExt; }

uint32_t obtenerTiempoRestanteControlVent() {
  if (controlVent == C_AUTO)
    return 0;
  uint32_t transcurrido = millis() - inicioControlVent;
  return (transcurrido > DURACION_CONTROL_MANUAL)
             ? 0
             : (DURACION_CONTROL_MANUAL - transcurrido);
}

uint32_t obtenerTiempoRestanteControlExt() {
  if (controlExt == C_AUTO)
    return 0;
  uint32_t transcurrido = millis() - inicioControlExt;
  return (transcurrido > DURACION_CONTROL_MANUAL)
             ? 0
             : (DURACION_CONTROL_MANUAL - transcurrido);
}

const char *nombreModoControl(ModoControl modo) {
  switch (modo) {
  case C_ON:
    return "ON";
  case C_OFF:
    return "OFF";
  default:
    return "AUTO";
  }
}
