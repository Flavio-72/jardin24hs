#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal.h>

// Configuración de pines para LCD Keypad Shield (D1 RoBoT)
// RS, Enable, D4, D5, D6, D7
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Función para leer el botón presionado
String obtenerNombreBoton(int valorAnalogo) {
  if (valorAnalogo < 50)   return "DERECHA ";
  if (valorAnalogo < 195)  return "ARRIBA  ";
  if (valorAnalogo < 380)  return "ABAJO   ";
  if (valorAnalogo < 555)  return "IZQUIERDA";
  if (valorAnalogo < 790)  return "SELECT  ";
  return "NINGUNO ";
}

void setup() {
  // Inicializar LCD: 16 columnas y 2 filas
  lcd.begin(16, 2);
  
  // Mensaje de bienvenida
  lcd.setCursor(0, 0);
  lcd.print("Microclima v1.0");
  
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Leer el valor analógico del teclado en A0
  int lecturaAnaloga = analogRead(A0);
  String nombreBoton = obtenerNombreBoton(lecturaAnaloga);

  // Línea fija superior
  lcd.setCursor(0, 0);
  lcd.print("Microclima Uno");

  // Línea dinámica inferior
  lcd.setCursor(0, 1);
  lcd.print("Boton: ");
  lcd.print(nombreBoton);
  
  delay(100); // Pequeña espera para estabilidad
}