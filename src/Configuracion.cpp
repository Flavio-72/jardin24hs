#include "Configuracion.h"

// ============================================================
// Microclima V2.0 — Persistencia con NVS (Preferences)
// Reemplaza el sistema EEPROM + Magic Number de V1.0
// ============================================================

ConfiguracionApp config;
Preferences prefs;

void cargarConfiguracion() {
  prefs.begin("microclima", true); // true = readonly

  // Modo de cultivo
  config.modoActual = (ModoCultivo)prefs.getUChar("modo", CRECIMIENTO);
  config.inicioCicloUnix = prefs.getULong("inicioCiclo", 0);

  // Perfil Vegetativo
  config.vege.horaOn  = prefs.getUChar("vHoraOn",  PERFIL_VEGE_DEFAULT.horaOn);
  config.vege.minOn   = prefs.getUChar("vMinOn",   PERFIL_VEGE_DEFAULT.minOn);
  config.vege.horaOff = prefs.getUChar("vHoraOff", PERFIL_VEGE_DEFAULT.horaOff);
  config.vege.minOff  = prefs.getUChar("vMinOff",  PERFIL_VEGE_DEFAULT.minOff);
  config.vege.tempMax = prefs.getFloat("vTempMax",  PERFIL_VEGE_DEFAULT.tempMax);
  config.vege.humMax  = prefs.getFloat("vHumMax",   PERFIL_VEGE_DEFAULT.humMax);

  // Perfil Floración
  config.flora.horaOn  = prefs.getUChar("fHoraOn",  PERFIL_FLORA_DEFAULT.horaOn);
  config.flora.minOn   = prefs.getUChar("fMinOn",   PERFIL_FLORA_DEFAULT.minOn);
  config.flora.horaOff = prefs.getUChar("fHoraOff", PERFIL_FLORA_DEFAULT.horaOff);
  config.flora.minOff  = prefs.getUChar("fMinOff",  PERFIL_FLORA_DEFAULT.minOff);
  config.flora.tempMax = prefs.getFloat("fTempMax",  PERFIL_FLORA_DEFAULT.tempMax);
  config.flora.humMax  = prefs.getFloat("fHumMax",   PERFIL_FLORA_DEFAULT.humMax);

  // Perfil Personalizado
  config.personalizado.horaOn  = prefs.getUChar("pHoraOn",  PERFIL_PERS_DEFAULT.horaOn);
  config.personalizado.minOn   = prefs.getUChar("pMinOn",   PERFIL_PERS_DEFAULT.minOn);
  config.personalizado.horaOff = prefs.getUChar("pHoraOff", PERFIL_PERS_DEFAULT.horaOff);
  config.personalizado.minOff  = prefs.getUChar("pMinOff",  PERFIL_PERS_DEFAULT.minOff);
  config.personalizado.tempMax = prefs.getFloat("pTempMax",  PERFIL_PERS_DEFAULT.tempMax);
  config.personalizado.humMax  = prefs.getFloat("pHumMax",   PERFIL_PERS_DEFAULT.humMax);

  prefs.end();
}

void guardarConfiguracion() {
  prefs.begin("microclima", false); // false = readwrite

  prefs.putUChar("modo", (uint8_t)config.modoActual);
  prefs.putULong("inicioCiclo", config.inicioCicloUnix);

  prefs.putUChar("vHoraOn",  config.vege.horaOn);
  prefs.putUChar("vMinOn",   config.vege.minOn);
  prefs.putUChar("vHoraOff", config.vege.horaOff);
  prefs.putUChar("vMinOff",  config.vege.minOff);
  prefs.putFloat("vTempMax",  config.vege.tempMax);
  prefs.putFloat("vHumMax",   config.vege.humMax);

  prefs.putUChar("fHoraOn",  config.flora.horaOn);
  prefs.putUChar("fMinOn",   config.flora.minOn);
  prefs.putUChar("fHoraOff", config.flora.horaOff);
  prefs.putUChar("fMinOff",  config.flora.minOff);
  prefs.putFloat("fTempMax",  config.flora.tempMax);
  prefs.putFloat("fHumMax",   config.flora.humMax);

  prefs.putUChar("pHoraOn",  config.personalizado.horaOn);
  prefs.putUChar("pMinOn",   config.personalizado.minOn);
  prefs.putUChar("pHoraOff", config.personalizado.horaOff);
  prefs.putUChar("pMinOff",  config.personalizado.minOff);
  prefs.putFloat("pTempMax",  config.personalizado.tempMax);
  prefs.putFloat("pHumMax",   config.personalizado.humMax);

  prefs.end();
}

PerfilCultivo& obtenerPerfilActual() {
  if (config.modoActual == CRECIMIENTO) return config.vege;
  if (config.modoActual == FLORACION) return config.flora;
  return config.personalizado;
}
