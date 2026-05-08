# Especificaciones Técnicas — Jardín 24hs (Microclima V2.0)

Este documento detalla la arquitectura de hardware, asignación de pines y lógica de ingeniería del sistema de control ambiental para cultivo automatizado.

## 1. Hardware Principal

- **Controlador:** Waveshare ESP32-S3-Touch-LCD-2.
- **Sensores:**
  - **DHT22:** Sensor de temperatura y humedad ambiente.
  - **RTC DS3231:** Reloj de tiempo real de alta precisión (bus I2C).
- **Interfaz Visual:** Pantalla OLED SSD1306 1.3" (bus I2C).
- **Actuadores:** Módulo de relés para control de potencia.

## 2. Definición de Pines (GPIO)

| Función | Pin ESP32-S3 | Nota |
| :--- | :---: | :--- |
| **DHT22 Data** | GPIO 18 | Pin seguro (evita conflicto TX0) |
| **Relé LUZ** | GPIO 40 | Salida digital (Active High) |
| **Relé EXTRACTOR** | GPIO 41 | Salida digital (Control por temperatura/humedad) |
| **Relé VENTILADOR** | GPIO 42 | Salida digital (Circulación interna) |
| **I2C SDA** | GPIO 10 | Bus compartido (OLED + RTC) |
| **I2C SCL** | GPIO 11 | Bus compartido (OLED + RTC) |

## 3. Lógica de Control Ambiental

### Fotoperiodo (Iluminación)

El control se basa en la hora del RTC comparada con los perfiles configurados:

- **Crecimiento (Vege):** 18 horas de luz (Defecto: 06:00 - 00:00).
- **Floración (Flora):** 12 horas de luz (Defecto: 07:00 - 19:00).
- **Personalizado:** Configurable por el usuario vía web.

### Extracción y Clima

El extractor se activa automáticamente si se cumple cualquiera de estas condiciones:

1. **Temperatura > TempMax:** Umbral definido en el perfil activo.
2. **Humedad > HumMax:** Umbral definido en el perfil activo.
3. **Manual ON:** Forzado desde la interfaz web.

## 4. Conectividad y Almacenamiento

- **WiFi:** El sistema opera en modo **Access Point (AP)**.
  - **SSID:** `Microclima`
  - **Pass:** `micro2025`
  - **IP:** `192.168.4.1`
- **Persistencia:**
  - **Ajustes:** Memoria NVS (librería `Preferences`).
  - **Historial (Datalogger):** Archivos CSV mensuales en LittleFS (`/log_YYYY_MM.csv`).
  - **Calendario (Diario):** Archivos NDJSON mensuales en LittleFS (`/cal_YYYY_MM.ndjson`).

## 5. Almacenamiento de Datos (LittleFS)

El sistema utiliza un sistema de archivos LittleFS (4MB) para almacenamiento histórico:
- **Sensores:** Registra Temperatura, Humedad y VPD cada 10 minutos (ajustable).
- **Diario:** Registro persistente de riegos, fertilizaciones y notas.
- **Gráficos:** El servidor web sirve archivos estáticos (incluyendo Chart.js localmente) para renderizar historiales de 24 horas sin necesidad de internet.

## 6. Interfaz Física (OLED)

La pantalla OLED rota automáticamente cada 3-5 segundos mostrando:
1. **Clima:** Temperatura y Humedad en caracteres grandes.
2. **Estado:** Hora RTC, estado de relés, modo activo y día del ciclo.
3. **WiFi:** SSID e IP (solo si no hay clientes conectados).
