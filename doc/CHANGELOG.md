# Historial de Versiones (Changelog)

## [V0.3.1] - 2026-03-29
### Añadido
- **Software:** 
  - `[x]` Incluir `EEPROM.h` y definir direcciones de memoria.
  - `[x]` Crear funciones `cargarConfig()` y `guardarConfig()`.
  - `[x]` Implementar el estado `CAMBIAR_MODO` en la FSM.
  - `[x]` Limpiar `main.cpp` para que solo tenga el reloj y el selector de modo.
  - `[x]` Crear documentación `doc/V0.3.1_EEPROM_Estructura.md`.
- **Persistencia:** El sistema recuerda el modo elegido incluso tras una pérdida total de energía.

## [V0.1.2] - 2026-03-29
### Actualizado
- **Software:** Se añadió soporte para RTC DS3231.
- **Interfaz:** Visualización de hora en línea 1 del LCD (`HH:MM:SS`).
- **Fix:** Corregido bug de caracteres residuales en la línea 2 (padding de espacios).

## [V0.0.1] - 2026-03-29
### Inicial
- **Entorno:** Configuración inicial de PlatformIO para Arduino Uno.
- **Hardware:** Test de LCD Keypad Shield (D1 RoBoT) y validación de botones analógicos.
- **Repositorio:** Sincronización con GitHub (rama `main`).
