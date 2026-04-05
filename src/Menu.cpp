#include "Menu.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
EstadoSistema estadoActual = MONITOREO;
int ultimoBoton = -1;
uint32_t lastUpdate = 0;

// Variables temporales para edición y UI
int eHora = 0, eMin = 0, eDia = 1, eMes = 1, eAnio = 2024;
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
int pantallaMonitoreo = 0; // 0 = Principal (Hora/Temp), 1 = Secundaria (Modo/Luz)


enum EventoBoton {
  EV_NINGUNO,
  EV_CLICK,
  EV_LONG_CLICK,
  EV_HOLDING
};

enum CampoEdicion {
  FLD_HORA_RTC,
  FLD_MIN_RTC,
  FLD_DIA_RTC,
  FLD_MES_RTC,
  FLD_ANIO_RTC,
  FLD_MODO,
  FLD_LUZ_ON,
  NUM_FIELDS
};

CampoEdicion campoActual = FLD_HORA_RTC;
bool modoEdicion = false;

#define PIN_LUZ 13
#define PIN_EXTRACTOR 11
#define PIN_VENTILADOR_INT 12

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
    if (duracion > 500) { // Esperar 500ms antes de empezar a repetir
      if (ahora - lastRepeat > 150) { // Repite más rápido (~6.6 pasos por segundo)
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
  if (pantallaMonitoreo == 0) {
    // Pantalla 0: Hora, Fecha, Temp y Hum
    lcd.setCursor(0, 0);
    if (ahora.hour() < 10) lcd.print('0');
    lcd.print(ahora.hour()); lcd.print(':');
    if (ahora.minute() < 10) lcd.print('0');
    lcd.print(ahora.minute());
    
    lcd.print("  ");
    const char* diasSemana[] = {"dom", "lun", "mar", "mie", "jue", "vie", "sab"};
    // Limitamos por seguridad aunque RTClib retorna 0-6
    lcd.print(diasSemana[ahora.dayOfTheWeek() % 7]);
    lcd.print(" ");
    
    if (ahora.day() < 10) lcd.print('0');
    lcd.print(ahora.day()); lcd.print('/');
    if (ahora.month() < 10) lcd.print('0');
    lcd.print(ahora.month());
    
    lcd.setCursor(0, 1);
    
    static uint32_t lastSwitch = 0;
    static int subPantalla = 0;
    
    // Cambiamos de mensaje cada 3000ms (3 segundos)
    if (millis() - lastSwitch > 3000) {
      subPantalla = (subPantalla + 1) % 2; 
      lastSwitch = millis();
    }
    
    char msg[17];
    if (subPantalla == 0) {
      float t = getTemperatura();
      int t_int = (int)t;
      int t_dec = abs((int)(t * 10) % 10);
      sprintf(msg, "Temp. :%d.%dC", t_int, t_dec);
    } else {
      int h = (int)getHumedad();
      sprintf(msg, "Humedad: %d %%", h);
    }
    
    // Rellenar con espacios en blanco hasta 16 caracteres para limpiar rastros
    int len = strlen(msg);
    for (int i = len; i < 16; i++) {
      msg[i] = ' ';
    }
    msg[16] = '\0'; // Fin de cadena
    
    lcd.print(msg);
  } else if (pantallaMonitoreo == 1) {
    // Pantalla 1: Modo de Cultivo y Estado de Luz
    lcd.setCursor(0, 0);
    lcd.print(config.modoActual == CRECIMIENTO ? "Modo: Vege      " : "Modo: Flora     ");
    lcd.setCursor(0, 1);
    lcd.print(getEstadoLuz() ? "Luz: On         " : "Luz: Off        ");
  }
}

void mostrarEditCampo() {
  lcd.setCursor(0, 0);
  switch (campoActual) {
    case FLD_HORA_RTC: lcd.print("Config Hora    "); break;
    case FLD_MIN_RTC:  lcd.print("Config Minutos "); break;
    case FLD_DIA_RTC:  lcd.print("Config Dia     "); break;
    case FLD_MES_RTC:  lcd.print("Config Mes     "); break;
    case FLD_ANIO_RTC: lcd.print("Config Anio    "); break;
    case FLD_MODO:     lcd.print("Config Modo    "); break;
    case FLD_LUZ_ON:   lcd.print("Config Luz On  "); break;
    default: break;
  }

  lcd.setCursor(0, 1);
  lcd.print("> ");
  
  if (!blinkState) {
    switch (campoActual) {
      case FLD_HORA_RTC: lcd.print("__"); break;
      case FLD_MIN_RTC:  lcd.print("__"); break;
      case FLD_DIA_RTC:  lcd.print("__"); break;
      case FLD_MES_RTC:  lcd.print("__"); break;
      case FLD_ANIO_RTC: lcd.print("____"); break;
      case FLD_MODO:     lcd.print("_____"); break;
      case FLD_LUZ_ON:   lcd.print("__:00"); break;
      default:           lcd.print("__"); break;
    }
  } else {
    Perfil& p = getPerfilActual();
    switch (campoActual) {
      case FLD_HORA_RTC: 
        if (eHora < 10) lcd.print('0');
        lcd.print(eHora); break;
      case FLD_MIN_RTC:
        if (eMin < 10) lcd.print('0');
        lcd.print(eMin); break;
      case FLD_DIA_RTC:
        if (eDia < 10) lcd.print('0');
        lcd.print(eDia); break;
      case FLD_MES_RTC:
        if (eMes < 10) lcd.print('0');
        lcd.print(eMes); break;
      case FLD_ANIO_RTC:
        lcd.print(eAnio); break;
      case FLD_MODO:
        lcd.print(config.modoActual == CRECIMIENTO ? "Vege  " : "Flora "); break;
      case FLD_LUZ_ON:
        if (p.horaOn < 10) lcd.print('0');
        lcd.print(p.horaOn); lcd.print(":00"); break;
      default: break;
    }
  }
  lcd.print("          ");
}

void actualizarMenu() {
  EventoBoton evento = leerEventoBoton();

  uint32_t tiempoBlink = blinkState ? 600 : 200; // Encendido 600ms, apagado 200ms
  if (millis() - lastBlink > tiempoBlink) {
    blinkState = !blinkState;
    lastBlink = millis();
  }

  if (!modoEdicion) {
    mostrarMonitoreo();
    
    // Navegación de pantallas de monitoreo
    if (evento == EV_CLICK) {
      if (botonEvento == BTN_DERECHA) {
        pantallaMonitoreo = (pantallaMonitoreo + 1) % 2;
        lcd.clear();
      }
      if (botonEvento == BTN_IZQUIERDA) {
        pantallaMonitoreo = (pantallaMonitoreo == 0) ? 1 : pantallaMonitoreo - 1;
        lcd.clear();
      }
    }

    if (evento == EV_LONG_CLICK && botonEvento == BTN_SELECT) {
      modoEdicion = true;
      campoActual = FLD_HORA_RTC;
      DateTime ahora = rtc.now();
      eHora = ahora.hour();
      eMin = ahora.minute();
      eDia = ahora.day();
      eMes = ahora.month();
      eAnio = ahora.year();
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
          case FLD_HORA_RTC: eHora = (eHora + 1) % 24; break;
          case FLD_MIN_RTC:  eMin = (eMin + 1) % 60; break;
          case FLD_DIA_RTC:  eDia = (eDia % 31) + 1; break;
          case FLD_MES_RTC:  eMes = (eMes % 12) + 1; break;
          case FLD_ANIO_RTC: eAnio++; if(eAnio > 2099) eAnio = 2024; break;
          case FLD_MODO:     config.modoActual = (config.modoActual == CRECIMIENTO) ? FLORACION : CRECIMIENTO; break;
          case FLD_LUZ_ON:   p.horaOn = (p.horaOn + 1) % 24; break;
          default: break;
        }
      }
      if (botonEvento == BTN_ABAJO) {
        switch (campoActual) {
          case FLD_HORA_RTC: eHora = (eHora == 0) ? 23 : eHora - 1; break;
          case FLD_MIN_RTC:  eMin = (eMin == 0) ? 59 : eMin - 1; break;
          case FLD_DIA_RTC:  eDia = (eDia == 1) ? 31 : eDia - 1; break;
          case FLD_MES_RTC:  eMes = (eMes == 1) ? 12 : eMes - 1; break;
          case FLD_ANIO_RTC: eAnio--; if(eAnio < 2024) eAnio = 2099; break;
          case FLD_MODO:     config.modoActual = (config.modoActual == CRECIMIENTO) ? FLORACION : CRECIMIENTO; break;
          case FLD_LUZ_ON:   p.horaOn = (p.horaOn == 0) ? 23 : p.horaOn - 1; break;
          default: break;
        }
      }
    }

    // Guardar y Salir (Solo un clic corto en SELECT)
    if (evento == EV_CLICK && botonEvento == BTN_SELECT) {
      // Configuramos el RTC local
      rtc.adjust(DateTime(eAnio, eMes, eDia, eHora, eMin, 0));
      
      // Auto-calcular horaOff según CBD Photoperiods (Experto)
      Perfil& p = getPerfilActual();
      if (config.modoActual == CRECIMIENTO) {
        p.horaOff = (p.horaOn + 18) % 24; // 18hs Luz
      } else {
        p.horaOff = (p.horaOn + 12) % 24; // 12hs Luz
      }
      
      guardarConfig();
      modoEdicion = false;
      lcd.clear();
      lcd.print("Guardado!");
      delay(1000);
      lcd.clear();
    }
  }
}
