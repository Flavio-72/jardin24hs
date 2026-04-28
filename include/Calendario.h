#ifndef CALENDARIO_H
#define CALENDARIO_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "LittleFS.h"

/**
 * @brief Estructura para representar un evento en el calendario de cultivo.
 * Factorizado para legibilidad y fácil extensión.
 */
struct EventoCultivo {
    String tipo;          // "riego", "fertilizante", "micorrizas", "nota"
    int cantidad_ml;      // Volumen en ml (si aplica)
    String descripcion;   // Notas adicionales del usuario
    uint32_t timestamp;   // Fecha y hora del evento
};

class Calendario {
public:
    Calendario();
    bool begin();
    
    // Gestión de eventos
    bool registrarEvento(String tipo, int ml, String nota);
    String obtenerEventosMes(int mes, int anio);
    
    // Configuración de UX
    const int PASO_VOLUMEN_ML = 100;

private:
    const char* _pathBase = "/cal_"; // Prefijo para archivos mensuales: /cal_2026_04.json
    void compactarArchivos();        // Mantenimiento preventivo de memoria
};

#endif
