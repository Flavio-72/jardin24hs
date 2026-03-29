# Jardín 24hs - Microclima CBD

Sistema de control ambiental automatizado para cultivo medicinal, basado en **Arduino Uno**. Este proyecto se enfoca en la eficiencia, simplicidad y autonomía, utilizando una interfaz física de LCD y teclado.

## 🛠️ Hardware Utilizado
- **Cerebro:** Arduino Uno (R3 / Italy).
- **Interfaz:** LCD Keypad Shield (D1 RoBoT).
- **Sensores:** 
  - DHT11 (Temperatura y Humedad).
  - RTC DS3231 (Reloj en Tiempo Real de Alta Precisión).
- **Actuadores:** Módulo de 4 Relés mecánicos.

## 🚀 Funcionalidades Actuales (V0.1.1)
- Monitoreo en tiempo real de la hora (HH:MM:SS) en pantalla LCD.
- Validación de periféricos mediante teclado analógico.
- Sincronización automática de hora mediante compilación.

## 📖 Documentación
Puedes encontrar detalles específicos en la carpeta `/doc`:
- [Historial de Cambios (Changelog)](doc/CHANGELOG.md)
- [Desarrollo V0.1: RTC y Menú](doc/V0.1_RTC_Menu.md)
- [Glosario de Bugs Solucionados](doc/GLOSARIO_BUGS.md)

## 🔧 Instalación
Proyecto gestionado con **PlatformIO**. Para compilar y subir:
```powershell
pio run -t upload
```
