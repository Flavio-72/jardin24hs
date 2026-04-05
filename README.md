# Jardín 24hs - Controlador Microclima V1.0.5

---

## 📜 Historial de Versiones

### [v1.0.5] - 2026-04-05
- **NEW**: Implementado **Pulso de Respiro** (Renovación forzada de aire) de 5 minutos cada 3 horas para asegurar niveles de CO2.
- **LOGIC**: Prioridad de VPD sobre el pulso de respiro (coexistencia segura).

### [v1.0.4] - 2026-04-05
- **JSON**: Reporte serial enriquecido con `luz_on` (hora de inicio) y `dia` (día del ciclo actual).
- **JSON**: Estados de relés simplificados a `"on"` / `"off"` para futura integración con dashboards.

### [v1.0.2] - 2026-04-05
- **REFACTOR**: Traducción integral del código fuente a español (variables, funciones y comentarios).
- **UX**: Mensajes de sistema y menús 100% en español.

### [v1.0.1] - 2026-04-05
- **FIX**: Corregido `pinMode` de relés (Luz y Extractor) que impedía la conmutación física.
- **NEW**: Implementado **Contador de Días de Ciclo** (EEPROM) con edición manual sincronizada al RTC.
- **NEW**: Agregado **Auto-Home** (Vuelta a pantalla principal tras 10s de inactividad).
- **UI**: Rotación de 3 pantallas en monitoreo (Hora -> Clima -> Ciclo).
- **DOC**: Agregado informe de pines (Pinout) detallado.

---

Sistema de control ambiental pasivo-activo diseñado específicamente para optimizar las fases de Crecimiento y Floración de plantas en entornos cerrados (carpas, invernaderos interiores, viveros). Prioriza la lógica biológica, la autonomía ante cortes de energía y una experiencia de usuario simplificada.

## 🌟 Descripción del Sistema

La arquitectura de Microclima se centra en **perfiles pre-cargados orientados a la transpiración biológica** (Déficit de Presión de Vapor). 
Con solo asignarle el horario en el que debe simular el amanecer y en qué fase vital se encuentra la planta, el sistema asume el control sobre las luces y la renovación del aire. Busca equilibrar de forma agresiva la atmósfera en épocas calurosas, mientras cuida el calor de las raíces y el suelo durante los fríos intensos.

### 🛠️ Hardware Compatible
- **Placa Principal:** Arduino Uno (R3).
- **Interfaz Visual:** LCD Keypad Shield (Manejo con 5 botones direccionales).
- **Sensores:** 
  - Módulo de Humedad y Temperatura DHT22 (Precisión).
  - Módulo Reloj RTC DS3231 (Con batería CR2032 de respaldo para la hora).
- **Manejo de Potencia:** Módulo de Relés Optoacoplados de Lógica Inversa.

---

## 🚀 Funcionalidades Principales (V1.0)

### 1. Interfaz Simplificada y Ciclo Lumínico Automático
Configuración rápida desde la pantalla LCD. Desaparecen los ajustes manuales intrincados. Al configurar la hora local y elegir entre Crecimiento o Floración, el software calcula internamente la franja horaria para apagar las luces en base al fotoperíodo ideal: 18 horas de día para Crecimiento, 12 horas de día para Floración.

### 2. Extracción Inteligente (Control Atmosférico)
El extractor opera como el pulmón del recinto basándose en tres mecanismos complementarios:
- **Gestión Atmosférica Normal (VPD):** Toma acción si se violan los topes térmicos o hay sobre-saturación de humedad. Utiliza márgenes de alivio (-2°C de Temperatura, -5% de Humedad) para evitar el "flickeo" del motor.
- **Pulso de Respiro (Seguridad de CO2):** Incluso si el clima es perfecto, el sistema fuerza una extracción de **5 minutos cada 3 horas**. Esto garantiza la entrada de aire fresco y CO2, vital para la fotosíntesis, sin desgastar el motor.
- **Protección contra Frío Extremo (Winter Pulse):** Si la temperatura cae por debajo de 18°C, el sistema prioriza el calor y reduce la extracción a solo **2 minutos cada hora**, protegiendo el ecosistema térmico del cultivo.

### 3. Circulación de Aire Ininterrumpida
El algoritmo destina un canal específico de energía a permanecer encendido de forma ininterrumpida (Todo el día y la noche). En un recinto hermético, el movimiento del aire interno no debe detenerse para garantizar un respiro constante sobre la cara exterior de las hojas (Capa Límite).

### 4. Respaldo contra Cortes de Energía (EEPROM)
El ecosistema es inmune a los apagones repentinos. Al volver la electricidad, el módulo RTC reestablece la línea de tiempo exacta e inyecta al Arduino los datos que guardó en su disco duro de estado sólido interno (EEPROM). La placa continúa rigiendo el ecosistema sin parpadear.

### 5. Salida de Datos (Monitoreo Terminal)
La placa matriz ejecuta informes continuos en segundo plano. Exporta un paquete de texto comprimido tipo JSON cada 15 segundos mediante su cable USB. Esto permite la lectura externa de datos por computadora o abre la puerta a un futuro registro histórico en tarjetas de memoria SD conectadas a una segunda placa principal.

---

## 📋 Informe de Conexiones (Pinout)

Para el montaje del hardware en el Arduino Uno, utiliza el siguiente esquema de pines:

| Componente | Pin Arduino | Función |
| :--- | :---: | :--- |
| **Relé Luz** | `13` | Control de iluminación (Lógica Inversa) |
| **Relé Extractor** | `11` | Extracción de aire / Purga de seguridad |
| **Relé Ventilador** | `12` | Circulación interna (Encendido 24hs) |
| **Sensor DHT22** | `2` | Lectura de Temperatura y Humedad |
| **RTC DS3231** | `SDA / SCL` | Comunicación I2C (Hora en tiempo real) |
| **LCD RS** | `8` | Registro de Selección LCD |
| **LCD Enable** | `9` | Habilitación de pulso LCD |
| **LCD D4-D7** | `4, 5, 6, 7` | Bus de datos del LCD |
| **LCD Backlight** | `10` | Control de retroiluminación |
| **Botones Teclado** | `A0` | Lectura analógica de los 5 botones |

---

## 🔧 Uso y Mantenimiento de Fábrica

El código fuente obedece al ecosistema **PlatformIO**. 
```powershell
# Compilar y grabar el código al Controlador:
pio run -t upload
```
