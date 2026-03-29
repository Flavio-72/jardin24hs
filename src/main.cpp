#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <RTClib.h>

// Instancias de hardware
RTC_DS3231 rtc;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Función para obtener texto del botón presionado (con espacios para limpiar el LCD)
String obtenerNombreBoton(int valorAnalogo) {
  if (valorAnalogo < 50)   return "DERECHA  ";
  if (valorAnalogo < 195)  return "ARRIBA   ";
  if (valorAnalogo < 380)  return "ABAJO    ";
  if (valorAnalogo < 555)  return "IZQUIERDA";
  if (valorAnalogo < 790)  return "SELECT   ";
  return "NINGUNO  ";
}

void setup() {
  // Inicialización del LCD
  lcd.begin(16, 2);
  
  // Inicialización del bus I2C y el RTC
  if (!rtc.begin()) {
    lcd.setCursor(0, 0);
    lcd.print("Error: Sin RTC  ");
    while (1); // Detener si no hay RTC
  }

  // Si el RTC perdió la hora (ej: cambió la pila), sincroniza con la PC
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.setCursor(0, 0);
  lcd.print("Microclima v0.1");
  delay(1500);
  lcd.clear();
}

void loop() {
  // Obtener tiempo actual
  DateTime ahora = rtc.now();

  // MOSTRAR HORA EN LÍNEA 1
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

  // MOSTRAR BOTÓN EN LÍNEA 2 (Validación de Hardware)
  int lecturaAnaloga = analogRead(A0);
  lcd.setCursor(0, 1);
  lcd.print("Boton: ");
  lcd.print(obtenerNombreBoton(lecturaAnaloga));

  delay(200); // Refresco para estabilidad visual
}