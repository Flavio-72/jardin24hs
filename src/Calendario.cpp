#include "Calendario.h"
#include "Configuracion.h"
#include <time.h>

Calendario::Calendario() {}

bool Calendario::begin() {
    if(!LittleFS.begin()){
        Serial.println("Error al montar LittleFS en Calendario");
        return false;
    }
    return true;
}

/**
 * @brief Registra un nuevo evento de cultivo en el sistema de archivos.
 * @param tipo El tipo de acción (riego, fertilizante, etc.)
 * @param ml Cantidad en mililitros.
 * @param nota Comentario opcional.
 * @param plantaID ID de la planta (0-3).
 */
bool Calendario::registrarEvento(String tipo, int ml, String nota) {
    // Sincronizamos con el RTC centralizado del sistema
    DateTime ahora = obtenerHoraActual();
    uint32_t nowUnix = ahora.unixtime();

    // Construir nombre del archivo basado en mes y año del RTC
    char fileName[32];
    snprintf(fileName, sizeof(fileName), "/cal_%04d_%02d.json", ahora.year(), ahora.month());

    JsonDocument doc;
    File file = LittleFS.open(fileName, "r");
    
    if (file) {
        deserializeJson(doc, file);
        file.close();
    }

    // Asegurar que exista el array de eventos
    if (!doc.containsKey("eventos")) {
        doc["eventos"].to<JsonArray>();
    }

    // Crear el nuevo objeto de evento
    JsonObject evento = doc["eventos"].add<JsonObject>();
    evento["fecha"] = nowUnix;
    evento["tipo"] = tipo;
    evento["ml"] = ml;
    evento["nota"] = nota;

    // Guardar el archivo actualizado
    file = LittleFS.open(fileName, "w");
    if (!file) return false;

    serializeJsonPretty(doc, file);
    file.close();

    Serial.printf("Evento registrado: %s (%d ml) en %s\n", tipo.c_str(), ml, fileName);
    return true;
}

/**
 * @brief Recupera los eventos de un mes específico en formato JSON.
 */
String Calendario::obtenerEventosMes(int mes, int anio) {
    char fileName[32];
    snprintf(fileName, sizeof(fileName), "/cal_%04d_%02d.json", anio, mes);

    File file = LittleFS.open(fileName, "r");
    if (!file) return "{\"eventos\":[]}";

    String output;
    while(file.available()){
        output += (char)file.read();
    }
    file.close();
    return output;
}
