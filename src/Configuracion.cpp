#include "Configuracion.h"

ConfiguracionApp config;

void cargarConfiguracion() {
  EEPROM.get(0, config);
  
  if (config.codigoVerificador != CONFIG_MAGIC) {
    // Valores por defecto lógicos para el usuario (reloj)
    config.vege = {6, 0, 0, 0, 25.0, 70.0};   // 18h luz: Enciende 06:00 / Apaga 00:00, Limite VPD Vege
    config.flora = {6, 0, 18, 0, 23.0, 55.0}; // 12h luz: Enciende 06:00 / Apaga 18:00, Limite VPD Flora
    config.modoActual = CRECIMIENTO;
    config.inicioCicloUnix = 0;
    config.codigoVerificador = CONFIG_MAGIC;
    guardarConfiguracion();
  }
}

void guardarConfiguracion() {
  EEPROM.put(0, config);
}

PerfilCultivo& obtenerPerfilActual() {
  return (config.modoActual == CRECIMIENTO) ? config.vege : config.flora;
}
