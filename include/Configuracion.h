#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include <Arduino.h>
#include <EEPROM.h>

enum ModoCultivo {
  CRECIMIENTO,
  FLORACION
};

struct Perfil {
  uint8_t horaOn;
  uint8_t minOn;
  uint8_t horaOff;
  uint8_t minOff;
  float tempMax;
  float humMin;
};

struct ConfigApp {
  Perfil vege;
  Perfil flora;
  ModoCultivo modoActual;
  uint32_t magic; // Para verificar si la EEPROM tiene datos válidos
};

const uint32_t CONFIG_MAGIC = 0xCAFE0002;

extern ConfigApp config;

void cargarConfig();
void guardarConfig();
Perfil& getPerfilActual();

#endif
