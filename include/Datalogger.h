#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <RTClib.h>

/**
 * @brief Clase para manejar el registro histórico de sensores (Temp, Hum, VPD).
 *        Almacena datos en LittleFS en formato CSV para fácil lectura y gráficos.
 */
class Datalogger {
public:
    Datalogger();
    bool begin();
    
    // Registra una muestra actual
    void registrar(float temp, float hum, float vpd);
    
    // Obtiene las últimas N horas de datos en formato JSON para el frontend
    String obtenerHistoricoJSON(int horas = 24);
    
    // Limpia registros antiguos si es necesario (mantenimiento)
    void limpiarAntiguos();

private:
    const char* FILE_PATH = "/sensor_log.csv";
    uint32_t ultimoRegistro = 0;
    const uint32_t INTERVALO_LOG = 30000; // 30 segundos (para debug/monitoreo rápido)
};

extern Datalogger datalogger;

#endif
