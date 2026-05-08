#include "Calendario.h"
#include "Configuracion.h"

Calendario::Calendario() {}

bool Calendario::begin() {
    if(!LittleFS.begin()){
        Serial.println("Error al montar LittleFS en Calendario");
        return false;
    }
    return true;
}

/**
 * @brief Registra un nuevo evento en formato NDJSON (append-only).
 *        Una línea JSON por evento — sin releer ni reescribir el archivo.
 *        Formato: {"f":<unix>,"t":"<tipo>","ml":<ml>,"n":"<nota>"}
 */
bool Calendario::registrarEvento(String tipo, int ml, String nota, uint32_t fechaUnix) {
    DateTime ahora;
    if (fechaUnix > 0) {
        ahora = DateTime(fechaUnix);
    } else {
        ahora = obtenerHoraActual();
    }

    // Nombre de archivo por mes: /cal_YYYY_MM.ndjson
    char fileName[36];
    snprintf(fileName, sizeof(fileName), "/cal_%04d_%02d.ndjson", ahora.year(), ahora.month());

    // Abrir en modo APPEND — nunca se reescribe el archivo completo
    File file = LittleFS.open(fileName, "a");
    if (!file) {
        Serial.printf("[CAL] Error al abrir %s para escritura\n", fileName);
        return false;
    }

    // Serializar el evento como una sola línea JSON
    JsonDocument doc;
    doc["f"]  = ahora.unixtime();
    doc["t"]  = tipo;
    doc["ml"] = ml;
    doc["n"]  = nota;

    String linea;
    serializeJson(doc, linea);
    file.println(linea);  // println agrega el \n que delimita cada evento
    file.close();

    Serial.printf("[CAL] Evento: %s | %d | %s  →  %s\n", tipo.c_str(), ml, nota.c_str(), fileName);
    return true;
}

/**
 * @brief Lee el archivo NDJSON del mes indicado y devuelve un JSON
 *        con array "eventos" compatible con el frontend.
 *        Convierte: NDJSON en disco → {"eventos":[...]} en memoria
 */
String Calendario::obtenerEventosMes(int mes, int anio) {
    char fileName[36];
    snprintf(fileName, sizeof(fileName), "/cal_%04d_%02d.ndjson", anio, mes);

    File file = LittleFS.open(fileName, "r");
    if (!file) {
        Serial.printf("[CAL] Sin datos para %04d/%02d\n", anio, mes);
        return "{\"eventos\":[]}";
    }

    // Construir la respuesta leyendo línea a línea (eficiente en memoria)
    String result = "{\"eventos\":[";
    bool primero = true;

    while (file.available()) {
        String linea = file.readStringUntil('\n');
        linea.trim();
        if (linea.length() > 2) {  // ignorar líneas vacías o malformadas
            if (!primero) result += ",";
            result += linea;
            primero = false;
        }
    }

    result += "]}";
    file.close();
    return result;
}

/**
 * @brief Obtiene los últimos N eventos del mes actual de forma rápida.
 */
String Calendario::obtenerRecientes(int cantidad) {
    DateTime ahora = obtenerHoraActual();
    char fileName[36];
    snprintf(fileName, sizeof(fileName), "/cal_%04d_%02d.ndjson", ahora.year(), ahora.month());

    File file = LittleFS.open(fileName, "r");
    if (!file) return "{\"eventos\":[]}";

    // Buffer circular para las últimas N líneas
    String lineas[10]; // Máximo 10 para no saturar RAM
    if (cantidad > 10) cantidad = 10;
    
    int total = 0;
    while (file.available()) {
        String l = file.readStringUntil('\n');
        l.trim();
        if (l.length() > 5) {
            lineas[total % cantidad] = l;
            total++;
        }
    }
    file.close();

    String result = "{\"eventos\":[";
    int count = (total > cantidad) ? cantidad : total;
    for (int i = 0; i < count; i++) {
        if (i > 0) result += ",";
        int idx = (total - 1 - i) % cantidad;
        result += lineas[idx];
    }
    result += "]}";
    return result;
}
