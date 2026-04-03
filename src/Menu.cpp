#include "Menu.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
EstadoSistema estadoActual = MONITOREO;
int ultimoBoton = -1;
uint32_t lastUpdate = 0;

// Variables temporales para edición y UI
int tempValInt = 0;
float tempValFloat = 0.0;
uint32_t pressStart = 0;
uint32_t lastRepeat = 0;
bool buttonHeld = false;
bool blinkState = true;
uint32_t lastBlink = 0;
#define BTN_DERECHA  0
#define BTN_ARRIBA   1
#define BTN_ABAJO    2
#define BTN_IZQUIERDA 3
#define BTN_SELECT   4
#define BTN_NINGUNO  5

int botonEvento = BTN_NINGUNO; // Guarda qué botón disparó el evento

enum EventoBoton {
  EV_NINGUNO,
  EV_CLICK,
  EV_LONG_CLICK,
  EV_HOLDING
};

enum CampoEdicion {
  FLD_HORA_RTC,
  FLD_MIN_RTC,
  FLD_MODO,
  FLD_TEMP_MAX,
  FLD_HUM_MIN,
  FLD_LUZ_ON,
  FLD_LUZ_OFF,
  NUM_FIELDS
};

CampoEdicion campoActual = FLD_HORA_RTC;
bool modoEdicion = false;

#define PIN_LUZ 13
#define PIN_EXTRACTOR 11
#define PIN_HUMIDIFICADOR 12

int leerBoton() {
  int valor = analogRead(A0);
  if (valor < 50)   return BTN_DERECHA;
  if (valor < 195)  return BTN_ARRIBA;
  if (valor < 380)  return BTN_ABAJO;
  if (valor < 555)  return BTN_IZQUIERDA;
  if (valor < 790)  return BTN_SELECT;
  return BTN_NINGUNO;
}

EventoBoton leerEventoBoton() {
  int botonActual = leerBoton();
  uint32_t ahora = millis();

  if (botonActual != BTN_NINGUNO) {
    // Si es un botón nuevo o cambió el botón sin ser soltado
    if (ultimoBoton == BTN_NINGUNO || botonActual != ultimoBoton) {
      pressStart = ahora;
      ultimoBoton = botonActual;
      buttonHeld = false;
      return EV_NINGUNO;
    }

    uint32_t duracion = ahora - pressStart;

    // Detectar Long Press inicial (para entrar a menú)
    if (duracion > 1500 && !buttonHeld) {
      buttonHeld = true;
      botonEvento = ultimoBoton;
      return EV_LONG_CLICK;
    }

    // Detectar Auto-repetición (para flechas subir/bajar)
    if (duracion > 600) { // Esperar 600ms antes de empezar a repetir
      if (ahora - lastRepeat > 333) { // 3 pasos por segundo
        lastRepeat = ahora;
        botonEvento = ultimoBoton;
        return EV_HOLDING;
      }
    }
  } else {
    // Si se suelta el botón
    if (ultimoBoton != BTN_NINGUNO) {
      uint32_t duracion = ahora - pressStart;
      botonEvento = ultimoBoton;
      ultimoBoton = BTN_NINGUNO;
      if (duracion < 1500 && !buttonHeld) {
        return EV_CLICK;
      }
    }
  }
  return EV_NINGUNO;
}

void inicializarMenu() {
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH); // Encender Backlight
  lcd.begin(16, 2);
  lcd.print("Microclima v0.4");
  delay(1000);
  lcd.clear();
}

void mostrarMonitoreo() {
  DateTime ahora = rtc.now();
  lcd.setCursor(0, 0);
  if (ahora.hour() < 10) lcd.print('0');
  lcd.print(ahora.hour()); lcd.print(':');
  if (ahora.minute() < 10) lcd.print('0');
  lcd.print(ahora.minute());
  
  lcd.print(" T:"); lcd.print(getTemperatura(), 1);
  lcd.print(" H:"); lcd.print(getHumedad(), 0);
  
  lcd.setCursor(0, 1);
  lcd.print(config.modoActual == CRECIMIENTO ? "VEGE " : "FLORA ");
  lcd.print(getEstadoLuz() ? "LUZ:ON " : "LUZ:OFF");
}

