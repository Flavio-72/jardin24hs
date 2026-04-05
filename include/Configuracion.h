#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include <Arduino.h>
#include <EEPROM.h>

enum ModoCultivo {
  CRECIMIENTO,
  FLORACION
};

struct PerfilCultivo {
  uint8_t horaOn;
  uint8_t minOn;
  uint8_t horaOff;
  uint8_t minOff;
  float tempMax;
  float humMax;
};

struct ConfiguracionApp {
  PerfilCultivo vege;
  PerfilCultivo flora;
  ModoCultivo modoActual;
  uint32_t inicioCicloUnix;
  uint32_t codigoVerificador; // Para verificar si la EEPROM tiene datos válidos
};

const uint32_t CONFIG_MAGIC = 0xCAFE0004;

extern ConfiguracionApp config;

void cargarConfiguracion();
void guardarConfiguracion();
PerfilCultivo& obtenerPerfilActual();

#endif
