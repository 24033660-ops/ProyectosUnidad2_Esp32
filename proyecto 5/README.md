# 🌡️ Práctica 5: Control Térmico ON/OFF con Histéresis y Ajuste Analógico (ESP32)

Sistema de control de temperatura en lazo cerrado con banda de histéresis desarrollado con el microcontrolador **ESP32**. Utiliza un sensor analógico **LM35** con promediado digital para medir la temperatura del proceso, **dos potenciómetros** para ajustar de forma continua los umbrales mínimo y máximo ($25\,^\circ\text{C}$ a $60\,^\circ\text{C}$), una pantalla **LCD I2C 16x2** para visualizar el estado en tiempo real y un **módulo de 2 relés** para alternar entre una etapa de calentamiento (foco) y una de enfriamiento (celda Peltier).

---

## 📚 Librerías Necesarias

- **`Arduino.h` y `Wire.h`**: Librerías nativas incluidas en el paquete base del ESP32 para funciones generales, lectura analógica (ADC) y comunicación I2C.
- **`LiquidCrystal_I2C.h`**: Librería externa para controlar pantallas LCD alfanuméricas mediante el módulo adaptador I2C `PCF8574` (autor: Frank de Brabander).

---

## 🧰 Componentes, Sensores y Actuadores Utilizados

- **Microcontrolador:** ESP32 (DevKit V1 de 30 pines).
- **Sensor de temperatura:** 1x Sensor analógico lineal de precisión **LM35** (encapsulado TO-92, factor de escala $10\,\text{mV}/^\circ\text{C}$).
- **Interfaz de ajuste:** 2x Potenciómetros lineales de $10\,\text{k}\Omega$ (para `T_min` y `T_max`).
- **Visualización:** 1x Pantalla **LCD 16x2** con módulo adaptador **I2C** (dirección `0x27`).
- **Etapa de potencia y actuadores:**
  - 1x Módulo de **2 relés** de 5 V con optoacopladores (lógica *Active LOW*).
  - 1x Foco incandescente (carga resistiva de calentamiento en CA) con socket y clavija.
  - 1x Celda Peltier (ej. `TEC1-12706` a 12 V CC) con disipador de aluminio, ventilador y pasta térmica.
- **Fuentes de alimentación:** Fuente de 5 V para el ESP32/relés y fuente dedicada de 12 V CC (mínimo 5 A) para la celda Peltier.

---

## 🔌 Tabla de Conexiones (Pinout)

| Componente | Pin ESP32 | Modo / Bus | Conexión física y observaciones |
| :--- | :--- | :--- | :--- |
| **Sensor LM35 (`Vout`)** | `GPIO 34` | `INPUT` (`ADC1_CH6`) | Pin central (`Vout`) al `GPIO 34`; `+Vs` a `5V` (`VIN`) y `GND` a tierra común. |
| **Potenciómetro 1 (`T_min`)** | `GPIO 32` | `INPUT` (`ADC1_CH4`) | Pin central (cursor) al `GPIO 32`; extremos a **`3.3V`** y `GND`. |
| **Potenciómetro 2 (`T_max`)** | `GPIO 33` | `INPUT` (`ADC1_CH5`) | Pin central (cursor) al `GPIO 33`; extremos a **`3.3V`** y `GND`. |
| **LCD I2C (`SDA`)** | `GPIO 21` | `I2C SDA` | Línea de datos I2C del módulo LCD (`0x27`). |
| **LCD I2C (`SCL`)** | `GPIO 22` | `I2C SCL` | Línea de reloj I2C del módulo LCD (`0x27`). |
| **Relé 1 (Foco)** | `GPIO 2` | `OUTPUT` | Señal `IN1` del módulo de relés (*Active LOW*: `LOW` = encendido). |
| **Relé 2 (Peltier)** | `GPIO 15` | `OUTPUT` | Señal `IN2` del módulo de relés (*Active LOW*: `LOW` = encendido). |

---

## ⚙️ Funcionamiento del Sistema