void mostrarEditCampo() {
  lcd.setCursor(0, 0);
  switch (campoActual) {
    case FLD_HORA_RTC: lcd.print("[HORA RTC]     "); break;
    case FLD_MIN_RTC:  lcd.print("[MINUTOS RTC]  "); break;
    case FLD_MODO:     lcd.print("[MODO CULTIVO] "); break;
    case FLD_TEMP_MAX: lcd.print("[T.MAX VEGE]   "); break;
    case FLD_HUM_MIN:  lcd.print("[H.MIN VEGE]   "); break;
    case FLD_LUZ_ON:   lcd.print("[LUZ ENCENDID] "); break;
    case FLD_LUZ_OFF:  lcd.print("[LUZ APAGADO]  "); break;
    default: break;
  }

  lcd.setCursor(0, 1);
  lcd.print("> ");
  
  if (!blinkState) {
    lcd.print("      "); // Espacio para parpadeo
  } else {
    Perfil& p = getPerfilActual();
    switch (campoActual) {
      case FLD_HORA_RTC: 
        if (tempValInt < 10) lcd.print('0');
        lcd.print(tempValInt); break;
      case FLD_MIN_RTC:
        if ((int)tempValFloat < 10) lcd.print('0');
        lcd.print((int)tempValFloat); break;
      case FLD_MODO:
        lcd.print(config.modoActual == CRECIMIENTO ? "VEGE " : "FLORA "); break;
      case FLD_TEMP_MAX:
        lcd.print(p.tempMax, 1); lcd.print(" C"); break;
      case FLD_HUM_MIN:
        lcd.print(p.humMin, 0); lcd.print(" %"); break;
      case FLD_LUZ_ON:
        if (p.horaOn < 10) lcd.print('0');
        lcd.print(p.horaOn); lcd.print(":00"); break;
      case FLD_LUZ_OFF:
        if (p.horaOff < 10) lcd.print('0');
        lcd.print(p.horaOff); lcd.print(":00"); break;
      default: break;
    }
  }
  lcd.print("          ");
}

void actualizarMenu() {
  EventoBoton evento = leerEventoBoton();

  if (millis() - lastBlink > 350) {
    blinkState = !blinkState;
    lastBlink = millis();
  }

  if (!modoEdicion) {
    mostrarMonitoreo();
    if (evento == EV_LONG_CLICK && botonEvento == BTN_SELECT) {
      modoEdicion = true;
      campoActual = FLD_HORA_RTC;
      DateTime ahora = rtc.now();
      tempValInt = ahora.hour();
      tempValFloat = ahora.minute();
      lcd.clear();
      delay(200);
    }
  } else {
    mostrarEditCampo();
    
    // Navegación Izquierda/Derecha
    if (evento == EV_CLICK) {
      if (botonEvento == BTN_DERECHA) {
        campoActual = (CampoEdicion)((campoActual + 1) % NUM_FIELDS);
        lcd.clear();
      }
      if (botonEvento == BTN_IZQUIERDA) {
        campoActual = (CampoEdicion)((campoActual == 0) ? NUM_FIELDS - 1 : campoActual - 1);
        lcd.clear();
      }
    }

    // Ajuste de valores Arriba/Abajo
    if (evento == EV_CLICK || evento == EV_HOLDING) {
      Perfil& p = getPerfilActual();
      if (botonEvento == BTN_ARRIBA) {
        switch (campoActual) {
          case FLD_HORA_RTC: tempValInt = (tempValInt + 1) % 24; break;
          case FLD_MIN_RTC:  tempValFloat = (int)(tempValFloat + 1) % 60; break;
          case FLD_MODO:     config.modoActual = (config.modoActual == CRECIMIENTO) ? FLORACION : CRECIMIENTO; break;
          case FLD_TEMP_MAX: p.tempMax += 0.5; break;
          case FLD_HUM_MIN:  p.humMin += 1; break;
          case FLD_LUZ_ON:   p.horaOn = (p.horaOn + 1) % 24; break;
          case FLD_LUZ_OFF:  p.horaOff = (p.horaOff + 1) % 24; break;
          default: break;
        }
      }
      if (botonEvento == BTN_ABAJO) {
        switch (campoActual) {
          case FLD_HORA_RTC: tempValInt = (tempValInt == 0) ? 23 : tempValInt - 1; break;
          case FLD_MIN_RTC:  tempValFloat = (int)(tempValFloat == 0) ? 59 : (int)tempValFloat - 1; break;
          case FLD_MODO:     config.modoActual = (config.modoActual == CRECIMIENTO) ? FLORACION : CRECIMIENTO; break;
          case FLD_TEMP_MAX: p.tempMax -= 0.5; break;
          case FLD_HUM_MIN:  p.humMin -= 1; break;
          case FLD_LUZ_ON:   p.horaOn = (p.horaOn == 0) ? 23 : p.horaOn - 1; break;
          case FLD_LUZ_OFF:  p.horaOff = (p.horaOff == 0) ? 23 : p.horaOff - 1; break;
          default: break;
        }
      }
    }

    // Guardar y Salir (Solo un clic corto en SELECT)
    if (evento == EV_CLICK && botonEvento == BTN_SELECT) {
      DateTime ahora = rtc.now();
      rtc.adjust(DateTime(ahora.year(), ahora.month(), ahora.day(), tempValInt, (int)tempValFloat, 0));
      guardarConfig();
      modoEdicion = false;
      lcd.clear();
      lcd.print("Guardado!");
      delay(1000);
      lcd.clear();
    }
  }
}
