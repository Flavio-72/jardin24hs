# Jardín 24hs - Controlador Microclima V1.0

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
El extractor opera como el pulmón del recinto basándose en dos mecanismos complementarios:
- **Gestión Atmosférica Normal:** Toma acción si se violan los topes térmicos o hay sobre-saturación de humedad (evitando la formación de mohos). Utiliza márgenes de alivio (-2°C de Temperatura, -5% de Humedad) para detener los motores únicamente cuando el cuarto se haya renovado por completo. Esto protege la vida mecánica del equipo contra parpadeos de encendido/apagado por mínimas variaciones.
- **Protección contra Frío Extremo:** El mayor peligro nocturno en recintos apartados. Si la temperatura perfora la línea de riesgo de las plantas (menor a 18°C), el sistema bloquea temporalmente la extracción principal para no inyectar aire exterior helado que destruya el calor natural acumulado. Para suplantarlo, acciona un **Pulso de Purga** estricto: obliga al motor de extracción a encenderse durante **2 minutos enteros al iniciar cada hora de reloj**, garantizando el recambio de oxígeno viciado sin sacrificar el ecosistema térmico.

### 3. Circulación de Aire Ininterrumpida
El algoritmo destina un canal específico de energía a permanecer encendido de forma ininterrumpida (Todo el día y la noche). En un recinto hermético, el movimiento del aire interno no debe detenerse para garantizar un respiro constante sobre la cara exterior de las hojas (Capa Límite).

### 4. Respaldo contra Cortes de Energía (EEPROM)
El ecosistema es inmune a los apagones repentinos. Al volver la electricidad, el módulo RTC reestablece la línea de tiempo exacta e inyecta al Arduino los datos que guardó en su disco duro de estado sólido interno (EEPROM). La placa continúa rigiendo el ecosistema sin parpadear.

### 5. Salida de Datos (Monitoreo Terminal)
La placa matriz ejecuta informes continuos en segundo plano. Exporta un paquete de texto comprimido tipo JSON cada 15 segundos mediante su cable USB. Esto permite la lectura externa de datos por computadora o abre la puerta a un futuro registro histórico en tarjetas de memoria SD conectadas a una segunda placa principal.

---

## 🔧 Uso y Mantenimiento de Fábrica

El código fuente obedece al ecosistema **PlatformIO**. 
```powershell
# Compilar y grabar el código al Controlador:
pio run -t upload
```