1. **Adquisición analógica filtrada (`LM35`):** Cada `300 ms`, la función `leerLM35()` toma **30 muestras consecutivas** en el `GPIO 34` separadas por $100\,\mu\text{s}$ para reducir el ruido eléctrico propio del conversor ADC del ESP32. El promedio de 12 bits ($0\text{–}4095$) se convierte a temperatura mediante:
   $$T\,(^\circ\text{C}) = \frac{\left(\frac{\text{ADC}_{\text{prom}}}{4095.0} \times 3300\,\text{mV}\right)}{10\,\text{mV}/^\circ\text{C}}$$
2. **Ajuste continuo de umbrales con protección:** Los potenciómetros conectados a `GPIO 32` y `GPIO 33` se mapean al rango de **$25\,^\circ\text{C}$ a $60\,^\circ\text{C}$**. El programa incorpora una validación de seguridad (`if (tempMax <= tempMin) tempMax = tempMin + 1`) para garantizar que siempre exista al menos $1\,^\circ\text{C}$ de histéresis entre ambos límites.
3. **Control por ciclo térmico (Histéresis):**
   - **Estado `CALENTANDO` (`[CAL]`):** Mantiene activado el **Foco** (`GPIO 2` en `LOW`) y apagada la **Celda Peltier** hasta que $T_{\text{actual}} \ge T_{\text{max}}$.
   - **Estado `ENFRIANDO` (`[ENF]`):** Al superar el límite superior, apaga el foco y enciende la **Celda Peltier** (`GPIO 15` en `LOW`) hasta que la temperatura descienda a $T_{\text{actual}} \le T_{\text{min}}$, reiniciando el ciclo.
4. **Visualización en LCD y Puerto Serie:** La pantalla LCD muestra en la línea superior la temperatura medida y el estado activo (`[CAL]` o `[ENF]`), y en la línea inferior los valores configurados de `Min` y `Max`. Paralelamente, envía la telemetría al Monitor Serie a **`115200 baudios`**.

---

## 🛡️ Cuidados de Sensores, Actuadores y Precauciones Eléctricas

- **Polaridad y alimentación del LM35:** El sensor LM35 requiere entre **4 V y 30 V** para operar (conéctalo a `VIN` de 5 V); su salida entrega $10\,\text{mV}/^\circ\text{C}$ (máximo $1.5\,\text{V}$ a $150\,^\circ\text{C}$), por lo que es totalmente segura para el ADC de 3.3 V del ESP32. Verifica su distribución de pines (`+Vs`, `Vout`, `GND`) viendo la cara plana de frente: **invertir la polaridad calienta y destruye el integrado en segundos**.
- **Alimentación de potenciómetros a `3.3V` (nunca `5V`):** Conecta los extremos de ambos potenciómetros exclusivamente a **`3.3V`** y `GND`. Si los alimentaras con `5V`, al girarlos al máximo enviarías voltaje por encima del límite tolerable de los pines `GPIO 32` y `GPIO 33`, dañando el conversor analógico del microcontrolador.
- **Uso de canales `ADC1` (`GPIO 32, 33, 34`):** Estos tres pines pertenecen al bloque **ADC1** del ESP32, lo que garantiza lecturas estables y sin conflictos de hardware (además, el `GPIO 34` es de solo entrada y carece de resistencias *pull-up* internas que alteren la medición).
- **Seguridad con Corriente Alterna (110 V / 220 V CA):** El circuito del foco maneja voltaje de red eléctrico potencialmente mortal. Aísla todas las terminales de CA con cinta aislante o termofit, mantén el cableado de potencia alejado de la protoboard y **nunca toques la parte inferior de la placa de relés** mientras esté energizada.
- **Disipación térmica obligatoria en la Celda Peltier:** Nunca enciendas una celda Peltier sin un **disipador de aluminio con ventilador y pasta térmica** instalados en su cara caliente, ya que se sobrecalienta y daña en pocos segundos. Aliméntala siempre desde una **fuente externa de 12 V CC**, nunca desde el ESP32.
- **Precaución con pines de arranque (`GPIO 2` y `GPIO 15`):** Si al cargar un programa nuevo desde la computadora ocurre un error de conexión (*"Failed to connect to ESP32"*), desconecta temporalmente el cable del `GPIO 2` durante la carga, ya que ciertos módulos de relés *Active LOW* fuerzan ese pin a nivel alto e impiden el modo de programación.
