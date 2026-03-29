# Bitácora del Proyecto: Microclima CBD 24hs

Registro cronológico de avances técnicos del sistema de automatización basado en Arduino Uno.

---

## 29/03/2026 - Fase 1: Cimientos y Validación de Hardware

### Hito 1: Inicialización del Entorno
- **Configuración Git:** Se inicializó el repositorio local para control de versiones.
- **Estructura:** Uso del entorno PlatformIO en Windows.
- **Pines Definidos para Relés:**
  - Relé 1 (LUZ): Pin 2
  - Relé 2 (VENTILACION): Pin 3
  - Relé 3 (EXTRACCION): Pin 11
  - Relé 4 (RIEGO): Pin 12

### Hito 2: Configuración de Dependencias (platformio.ini)
- Se añadieron las librerías oficiales para máxima compatibilidad:
  - `LiquidCrystal` (LCD Keypad Shield).
  - `RTClib` de Adafruit (Reloj en tiempo real).
  - `DHT sensor library` de Adafruit (Temperatura y Humedad).
  - `SPI` y `Wire` (Buses de datos fundamentales, añadidos para corregir error de compilación).

### Hito 3: Hola Mundo Creativo (LCD + Teclado)
- **Objetivo:** Validar que el Shield D1 RoBoT interactúa correctamente.
- **Resultado:** El sistema muestra "Iniciando..." seguido de una detección dinámica de botones en la segunda línea del LCD.
- **Variables:** Implementadas en español (`botonPresionado`, `valorAnalogo`, etc.) según requerimiento.
