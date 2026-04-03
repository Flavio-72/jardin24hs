#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <LiquidCrystal.h>
#include "Configuracion.h"
#include "Control.h"

enum EstadoSistema {
  MONITOREO,
  MENU_PRINCIPAL,
  AJUSTE_RTC_HORA,
  AJUSTE_RTC_MINUTO,
  CAMBIAR_MODO,
  CONFIG_PERFIL_TEMP,
  CONFIG_PERFIL_HUM,
  CONFIG_PERFIL_ON_H,
  CONFIG_PERFIL_OFF_H
};

void inicializarMenu();
void actualizarMenu();

#endif
