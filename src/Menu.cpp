#include "Menu.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
EstadoSistema estadoActual = MONITOREO;
int ultimoBoton = -1;
uint32_t ultimaActualizacion = 0;
uint32_t ultimaActividad = 0;
#define TIEMPO_INACTIVIDAD 10000 // 10 segundos para volver al inicio
#define TIEMPO_BACKLIGHT   180000 // 3 minutos para apagar backlight

// Variables temporales para edición y UI
int eHora = 0, eMin = 0, eDia = 1, eMes = 1, eAnio = 2024;
int eDiasCiclo = 0;
uint32_t inicioPresion = 0;
uint32_t ultimaRepeticion = 0;
bool botonMantenido = false;
bool estadoParpadeo = true;
uint32_t ultimoParpadeo = 0;
#define BTN_DERECHA  0
#define BTN_ARRIBA   1
#define BTN_ABAJO    2
#define BTN_IZQUIERDA 3
#define BTN_SELECT   4
#define BTN_NINGUNO  5

int eventoBoton = BTN_NINGUNO; // Guarda qué botón disparó el evento
int pantallaMonitoreo = 0; // 0 = Principal (Hora/Temp), 1 = Secundaria (Modo/Luz)


enum TipoEvento {
  EV_NINGUNO,
  EV_CLIC,
  EV_CLIC_LARGO,
  EV_MANTENIENDO
};

enum CampoEdicion {
  CAMPO_HORA_RTC,
  CAMPO_MIN_RTC,
  CAMPO_DIA_RTC,
  CAMPO_MES_RTC,
  CAMPO_ANIO_RTC,
  CAMPO_MODO,
  CAMPO_DIAS_CICLO,
  CAMPO_LUZ_ON,
  CAMPO_VENT_MANUAL,
  CAMPO_EXT_MANUAL,
  TOTAL_CAMPOS
};

CampoEdicion campoActual = CAMPO_HORA_RTC;
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

TipoEvento leerEventoBoton() {
  int botonActual = leerBoton();
  uint32_t ahora = millis();

  if (botonActual != BTN_NINGUNO) {
    // Si es un botón nuevo o cambió el botón sin ser soltado
    if (ultimoBoton == BTN_NINGUNO || botonActual != ultimoBoton) {
      inicioPresion = ahora;
      ultimoBoton = botonActual;
      botonMantenido = false;
      return EV_NINGUNO;
    }

    uint32_t duracion = ahora - inicioPresion;

    // Detectar Long Press inicial (para entrar a menú)
    if (duracion > 1500 && !botonMantenido) {
      botonMantenido = true;
      eventoBoton = ultimoBoton;
      return EV_CLIC_LARGO;
    }

    // Detectar Auto-repetición (para flechas subir/bajar)
    if (duracion > 500) { // Esperar 500ms antes de empezar a repetir
      if (ahora - ultimaRepeticion > 150) { // Repite más rápido (~6.6 pasos por segundo)
        ultimaRepeticion = ahora;
        eventoBoton = ultimoBoton;
        return EV_MANTENIENDO;
      }
    }
  } else {
    // Si se suelta el botón
    if (ultimoBoton != BTN_NINGUNO) {
      uint32_t duracion = ahora - inicioPresion;
      eventoBoton = ultimoBoton;
      ultimoBoton = BTN_NINGUNO;
      if (duracion < 1500 && !botonMantenido) {
        return EV_CLIC;
      }
    }
  }
  return EV_NINGUNO;
}

