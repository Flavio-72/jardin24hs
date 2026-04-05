#include "Control.h"

DHT dht(DHTPIN, DHTTYPE);
float temperatura = 0.0;
float humedad = 0.0;
bool estadoLuz = false;
uint32_t ultimaLectura = 0;

// Variables de Control Manual
ModoManual modoManualVent = M_AUTO;
ModoManual modoManualExt = M_AUTO;
uint32_t inicioManualVent = 0;
uint32_t inicioManualExt = 0;
const uint32_t DURACION_MANUAL = 1800000; // 30 minutos (1800s)

void prepararControl() {
  dht.begin();
  pinMode(PIN_LUZ, OUTPUT);
  pinMode(PIN_EXTRACTOR, OUTPUT);
  pinMode(PIN_VENTILADOR_INT, OUTPUT);
  
  // Condición Inicial Segura
  digitalWrite(PIN_LUZ, HIGH); // Apagado
  digitalWrite(PIN_EXTRACTOR, HIGH); // Apagado
  digitalWrite(PIN_VENTILADOR_INT, LOW); // Encendido 24/7 (Lógica inversa)
}

void actualizarControl() {
  // Leer sensores cada 2 segundos
  if (millis() - ultimaLectura > 2000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    if (!isnan(t)) temperatura = t;
    if (!isnan(h)) humedad = h;
    
    ultimaLectura = millis();
  }

  PerfilCultivo& p = obtenerPerfilActual();
  DateTime ahora = rtc.now();
  int minActuales = ahora.hour() * 60 + ahora.minute();
  int minOn = p.horaOn * 60 + p.minOn;
  int minOff = p.horaOff * 60 + p.minOff;

  // Lógica de Luz
  if (minOn < minOff) {
    estadoLuz = (minActuales >= minOn && minActuales < minOff);
  } else { // Cruce de medianoche
    estadoLuz = (minActuales >= minOn || minActuales < minOff);
  }

  digitalWrite(PIN_LUZ, estadoLuz ? LOW : HIGH); // Lógica inversa del relé

  // --- Lógica de Ventilador Interno ---
  if (modoManualVent == M_ON) {
    digitalWrite(PIN_VENTILADOR_INT, LOW); // Forzado ON
  } else if (modoManualVent == M_OFF) {
    digitalWrite(PIN_VENTILADOR_INT, HIGH); // Forzado OFF
  } else {
    digitalWrite(PIN_VENTILADOR_INT, LOW); // AUTO (24/7 ON por diseño)
  }

  // --- Lógica de Extracción Inteligente ---
  // Primero procesamos el retorno automático si expiró el tiempo
  if (modoManualExt != M_AUTO && millis() - inicioManualExt > DURACION_MANUAL) {
    modoManualExt = M_AUTO;
  }
  if (modoManualVent != M_AUTO && millis() - inicioManualVent > DURACION_MANUAL) {
    modoManualVent = M_AUTO;
  }

  if (modoManualExt == M_ON) {
    digitalWrite(PIN_EXTRACTOR, LOW); // Forzado ON
  } else if (modoManualExt == M_OFF) {
    digitalWrite(PIN_EXTRACTOR, HIGH); // Forzado OFF
  } else {
    // Lógica Automática (VPD + Pulso de Respiro + Invierno)
    if (temperatura < 18.0) {
      if (ahora.minute() < 2) {
        digitalWrite(PIN_EXTRACTOR, LOW);
      } else {
        digitalWrite(PIN_EXTRACTOR, HIGH);
      }
    } else {
      bool tempAlta = (temperatura > p.tempMax);
      bool humAlta = (humedad > p.humMax);
      bool pulsoRespiro = (ahora.hour() % 3 == 0 && ahora.minute() < 5);
      
      if (tempAlta || humAlta || pulsoRespiro) {
        digitalWrite(PIN_EXTRACTOR, LOW);
      } 
      else if (!pulsoRespiro && temperatura < (p.tempMax - 2.0) && humedad < (p.humMax - 5.0)) { 
        digitalWrite(PIN_EXTRACTOR, HIGH);
      }
    }
  }
}

float obtenerTemperatura() { return temperatura; }
float obtenerHumedad() { return humedad; }
bool obtenerEstadoLuz() { return estadoLuz; }
bool obtenerEstadoExtractor() { return digitalRead(PIN_EXTRACTOR) == LOW; }

void establecerVentiladorManual(ModoManual modo) { 
  modoManualVent = modo; 
  if (modo != M_AUTO) inicioManualVent = millis();
}
void establecerExtractorManual(ModoManual modo) { 
  modoManualExt = modo; 
  if (modo != M_AUTO) inicioManualExt = millis();
}
ModoManual obtenerModoManualVent() { return modoManualVent; }
ModoManual obtenerModoManualExt() { return modoManualExt; }

uint32_t obtenerTiempoRestanteManualVent() {
  if (modoManualVent == M_AUTO) return 0;
  uint32_t transcurrido = millis() - inicioManualVent;
  return (transcurrido > DURACION_MANUAL) ? 0 : (DURACION_MANUAL - transcurrido);
}

uint32_t obtenerTiempoRestanteManualExt() {
  if (modoManualExt == M_AUTO) return 0;
  uint32_t transcurrido = millis() - inicioManualExt;
  return (transcurrido > DURACION_MANUAL) ? 0 : (DURACION_MANUAL - transcurrido);
}
