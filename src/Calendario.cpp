#include "Calendario.h"
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
 */
bool Calendario::registrarEvento(String tipo, int ml, String nota) {
    // Obtenemos el tiempo actual (debería estar sincronizado previamente por NTP o RTC)
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Construir nombre del archivo basado en mes y año
    char fileName[32];
    snprintf(fileName, sizeof(fileName), "/cal_%04d_%02d.json", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1);

    JsonDocument doc;
    File file = LittleFS.open(fileName, "r");
    
    if (file) {
        deserializeJson(doc, file);
        file.close();
    }

    // Crear el nuevo objeto de evento de forma legible
    JsonObject evento = doc["eventos"].add<JsonObject>();
    evento["fecha"] = now;
    evento["tipo"] = tipo;
    evento["ml"] = ml;
    evento["nota"] = nota;

    // Guardar el archivo actualizado (formateado para legibilidad humana si se desea)
    file = LittleFS.open(fileName, "w");
    if (!file) return false;

    serializeJsonPretty(doc, file); // Usamos Pretty para que sea legible al abrir el archivo
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
