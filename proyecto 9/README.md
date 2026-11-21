# 📈 Práctica 9: Controlador PID Discreto de Temperatura con Dimmer Mecánico (ESP32)

Sistema avanzado de control continuo de temperatura en lazo cerrado basado en un algoritmo **PID discreto** con compensación *anti-windup* y término derivativo sobre la medición, implementado en el microcontrolador **ESP32**. Mide la temperatura con un sensor digital **DS18B20** a 10 bits de resolución, permite sintonizar en tiempo real el *setpoint* (`T_ref`) y las tres ganancias (`Kp`, `Ki`, `Kd`) con valores decimales mediante un **teclado matricial 4x4**, alterna información en una pantalla **LCD I2C 16x2** y regula la potencia térmica de un foco incandescente posicionando un **dimmer de CA** mediante un **servomotor** calibrado.

---

## 📚 Librerías Necesarias

- **`Arduino.h` y `Wire.h`**: Librerías nativas incluidas en el paquete base del ESP32 para funciones generales y comunicación I2C.
- **`OneWire.h`**: Librería externa para gestionar el protocolo de comunicación de un solo hilo (*1-Wire*) del sensor DS18B20 (autor: Paul Stoffregen).
- **`DallasTemperature.h`**: Librería externa que gestiona la lectura en grados Celsius y la resolución del sensor DS18B20 (autor: Miles Burton).
- **`LiquidCrystal_I2C.h`**: Librería externa para controlar pantallas LCD alfanuméricas mediante el adaptador I2C `PCF8574` (autor: Frank de Brabander).
- **`Keypad.h`**: Librería externa para el escaneo matricial de filas y columnas del teclado 4x4 (autores: Mark Stanley, Alexander Brevig).
- **`ESP32Servo.h`**: Librería externa para generar señales PWM precisas mediante los temporizadores de hardware del ESP32 (autor: Kevin Harrington / John K. Bennett).

---

## 🧰 Componentes, Sensores y Actuadores Utilizados

- **Microcontrolador:** ESP32 (DevKit V1 de 30 pines).
- **Sensor de temperatura:** 1x Sensor digital **DS18B20** (encapsulado TO-92 o sonda impermeable) + 1x Resistencia *pull-up* de $4.7\,\text{k}\Omega$.
- **Interfaz de usuario:**
  - 1x Teclado matricial de membrana **4x4** (16 teclas).
  - 1x Pantalla **LCD 16x2** con módulo adaptador **I2C** (dirección `0x27`).
- **Actuador electromecánico y etapa de potencia:**
  - 1x Servomotor estándar o micro-servo (ej. `SG90` o `MG996R`) con acople mecánico aislante al eje del dimmer.
  - 1x Módulo **Dimmer rotativo de CA** (basado en TRIAC) para regulación de potencia.
  - 1x Foco incandescente (carga resistiva dimmeable en CA) con socket y clavija.
- **Alimentación:** Fuente externa regulada de **5 V CC** para el servomotor (con `GND` común al ESP32) y alimentación de red (110 V / 220 V CA) para el circuito del dimmer.

---

## 🔌 Tabla de Conexiones (Pinout)

| Componente | Pin ESP32 | Modo / Bus | Conexión física y observaciones |
| :--- | :--- | :--- | :--- |
| **DS18B20 (`DATA`)** | `GPIO 4` | `1-Wire` | Pin central (`DATA`) al `GPIO 4` con resistencia de **$4.7\,\text{k}\Omega$** hacia `3.3V`. |
| **Servomotor (`Señal`)** | `GPIO 18` | `PWM (50 Hz)` | Cable de señal al `GPIO 18`; `VCC` a fuente externa de `5V` y `GND` a tierra común. |
| **LCD I2C (`SDA`)** | `GPIO 21` | `I2C SDA` | Línea de datos I2C del módulo LCD (`0x27`). |
| **LCD I2C (`SCL`)** | `GPIO 22` | `I2C SCL` | Línea de reloj I2C del módulo LCD (`0x27`). |
| **Teclado 4x4 (`F1, F2, F3, F4`)** | `GPIO 13, 12, 14, 27` | Barrido Matricial | Pines 1 al 4 del conector del teclado (filas). |
| **Teclado 4x4 (`C1, C2, C3, C4`)** | `GPIO 26, 25, 33, 32` | Barrido Matricial | Pines 5 al 8 del conector del teclado (columnas). |

---

## ⚙️ Funcionamiento y Algoritmo PID Discreto

1. **Adquisición de temperatura a 10 bits (`Ts = 1.0 s`):** El sensor **DS18B20** se configura a **10 bits** de resolución ($0.25\,^\circ\text{C}$ de precisión) en modo asíncrono (`setWaitForConversion(false)`), ejecutando el lazo de control cada periodo de muestreo fijo $T_s = 1.0\,\text{s}$.
2. **Ecuaciones del Controlador PID Discreto:**
   - **Término Proporcional ($P$):** Responde al error instantáneo $e(k) = T_{\text{ref}} - T_{\text{medida}}(k)$:
     $$P(k) = K_p \cdot e(k)$$
   - **Término Integral ($I$) con *Anti-Windup Clamping*:** Acumula el error histórico $I_{\text{tent}} = I(k-1) + K_i \cdot e(k) \cdot T_s$, pero **detiene la integración** cuando la salida total satura fuera del rango lineal $[0\%, 100\%]$ (limitando el acumulado entre $-50\%$ y $+50\%$) para evitar sobreimpulsos prolongados tras cambios bruscos de referencia.
   - **Término Derivativo ($D$) sobre la Medición:** Calcula la tasa de cambio directamente sobre la temperatura medida en lugar del error, eliminando el golpe derivativo (*Derivative Kick*) cuando el usuario modifica el *setpoint*:
     $$D(k) = -K_d \cdot \frac{T_{\text{medida}}(k) - T_{\text{medida}}(k-1)}{T_s}$$
   *(Valores iniciales por defecto: `T_ref = 40.0 °C`, `Kp = 10.0`, `Ki = 0.2`, `Kd = 1.0`).*
3. **Compensación de zona muerta mecánica:** Mapea linealmente la señal de control saturada $u(k) \in [0\%, 100\%]$ al rango operativo sintonizado del dimmer entre **`80°`** (`ANGULO_MIN`, umbral de encendido / $0\%$) y **`180°`** (`ANGULO_MAX`, potencia máxima / $100\%$):
   $$\theta_{\text{servo}} = 80^\circ + \left(\frac{u(k)}{100}\right) \cdot (180^\circ - 80^\circ)$$
4. **Sintonización en vivo (Teclado 4x4) y Pantalla LCD:**
   - **Teclas de selección:** `A` edita `T_ref` (`0`–`100`), `B` edita `Kp