void inicializarMenu() {
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH); // Encender Backlight
  lcd.begin(16, 2);
  lcd.print("Jardin 24hs v1.0.8");
  delay(1000);
  lcd.clear();
  ultimaActividad = millis();
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
    
    static uint32_t ultimoCambio = 0;
    static int subPantalla = 0;
    
    // Cambiamos de mensaje cada 3000ms (3 segundos)
    if (millis() - ultimoCambio > 3000) {
      subPantalla = (subPantalla + 1) % 3; 
      ultimoCambio = millis();
    }
    
    char msg[17];
    if (subPantalla == 0) {
      float t = obtenerTemperatura();
      int t_int = (int)t;
      int t_dec = abs((int)(t * 10) % 10);
      sprintf(msg, "Temp. :%d.%dC", t_int, t_dec);
    } else if (subPantalla == 1) {
      int h = (int)obtenerHumedad();
      sprintf(msg, "Humedad: %d %%", h);
    } else {
      int dias = 0;
      if (config.inicioCicloUnix == 0 && ahora.year() > 2020) {
        config.inicioCicloUnix = ahora.unixtime();
        guardarConfiguracion();
      }
      if (ahora.unixtime() >= config.inicioCicloUnix) {
        dias = (ahora.unixtime() - config.inicioCicloUnix) / 86400;
      }
      sprintf(msg, "%s Dia %d", config.modoActual == CRECIMIENTO ? "Vege" : "Flora", dias);
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
    int dias = 0;
    if (config.inicioCicloUnix == 0 && ahora.year() > 2020) {
      config.inicioCicloUnix = ahora.unixtime();
      guardarConfiguracion();
    }
    if (ahora.unixtime() >= config.inicioCicloUnix) {
      dias = (ahora.unixtime() - config.inicioCicloUnix) / 86400;
    }
    char buf[17];
    sprintf(buf, "%s - Dia %d", config.modoActual == CRECIMIENTO ? "Vege" : "Flora", dias);
    for(int i=strlen(buf); i<16; i++) buf[i] = ' ';
    buf[16] = '\0';

    lcd.setCursor(0, 0);
    lcd.print(buf);
    lcd.setCursor(0, 1);
    lcd.print(obtenerEstadoLuz() ? "Luz: On         " : "Luz: Off        ");
  }
}

void mostrarEditCampo() {
  lcd.setCursor(0, 0);
  switch (campoActual) {
    case CAMPO_HORA_RTC: lcd.print("Config Hora    "); break;
    case CAMPO_MIN_RTC:  lcd.print("Config Minutos "); break;
    case CAMPO_DIA_RTC:  lcd.print("Config Dia     "); break;
    case CAMPO_MES_RTC:  lcd.print("Config Mes     "); break;
    case CAMPO_ANIO_RTC: lcd.print("Config Anio    "); break;
    case CAMPO_MODO:     lcd.print("Config Modo    "); break;
    case CAMPO_DIAS_CICLO: lcd.print("Dias del Ciclo "); break;
    case CAMPO_LUZ_ON:   lcd.print("Config Luz On  "); break;
    case CAMPO_VENT_MANUAL: lcd.print("Vent. Manual   "); break;
    case CAMPO_EXT_MANUAL:  lcd.print("Extr. Manual   "); break;
    default: break;
  }

  lcd.setCursor(0, 1);
  lcd.print("> ");
  
  if (!estadoParpadeo) {
    switch (campoActual) {
      case CAMPO_HORA_RTC: lcd.print("__"); break;
      case CAMPO_MIN_RTC:  lcd.print("__"); break;
      case CAMPO_DIA_RTC:  lcd.print("__"); break;
      case CAMPO_MES_RTC:  lcd.print("__"); break;
      case CAMPO_ANIO_RTC: lcd.print("____"); break;
      case CAMPO_MODO:     lcd.print("_____"); break;
      case CAMPO_DIAS_CICLO: lcd.print("___"); break;
      case CAMPO_LUZ_ON:   lcd.print("__:00"); break;
      default:           lcd.print("__"); break;
    }
  } else {
    PerfilCultivo& p = obtenerPerfilActual();
    switch (campoActual) {
      case CAMPO_HORA_RTC: 
        if (eHora < 10) lcd.print('0');
        lcd.print(eHora); break;
      case CAMPO_MIN_RTC:
        if (eMin < 10) lcd.print('0');
        lcd.print(eMin); break;
      case CAMPO_DIA_RTC:
        if (eDia < 10) lcd.print('0');
        lcd.print(eDia); break;
      case CAMPO_MES_RTC:
        if (eMes < 10) lcd.print('0');
        lcd.print(eMes); break;
      case CAMPO_ANIO_RTC:
        lcd.print(eAnio); break;
      case CAMPO_MODO:
        lcd.print(config.modoActual == CRECIMIENTO ? "Vege  " : "Flora "); break;
      case CAMPO_DIAS_CICLO:
        if (eDiasCiclo < 10) lcd.print("00");
        else if (eDiasCiclo < 100) lcd.print('0');
        lcd.print(eDiasCiclo); break;
      case CAMPO_LUZ_ON:
        if (p.horaOn < 10) lcd.print('0');
        lcd.print(p.horaOn); lcd.print(":00"); break;
      case CAMPO_VENT_MANUAL: {
        ModoManual m = obtenerModoManualVent();
        if (m == M_AUTO) lcd.print("[AUTO]   ");
        else {
          lcd.print(m == M_ON ? "[ON] " : "[OFF]");
          lcd.print(obtenerTiempoRestanteManualVent() / 60000); lcd.print("m");
        }
        break;
      }
      case CAMPO_EXT_MANUAL: {
        ModoManual m = obtenerModoManualExt();
        if (m == M_AUTO) lcd.print("[AUTO]   ");
        else {
          lcd.print(m == M_ON ? "[ON] " : "[OFF]");
          lcd.print(obtenerTiempoRestanteManualExt() / 60000); lcd.print("m");
        }
        break;
      }
      default: break;
    }
  }
  lcd.print("          ");
}

