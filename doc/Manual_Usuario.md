# Manual de Usuario — Jardín 24hs

¡Bienvenido al sistema **Jardín 24hs**! Este controlador automatiza el clima de tu cultivo para que no tengas que preocuparte por encender las luces o controlar el calor.

## 1. Primeros Pasos
Al encender el equipo, este creará una red WiFi propia.
1. Busca en tu celular la red WiFi llamada: **Microclima**.
2. Conéctate usando la contraseña: **micro2025**.
3. Una vez conectado, abre tu navegador y entra a: `http://192.168.4.1`.

## 2. Modos de Cultivo
El sistema tiene 3 modos pre-configurados:
- **Crecimiento (Vege):** Configurado para dar 18 horas de luz. Etapas de germinación, desarrollo de ramas y hojas nuevas, esquejes y plantas madres.
- **Floración (Flora):** Configurado para dar 12 horas de luz. Activa la fase de producción.
- **Personalizado:** Te permite definir tus propios horarios y límites de temperatura/humedad.

## 3. Configuración del Clima
En el panel de control podrás ver:
- **Temperatura y Humedad:** Medidas en tiempo real.
- **Estado de Equipos:** Indica si la luz, el extractor o el ventilador están encendidos.
- **Ajustes:** Aquí puedes cambiar el modo de cultivo o ajustar los límites máximos de calor y humedad para que el extractor se active automáticamente.

## 4. Consejos de Uso
- **Sensor:** Mantén el sensor (punta blanca) a la altura de las puntas de las plantas, pero que no le dé la luz directamente para evitar mediciones falsas.
- **Seguridad:** No sobrepases la capacidad de los relés (máximo 10A por salida).
- **Corte de luz:** Si se corta la energía, el sistema tiene un módulo con batería interna (RTC) que mantiene la hora exacta y volverá a su estado normal automáticamente al regresar la luz.

> IMPORTANTE:
> **Mantenimiento de la Batería (RTC):**
> El sistema utiliza una batería de litio **CR2032** (estándar de reloj/computadora) para mantener la hora.
> - **Duración estimada:** 2 a 5 años.
> - **Síntoma de agotamiento:** Si tras un corte de luz el sistema inicia con una hora incorrecta (ej: 00:00 del año 2000), la batería debe ser reemplazada.
> - **Reemplazo:** El usuario puede cambiarla fácilmente abriendo el gabinete y deslizando la batería vieja fuera de su soporte. No requiere servicio técnico especializado.
