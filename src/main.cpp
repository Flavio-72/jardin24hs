#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <RTClib.h>

// --- Configuración de Hardware ---
RTC_DS3231 rtc;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// --- Definición de Estados (Máquina de Estados) ---
enum EstadoSistema {
  MONITOREO,
  AJUSTE_HORAS,
  AJUSTE_MINUTOS
};

EstadoSistema estadoActual = MONITOREO;

// Variables para el ajuste de hora
int horasTemp = 0;
int minutosTemp = 0;

// Variables para lectura de botones
int ultimoBoton = -1; 
unsigned long tiempoUltimoBoton = 0;

// --- Definición de Botones ---
#define BTN_DERECHA  0
#define BTN_ARRIBA   1
#define BTN_ABAJO    2
#define BTN_IZQUIERDA 3
#define BTN_SELECT   4
#define BTN_NINGUNO  5

// --- Funciones Auxiliares ---

int leerBoton() {
  int valor = analogRead(A0);
  if (valor < 50)   return BTN_DERECHA;
  if (valor < 195)  return BTN_ARRIBA;
  if (valor < 380)  return BTN_ABAJO;
  if (valor < 555)  return BTN_IZQUIERDA;
  if (valor < 790)  return BTN_SELECT;
  return BTN_NINGUNO;
}

// Devuelve el botón solo en el momento en que se presiona (flanco de subida)
int leerBotonUnico() {
  int botonActual = leerBoton();
  if (botonActual != ultimoBoton) {
    ultimoBoton = botonActual;
    if (botonActual != BTN_NINGUNO) return botonActual;
  }
  return BTN_NINGUNO;
}

String obtenerNombreBoton(int id) {
  switch(id) {
    case BTN_DERECHA:   return "DERECHA  ";
    case BTN_ARRIBA:    return "ARRIBA   ";
    case BTN_ABAJO:     return "ABAJO    ";
    case BTN_IZQUIERDA: return "IZQUIERDA";
    case BTN_SELECT:    return "SELECT   ";
    default:            return "NINGUNO  ";
  }
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

  lcd.print("Microclima v0.1");
  delay(1000);
  lcd.clear();
}

void loop() {
  int boton = leerBotonUnico();
  DateTime ahora = rtc.now();

  switch (estadoActual) {
    
    case MONITOREO:
      // Línea 1: Mostrar Reloj en vivo
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

      // Línea 2: Mostrar botón para feedback
      lcd.setCursor(0, 1);
      lcd.print("Boton: ");
      lcd.print(obtenerNombreBoton(leerBoton())); // Aquí usamos leerBoton() para ver el estado actual

      // Transición al menú de ajuste
      if (boton == BTN_SELECT) {
        horasTemp = ahora.hour();
        minutosTemp = ahora.minute();
        estadoActual = AJUSTE_HORAS;
        lcd.clear();
      }
      break;

    case AJUSTE_HORAS:
      lcd.setCursor(0, 0);
      lcd.print("CONFIGURAR     "); // Padding para borrar "HORA: HH:MM:SS"
      lcd.setCursor(0, 1);
      lcd.print("HORAS: >");
      if (horasTemp < 10) lcd.print('0');
      lcd.print(horasTemp);
      lcd.print(":");
      if (minutosTemp < 10) lcd.print('0');
      lcd.print(minutosTemp);

      if (boton == BTN_ARRIBA) {
        horasTemp = (horasTemp + 1) % 24;
      }
      if (boton == BTN_ABAJO) {
        horasTemp = (horasTemp == 0) ? 23 : horasTemp - 1;
      }
      if (boton == BTN_SELECT) {
        estadoActual = AJUSTE_MINUTOS;
        delay(200); // Debounce manual
      }
      break;

    case AJUSTE_MINUTOS:
      lcd.setCursor(0, 0);
      lcd.print("CONFIGURAR     "); // Padding para borrar "HORA: HH:MM:SS"
      lcd.setCursor(0, 1);
      lcd.print("MINUTOS: ");
      if (horasTemp < 10) lcd.print('0');
      lcd.print(horasTemp);
      lcd.print(":>");
      if (minutosTemp < 10) lcd.print('0');
      lcd.print(minutosTemp);

      if (boton == BTN_ARRIBA) {
        minutosTemp = (minutosTemp + 1) % 60;
      }
      if (boton == BTN_ABAJO) {
        minutosTemp = (minutosTemp == 0) ? 59 : minutosTemp - 1;
      }
      if (boton == BTN_SELECT) {
        // Guardar cambios en el RTC
        rtc.adjust(DateTime(ahora.year(), ahora.month(), ahora.day(), horasTemp, minutosTemp, 0));
        estadoActual = MONITOREO;
        lcd.clear();
        lcd.print("Guardado!");
        delay(1000);
        lcd.clear();
      }
      break;
  }

  delay(50); // Estabilidad de CPU
}