void actualizarMenu() {
  TipoEvento evento = leerEventoBoton();

  if (evento != EV_NINGUNO) {
    ultimaActividad = millis();
    digitalWrite(10, HIGH); // Encender backlight ante cualquier evento
  }

  // Apagado automático de backlight por inactividad (3 min)
  if (millis() - ultimaActividad > TIEMPO_BACKLIGHT) {
    digitalWrite(10, LOW);
  }

  // Vuelta automática por inactividad
  if (millis() - ultimaActividad > TIEMPO_INACTIVIDAD) {
    if (modoEdicion || pantallaMonitoreo != 0) {
      modoEdicion = false;
      pantallaMonitoreo = 0;
      lcd.clear();
    }
  }

  uint32_t tiempoBlink = estadoParpadeo ? 600 : 200; // Encendido 600ms, apagado 200ms
  if (millis() - ultimoParpadeo > tiempoBlink) {
    estadoParpadeo = !estadoParpadeo;
    ultimoParpadeo = millis();
  }

  if (!modoEdicion) {
    mostrarMonitoreo();
    
    // Navegación de pantallas de monitoreo
    if (evento == EV_CLIC) {
      if (eventoBoton == BTN_DERECHA) {
        pantallaMonitoreo = (pantallaMonitoreo + 1) % 2;
        lcd.clear();
      }
      if (eventoBoton == BTN_IZQUIERDA) {
        pantallaMonitoreo = (pantallaMonitoreo == 0) ? 1 : pantallaMonitoreo - 1;
        lcd.clear();
      }
    }

    if (evento == EV_CLIC_LARGO && eventoBoton == BTN_SELECT) {
      modoEdicion = true;
      campoActual = CAMPO_HORA_RTC;
      DateTime ahora = rtc.now();
      eHora = ahora.hour();
      eMin = ahora.minute();
      eDia = ahora.day();
      eMes = ahora.month();
      eAnio = ahora.year();
      if (ahora.unixtime() >= config.inicioCicloUnix && config.inicioCicloUnix != 0) {
        eDiasCiclo = (ahora.unixtime() - config.inicioCicloUnix) / 86400;
      } else {
        eDiasCiclo = 0;
      }
      lcd.clear();
      delay(200);
    }
  } else {
    mostrarEditCampo();
    
    // Navegación Izquierda/Derecha
    if (evento == EV_CLIC) {
      if (eventoBoton == BTN_DERECHA) {
        campoActual = (CampoEdicion)((campoActual + 1) % TOTAL_CAMPOS);
        lcd.clear();
      }
      if (eventoBoton == BTN_IZQUIERDA) {
        campoActual = (CampoEdicion)((campoActual == 0) ? TOTAL_CAMPOS - 1 : campoActual - 1);
        lcd.clear();
      }
    }

    // Ajuste de valores Arriba/Abajo
    if (evento == EV_CLIC || evento == EV_MANTENIENDO) {
      PerfilCultivo& p = obtenerPerfilActual();
      if (eventoBoton == BTN_ARRIBA) {
        switch (campoActual) {
          case CAMPO_HORA_RTC: eHora = (eHora + 1) % 24; break;
          case CAMPO_MIN_RTC:  eMin = (eMin + 1) % 60; break;
          case CAMPO_DIA_RTC:  eDia = (eDia % 31) + 1; break;
          case CAMPO_MES_RTC:  eMes = (eMes % 12) + 1; break;
          case CAMPO_ANIO_RTC: eAnio++; if(eAnio > 2099) eAnio = 2024; break;
          case CAMPO_MODO:     
            config.modoActual = (config.modoActual == CRECIMIENTO) ? FLORACION : CRECIMIENTO; 
            eDiasCiclo = 0; // Reiniciar días si cambiamos de modo
            break;
          case CAMPO_DIAS_CICLO: eDiasCiclo = (eDiasCiclo + 1) % 999; break;
          case CAMPO_LUZ_ON:   p.horaOn = (p.horaOn + 1) % 24; break;
          case CAMPO_VENT_MANUAL: {
            ModoManual sig = (ModoManual)((obtenerModoManualVent() + 1) % 3);
            establecerVentiladorManual(sig);
            break;
          }
          case CAMPO_EXT_MANUAL: {
            ModoManual sig = (ModoManual)((obtenerModoManualExt() + 1) % 3);
            establecerExtractorManual(sig);
            break;
          }
          default: break;
        }
      }
      if (eventoBoton == BTN_ABAJO) {
        switch (campoActual) {
          case CAMPO_HORA_RTC: eHora = (eHora == 0) ? 23 : eHora - 1; break;
          case CAMPO_MIN_RTC:  eMin = (eMin == 0) ? 59 : eMin - 1; break;
          case CAMPO_DIA_RTC:  eDia = (eDia == 1) ? 31 : eDia - 1; break;
          case CAMPO_MES_RTC:  eMes = (eMes == 1) ? 12 : eMes - 1; break;
          case CAMPO_ANIO_RTC: eAnio--; if(eAnio < 2024) eAnio = 2099; break;
          case CAMPO_MODO:     
            config.modoActual = (config.modoActual == CRECIMIENTO) ? FLORACION : CRECIMIENTO; 
            eDiasCiclo = 0; // Reiniciar días si cambiamos de modo
            break;
          case CAMPO_DIAS_CICLO: eDiasCiclo = (eDiasCiclo == 0) ? 999 : eDiasCiclo - 1; break;
          case CAMPO_LUZ_ON:   p.horaOn = (p.horaOn == 0) ? 23 : p.horaOn - 1; break;
          case CAMPO_VENT_MANUAL: {
            ModoManual act = obtenerModoManualVent();
            ModoManual sig = (act == M_AUTO) ? M_OFF : (ModoManual)(act - 1);
            establecerVentiladorManual(sig);
            break;
          }
          case CAMPO_EXT_MANUAL: {
            ModoManual act = obtenerModoManualExt();
            ModoManual sig = (act == M_AUTO) ? M_OFF : (ModoManual)(act - 1);
            establecerExtractorManual(sig);
            break;
          }
          default: break;
        }
      }
    }

    // Guardar y Salir (Solo un clic corto en SELECT)
    if (evento == EV_CLIC && eventoBoton == BTN_SELECT) {
      // Configuramos el RTC local
      rtc.adjust(DateTime(eAnio, eMes, eDia, eHora, eMin, 0));
      
      // Auto-calcular horaOff según CBD Photoperiods (Experto)
      PerfilCultivo& p = obtenerPerfilActual();
      if (config.modoActual == CRECIMIENTO) {
        p.horaOff = (p.horaOn + 18) % 24; // 18hs Luz
      } else {
        p.horaOff = (p.horaOn + 12) % 24; // 12hs Luz
      }
      
      // Ajustar fecha inicial del ciclo según el día ingresado manualmente
      config.inicioCicloUnix = DateTime(eAnio, eMes, eDia, eHora, eMin, 0).unixtime() - (eDiasCiclo * 86400UL);

      guardarConfiguracion();
      modoEdicion = false;
      lcd.clear();
      lcd.print("Guardado!");
      delay(1000);
      lcd.clear();
    }
  }
}
