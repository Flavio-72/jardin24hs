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
  
  // Apagar todo por seguridad al iniciar
  digitalWrite(PIN_LUZ, HIGH); // Asumiendo lógica inversa (relé común)
  digitalWrite(PIN_EXTRACTOR, HIGH);
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

  // Lógica de Extracción (Temperatura O Humedad)
  bool tempAlta = (temperatura > p.tempMax);
  bool humAlta = (humedad > p.humMax);
  
  // Condición agresiva: con que una variable viole el umbral, se succiona
  if (tempAlta || humAlta) {
    digitalWrite(PIN_EXTRACTOR, LOW); // Encender extractor
  } 
  // Histéresis: Se deben relajar AMBAS variables para detener la extracción.
  // -2° para temp y -5% para hum.
  else if (temperatura < (p.tempMax - 2.0) && humedad < (p.humMax - 5.0)) { 
    digitalWrite(PIN_EXTRACTOR, HIGH); // Apagar
  }
}

float getTemperatura() { return temperatura; }
float getHumedad() { return humedad; }
bool getEstadoLuz() { return estadoLuz; }
