#include "Control.h"

DHT dht(DHTPIN, DHTTYPE);
float temperatura = 0.0;
float humedad = 0.0;
bool estadoLuz = false;
uint32_t lastRead = 0;

void inicializarControl() {
  dht.begin();
  pinMode(PIN_LUZ, OUTPUT);
  pinMode(PIN_EXTRACTOR, OUTPUT);
  pinMode(PIN_HUMIDIFICADOR, OUTPUT);
  
  // Apagar todo por seguridad al iniciar
  digitalWrite(PIN_LUZ, HIGH); // Asumiendo lógica inversa (relé común)
  digitalWrite(PIN_EXTRACTOR, HIGH);
  digitalWrite(PIN_HUMIDIFICADOR, HIGH);
}

void actualizarControl() {
  // Leer sensores cada 2 segundos
  if (millis() - lastRead > 2000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    if (!isnan(t)) temperatura = t;
    if (!isnan(h)) humedad = h;
    
    lastRead = millis();
  }

  Perfil& p = getPerfilActual();
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

  // Lógica de Extracción (Temperatura)
  if (temperatura > p.tempMax) {
    digitalWrite(PIN_EXTRACTOR, LOW); // Encender extractor
  } else if (temperatura < (p.tempMax - 2)) { // Histéresis de 2 grados
    digitalWrite(PIN_EXTRACTOR, HIGH);
  }

  // Lógica de Humidificación (Humedad)
  if (humedad < p.humMin) {
    digitalWrite(PIN_HUMIDIFICADOR, LOW); // Encender humidificador
  } else if (humedad > (p.humMin + 5)) { // Histéresis de 5%
    digitalWrite(PIN_HUMIDIFICADOR, HIGH);
  }
}

float getTemperatura() { return temperatura; }
float getHumedad() { return humedad; }
bool getEstadoLuz() { return estadoLuz; }
