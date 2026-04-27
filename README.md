# Jardín 24hs - Controlador Microclima V2.0

---

## 📜 Historial de Versiones

### [v2.0.0] - 2026-04-23 — **Migración ESP32-S3**
- **BREAKING**: Migración completa de Arduino Uno a **ESP32-S3** (Waveshare ESP32-S3-Touch-LCD-2).
- **NEW**: **Dashboard Web** — Interfaz mobile-first accesible desde el celular via WiFi AP.
- **NEW**: **OLED 1.3" SSD1306** — Pantalla de estado pasiva con rotación automática (3 pantallas).
- **NEW**: **WebSocket** — Datos en tiempo real y control remoto de actuadores.
- **NEW**: **NVS (Preferences)** — Reemplaza EEPROM para persistencia de configuración.
- **NEW**: **LittleFS** — Sistema de archivos para dashboard web y futuro datalog.
- **RENAME**: `manualExt/manualVent` → `controlExt/controlVent` (modo control AUTO/ON/OFF).
- **REMOVED**: LCD Keypad Shield y sistema de menú por botones (reemplazado por web).

### [v1.0.7] - 2026-04-05 (branch: main)
- Control Manual Temporal para Ventilador y Extractor.
- Retorno automático a AUTO tras 30 minutos.

*(Ver branch `main` para historial completo de V1.0)*

---

## 🌟 Descripción del Sistema

Sistema de control ambiental diseñado para optimizar las fases de Crecimiento y Floración en entornos cerrados. V2.0 es un salto generacional respecto al prototipo Arduino (V1.0).

### Arquitectura V2.0

```
┌─────────────────────────────────────────┐
│        ESP32-S3 (Waveshare)             │
├─────────────────────────────────────────┤
│  DHT22 ──── Temperatura + Humedad       │
│  DS3231 ──── Reloj RTC (I2C)            │
│  OLED 1.3" ── Display estado (I2C)      │
│  3x Relés ── Luz / Extractor / Ventilador│
│  WiFi AP ──── "Microclima" (192.168.4.1) │
│  LittleFS ── Dashboard web + Datalog    │
└─────────────────────────────────────────┘
        │
        │ WiFi (sin internet)
        ▼
┌─────────────────────────────────────────┐
│  📱 Celular / Tablet / PC              │
│  Navegador → http://192.168.4.1        │
│  Dashboard en tiempo real (WebSocket)   │
│  Configuración + Control manual         │
└─────────────────────────────────────────┘
```

### Interfaces Duales

| Interfaz | Rol | Detalle |
|---|---|---|
| **OLED 1.3"** | Estado pasivo | 3 pantallas rotativas (Clima, Estado, WiFi) |
| **Dashboard Web** | Control principal | Monitoreo en vivo, config, control manual, alertas |

---

## 🛠️ Hardware V2.0

| Componente | GPIO | Función |
|---|---|---|
| **DHT22** | `GPIO 1` | Temperatura y Humedad |
| **RTC DS3231** | `SDA:10 / SCL:11` | Reloj en tiempo real (I2C) |
| **OLED SSD1306** | `SDA:10 / SCL:11` | Display 128x64 (I2C, comparte bus) |
| **Relé Luz** | `GPIO 40` | Control de iluminación |
| **Relé Extractor** | `GPIO 41` | Extracción de aire |
| **Relé Ventilador** | `GPIO 42` | Circulación interna 24/7 |

---

## 🌐 API del Servidor

| Endpoint | Método | Descripción |
|---|---|---|
| `/` | GET | Dashboard HTML |
| `/api/estado` | GET | JSON con estado actual |
| `/api/config` | GET | JSON con configuración |
| `/api/config` | POST | Guardar nueva configuración |
| `/ws` | WebSocket | Stream en tiempo real |

---

## 🔧 Compilación y Flasheo

```powershell
# Compilar firmware
pio run -e esp32s3

# Subir firmware
pio run -e esp32s3 -t upload

# Subir archivos web a LittleFS
pio run -e esp32s3 -t uploadfs

# Monitor serial
pio device monitor -b 115200
```

---

## 📁 Estructura del Proyecto

```
├── data/               # Archivos web (LittleFS → se suben a la flash)
│   └── index.html      # Dashboard mobile-first
├── include/
│   ├── Configuracion.h # Pines, structs, perfiles
│   ├── Control.h       # Lógica VPD / Actuadores
│   ├── Pantalla.h      # OLED 1.3" SSD1306
│   └── Servidor.h      # WiFi AP + HTTP + WebSocket
├── src/
│   ├── main.cpp        # Setup + Loop
│   ├── Configuracion.cpp
│   ├── Control.cpp
│   ├── Pantalla.cpp
│   └── Servidor.cpp
└── platformio.ini      # Configuración ESP32-S3
```
