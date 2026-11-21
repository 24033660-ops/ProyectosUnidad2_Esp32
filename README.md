# ⚡ ProyectosUnidad2_Esp32

Repositorio con las prácticas de sistemas de control y automatización desarrolladas con microcontroladores (ESP32 / Arduino). Dentro de la carpeta de cada práctica se adjuntan los 📄 **documentos de reporte**, el 💻 **código fuente en uso** y el 🔌 **diagrama de conexiones** correspondiente.

## 📂 Contenido del repositorio

- 🌡️ **proyecto 5:** Control térmico ON/OFF con histéresis mediante sensor analógico LM35, dos potenciómetros para ajustar los límites de temperatura mínima y máxima, pantalla LCD I2C 16x2 y dos módulos de relé para alternar entre un foco calefactor y una celda Peltier.
- ⌨️ **proyecto 6:** Control térmico ON/OFF con histéresis utilizando sensor digital DS18B20, teclado matricial 4x4 y botón selector para configurar los umbrales de temperatura, pantalla LCD I2C 16x2 y dos relés para el control de calentamiento (foco) y enfriamiento (celda Peltier).
- 🎭 **Proyecto 7:** Máscara animatrónica activada por movimiento mediante sensor PIR (HC-SR501); al detectar presencia, reproduce efectos de audio con un módulo DFPlayer Mini, parpadea 4 LEDs mediante PWM en los ojos y articula la mandíbula con un servomotor SG90.
- 🎛️ **proyecto 8:** Controlador Proporcional (P) de temperatura con sensor digital DS18B20, teclado matricial 4x4 para ajustar el *setpoint* (`T_ref`) y la ganancia (`Kp`), pantalla LCD I2C 16x2 y un servomotor calibrado para regular mecánicamente la potencia de un *dimmer*.
- 📈 **proyecto 9:** Controlador PID discreto de temperatura (con *anti-windup* y término derivativo sobre la medición) usando sensor digital DS18B20, teclado matricial 4x4 para sintonizar `T_ref`, `Kp`, `Ki` y `Kd`, pantalla LCD I2C 16x2 con vista alternada y un servomotor acoplado a un *dimmer* para modular la potencia térmica.
