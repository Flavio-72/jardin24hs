# Glosario de Bugs Solucionados (Troubleshooting)

Este documento registra los inconvenientes técnicos encontrados durante el desarrollo y su solución, para referencia futura y aprendizaje sistemático.

---

## 🐞 Bug 01: Caracteres "Colgados" en LCD
- **Fecha:** 2026-03-29
- **Síntoma:** Al pasar de un texto largo (ej: "IZQUIERDA") a uno corto (ej: "UP"), las últimas letras del texto anterior permanecían en la pantalla (ej: "UPQUIERDA").
- **Causa:** La función `lcd.print()` no borra el resto de la línea, solo sobrescribe los caracteres necesarios.
- **Solución:** Aplicar **Padding de espacios**. Asegurar que todas las cadenas de texto que se imprimen en la misma posición tengan el mismo número de caracteres, rellenando con espacios en blanco al final.
  - **Ejemplo:** `"UP     "` en lugar de `"UP"`.
- **Alternativa Descartada:** Usar `lcd.clear()` en cada loop provoca parpadeo visual molesto.

---

## 🐞 Bug 02: Error de Compilación `SPI.h: No such file or directory`
- **Fecha:** 2026-03-29
- **Síntoma:** Error fatal al compilar librerías de Adafruit (RTC o DHT).
- **Causa:** El buscador de dependencias de PlatformIO no incluía las librerías base `SPI` y `Wire` automáticamente.
- **Solución:** 
  1. Añadir `SPI` y `Wire` a las `lib_deps` en `platformio.ini`.
  2. Activar `lib_ldf_mode = deep+` para forzar un escaneo profundo de dependencias.
