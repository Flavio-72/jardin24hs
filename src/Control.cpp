#include "Control.h"

DHT dht(DHTPIN, DHTTYPE);
float temperatura = 0.0;
float humedad = 0.0;
bool estadoLuz = false;
uint32_t ultimaLectura = 0;

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

  // --- Lógica de Extracción Inteligente ---
  if (temperatura < 18.0) {
    // Protocolo de Invierno (Winter Pulse): Aislar el cultivo del frío,
    // purgando humedad pesada y renovando CO2 únicamente durante los 2 primeros minutos de cada hora.
    if (ahora.minute() < 2) {
      digitalWrite(PIN_EXTRACTOR, LOW);  // Encender extractor (Pulso Activo)
    } else {
      digitalWrite(PIN_EXTRACTOR, HIGH); // Apagar extractor (Hibernando)
    }
  } else {
    // Rango Biológico Seguro (Gestión VPD + Pulso de Respiro): 
    // Combate cualquier exceso de Calor/Humedad Y asegura CO2 cada 3 horas.
    bool tempAlta = (temperatura > p.tempMax);
    bool humAlta = (humedad > p.humMax);
    bool pulsoRespiro = (ahora.hour() % 3 == 0 && ahora.minute() < 5);
    
    if (tempAlta || humAlta || pulsoRespiro) {
      digitalWrite(PIN_EXTRACTOR, LOW); // Encender extractor
    } 
    // Histéresis dual: se debe normalizar AMBAS variables para dejar de extraer.
    // Solo apagamos si el pulso de respiro no está activo.
    else if (!pulsoRespiro && temperatura < (p.tempMax - 2.0) && humedad < (p.humMax - 5.0)) { 
      digitalWrite(PIN_EXTRACTOR, HIGH); // Apagar extractor
    }
  }
}

float obtenerTemperatura() { return temperatura; }
float obtenerHumedad() { return humedad; }
bool obtenerEstadoLuz() { return estadoLuz; }
bool obtenerEstadoExtractor() { return digitalRead(PIN_EXTRACTOR) == LOW; }
