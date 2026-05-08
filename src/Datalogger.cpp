#include "Datalogger.h"
#include "Configuracion.h"
#include <ArduinoJson.h>

Datalogger datalogger;

Datalogger::Datalogger() {}

bool Datalogger::begin() {
    // LittleFS ya debería estar iniciado en main.cpp, pero nos aseguramos
    if (!LittleFS.begin()) {
        Serial.println("[LOG] Error montando LittleFS");
        return false;
    }
    return true;
}

void Datalogger::registrar(float temp, float hum, float vpd) {
    // Solo registrar cada INTERVALO_LOG
    if (millis() - ultimoRegistro < INTERVALO_LOG && ultimoRegistro != 0) return;
    
    DateTime ahora = obtenerHoraActual();
    if (ahora.year() < 2024) return; // Esperar a que el RTC tenga hora válida

    char fileName[32];
    snprintf(fileName, sizeof(fileName), "/log_%04d_%02d.csv", ahora.year(), ahora.month());

    File file = LittleFS.open(fileName, "a");
    if (!file) {
        Serial.println("[LOG] Error abriendo log para escritura");
        return;
    }

    // Formato: unix,temp,hum,vpd
    file.printf("%lu,%.1f,%.1f,%.2f\n", ahora.unixtime(), temp, hum, vpd);
    file.close();
    
    ultimoRegistro = millis();
    Serial.printf("[LOG] Guardado: T:%.1f H:%.1f V:%.2f -> %s\n", temp, hum, vpd, fileName);
}

String Datalogger::obtenerHistoricoJSON(int horas) {
    DateTime ahora = obtenerHoraActual();
    uint32_t limite = ahora.unixtime() - (horas * 3600);
    
    JsonDocument doc;
    JsonArray data = doc["data"].to<JsonArray>();

    // Debemos revisar el archivo actual y el anterior por si el límite cruza el mes
    char files[2][32];
    snprintf(files[1], 32, "/log_%04d_%02d.csv", ahora.year(), ahora.month());
    
    // Mes anterior
    int mAnt = ahora.month() - 1;
    int aAnt = ahora.year();
    if (mAnt == 0) { mAnt = 12; aAnt--; }
    snprintf(files[0], 32, "/log_%04d_%02d.csv", aAnt, mAnt);

    for (int i = 0; i < 2; i++) {
        if (!LittleFS.exists(files[i])) continue;
        
        File file = LittleFS.open(files[i], "r");
        if (!file) continue;

        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.length() < 10) continue;

            // Parsear manual para velocidad (unix,t,h,v)
            int firstComma = line.indexOf(',');
            int secondComma = line.indexOf(',', firstComma + 1);
            int thirdComma = line.indexOf(',', secondComma + 1);

            if (firstComma == -1 || secondComma == -1 || thirdComma == -1) continue;

            uint32_t unix = line.substring(0, firstComma).toInt();
            if (unix < limite) continue;

            float t = line.substring(firstComma + 1, secondComma).toFloat();
            float h = line.substring(secondComma + 1, thirdComma).toFloat();
            float v = line.substring(thirdComma + 1).toFloat();

            JsonArray entry = data.add<JsonArray>();
            entry.add(unix);
            entry.add(t);
            entry.add(h);
            entry.add(v);
        }
        file.close();
    }

    String result;
    serializeJson(doc, result);
    return result;
}

void Datalogger::limpiarAntiguos() {
    // TODO: Implementar rotación de archivos si el espacio es crítico
    // Por ahora con 4MB y logs mensuales estamos bien por años.
}
