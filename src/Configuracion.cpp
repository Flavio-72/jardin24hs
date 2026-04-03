#include "Configuracion.h"

ConfigApp config;

void cargarConfig() {
  EEPROM.get(0, config);
  
  if (config.magic != CONFIG_MAGIC) {
    // Valores por defecto lógicos para el usuario (reloj)
    config.vege = {6, 0, 0, 0, 25.0, 60.0};   // 18h luz: Enciende 06:00 / Apaga 00:00
    config.flora = {6, 0, 18, 0, 23.0, 50.0}; // 12h luz: Enciende 06:00 / Apaga 18:00
    config.modoActual = CRECIMIENTO;
    config.magic = CONFIG_MAGIC;
    guardarConfig();
  }
}

void guardarConfig() {
  EEPROM.put(0, config);
}

Perfil& getPerfilActual() {
  return (config.modoActual == CRECIMIENTO) ? config.vege : config.flora;
}
