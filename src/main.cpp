#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <RTClib.h>
#include <EEPROM.h>

// --- Configuración de Hardware ---
RTC_DS3231 rtc;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// --- Direcciones EEPROM ---
const int ADDR_MODO = 0;

// --- Definición de Estados (Máquina de Estados) ---
enum EstadoSistema {
  MONITOREO,
  AJUSTE_HORAS,
  AJUSTE_MINUTOS,
  CAMBIAR_MODO
};

enum ModoCultivo {
  CRECIMIENTO,
  FLORACION
};

EstadoSistema estadoActual = MONITOREO;
ModoCultivo modoActual = CRECIMIENTO;

// Variables temporales
int horasTemp = 0;
int minutosTemp = 0;
int ultimoBoton = -1;

// --- Definición de Botones ---
#define BTN_DERECHA  0
#define BTN_ARRIBA   1
#define BTN_ABAJO    2
#define BTN_IZQUIERDA 3
#define BTN_SELECT   4
#define BTN_NINGUNO  5

// --- Funciones de Persistencia (EEPROM) ---

void cargarConfig() {
  int m = EEPROM.read(ADDR_MODO);
  if (m == 255) { // Memoria vacía
    modoActual = CRECIMIENTO;
  } else {
    modoActual = (ModoCultivo)m;
  }
}

void guardarConfig() {
  EEPROM.write(ADDR_MODO, (int)modoActual);
}

// --- Funciones de Hardware ---

int leerBoton() {
  int valor = analogRead(A0);
  if (valor < 50)   return BTN_DERECHA;
  if (valor < 195)  return BTN_ARRIBA;
  if (valor < 380)  return BTN_ABAJO;
  if (valor < 555)  return BTN_IZQUIERDA;
  if (valor < 790)  return BTN_SELECT;
  return BTN_NINGUNO;
}

int leerBotonUnico() {
  int botonActual = leerBoton();
  if (botonActual != ultimoBoton) {
    ultimoBoton = botonActual;
    if (botonActual != BTN_NINGUNO) return botonActual;
  }
  return BTN_NINGUNO;
}

void setup() {
  lcd.begin(16, 2);
  
  if (!rtc.begin()) {
    lcd.print("Error: Sin RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  cargarConfig();

  lcd.print("Microclima v0.3.1");
  delay(1000);
  lcd.clear();
}

void loop() {
  int boton = leerBotonUnico();
  DateTime ahora = rtc.now();

  switch (estadoActual) {
    
    case MONITOREO:
      lcd.setCursor(0, 0);
      lcd.print("HORA: ");
      if (ahora.hour() < 10) lcd.print('0');
      lcd.print(ahora.hour());
      lcd.print(':');
      if (ahora.minute() < 10) lcd.print('0');
      lcd.print(ahora.minute());
      lcd.print(':');
      if (ahora.second() < 10) lcd.print('0');
      lcd.print(ahora.second());

      lcd.setCursor(0, 1);
      lcd.print("MODO: ");
      lcd.print(modoActual == CRECIMIENTO ? "CRECIMIENTO " : "FLORACION   ");

      if (boton == BTN_SELECT) {
        horasTemp = ahora.hour();
        minutosTemp = ahora.minute();
        estadoActual = AJUSTE_HORAS;
        lcd.clear();
      }
      break;

    case AJUSTE_HORAS:
      lcd.setCursor(0, 0);
      lcd.print("CONFIGURAR HORA"); 
      lcd.setCursor(0, 1);
      lcd.print("HORAS: >");
      if (horasTemp < 10) lcd.print('0');
      lcd.print(horasTemp);
      lcd.print(":");
      if (minutosTemp < 10) lcd.print('0');
      lcd.print(minutosTemp);

      if (boton == BTN_ARRIBA) horasTemp = (horasTemp + 1) % 24;
      if (boton == BTN_ABAJO) horasTemp = (horasTemp == 0) ? 23 : horasTemp - 1;
      if (boton == BTN_SELECT) {
        estadoActual = AJUSTE_MINUTOS;
        delay(200);
      }
      break;

    case AJUSTE_MINUTOS:
      lcd.setCursor(0, 0);
      lcd.print("CONFIGURAR HORA");
      lcd.setCursor(0, 1);
      lcd.print("MINUTOS: ");
      if (horasTemp < 10) lcd.print('0');
      lcd.print(horasTemp);
      lcd.print(":>");
      if (minutosTemp < 10) lcd.print('0');
      lcd.print(minutosTemp);

      if (boton == BTN_ARRIBA) minutosTemp = (minutosTemp + 1) % 60;
      if (boton == BTN_ABAJO) minutosTemp = (minutosTemp == 0) ? 59 : minutosTemp - 1;
      if (boton == BTN_SELECT) {
        rtc.adjust(DateTime(ahora.year(), ahora.month(), ahora.day(), horasTemp, minutosTemp, 0));
        estadoActual = CAMBIAR_MODO;
        lcd.clear();
      }
      break;

    case CAMBIAR_MODO:
      lcd.setCursor(0, 0);
      lcd.print("ELEGIR CICLO:");
      lcd.setCursor(0, 1);
      lcd.print(modoActual == CRECIMIENTO ? ">CRECIMIENTO " : ">FLORACION   ");

      if (boton == BTN_ARRIBA || boton == BTN_ABAJO) {
        modoActual = (modoActual == CRECIMIENTO) ? FLORACION : CRECIMIENTO;
      }
      if (boton == BTN_SELECT) {
        guardarConfig();
        estadoActual = MONITOREO;
        lcd.clear();
        lcd.print("Ciclo guardado!");
        delay(1000);
        lcd.clear();
      }
      break;
  }

  delay(50);
}