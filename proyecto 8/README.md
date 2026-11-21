# 🎛️ Práctica 8: Controlador Proporcional (P) de Temperatura con Dimmer Mecánico (ESP32)

Sistema de control continuo de temperatura en lazo cerrado basado en un algoritmo **Proporcional (P)** implementado en el microcontrolador **ESP32**. Mide la temperatura del proceso mediante un sensor digital **DS18B20**, permite configurar en tiempo real el *setpoint* (`T_ref`) y la ganancia proporcional (`Kp`) con valores decimales a través de un **teclado matricial 4x4**, despliega variables operativas en una pantalla **LCD I2C 16x2** con doble vista y regula la potencia térmica de un foco incandescente posicionando mecánicamente la perilla de un **dimmer de CA** mediante un **servomotor** calibrado.

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
  - 1x Servomotor estándar o micro-servo (ej. `SG90` o `MG996R`) con acople mecánico al eje del dimmer.
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

## ⚙️ Funcionamiento y Ley de Control

1. **Adquisición de temperatura (`500 ms`):** El sensor **DS18B20** opera a **9 bits** de resolución ($0.5\,^\circ\text{C}$ de paso) en modo asíncrono (`setWaitForConversion(false)`), actualizando la variable `T_medida` cada `500 ms` sin bloquear la lectura del teclado.
2. **Algoritmo de Control Proporcional (P):** En cada ciclo de muestreo se calcula el error térmico y la acción de control porcentual saturada entre $0\%$ y $100\%$:
   $$e(t) = T_{\text{ref}} - T_{\text{medida}}$$
   $$u(t) = \text{constrain}\big(K_p \cdot e(t),\; 0\%,\; 100\%\big)$$
   *(Valores iniciales por defecto: `T_ref = 40.0 °C` y `Kp = 5.0 %/°C`).*
3. **Compensación de zona muerta mecánica:** Dado que el dimmer comercial comienza a conducir energía a partir de la mitad de su recorrido, el código mapea linealmente la salida $u(t)$ entre **`90°`** (`ANGULO_MIN`, umbral de encendido / $0\%$) y **`180°`** (`ANGULO_MAX`, potencia máxima / $100\%$):
   $$\theta_{\text{servo}} = 90^\circ + \left(\frac{u(t)}{100}\right) \cdot (180^\circ - 90^\circ)$$
4. **Menú interactivo en Teclado Matricial 4x4:**
   - **Tecla `A`:** Entra al modo de edición del *Setpoint* (`T_ref`, rango válido de `0.0` a `100.0 °C`).
   - **Tecla `B`:** Entra al modo de edición de la ganancia proporcional (`Kp`, rango válido de `0.0` a `50.0 %/°C`).
   - **Teclas `0`–`9` y `*`:** Permiten escribir valores numéricos con punto decimal (la tecla `*` inserta el carácter `.` hasta un máximo de 6 caracteres).
   - **Tecla `#`:** Confirma y guarda el parámetro ingresado regresando a la vista normal.
   - **Tecla `C`:** Borra el texto que se está escribiendo en pantalla.
   - **Tecla `D`:** Cancela la edición en curso; si está en modo normal, alterna la pantalla LCD entre la **Vista Principal** (`T`, `SP`, `E`, `OUT%`) y la **Vista Secundaria** (`Kp` y `SERVO DEG`).

---

## 🛡️ Cuidados de Sensores, Actuadores y Precauciones Eléctricas

- **Aislamiento galvánico y seguridad con Corriente Alterna (110 V / 220 V CA):** El dimmer regula voltaje de red eléctrico potencialmente mortal. Utiliza un **acople mecánico aislante** (plástico, nylon o pieza impresa en 3D) entre la cruceta del servomotor y el vástago del potenciómetro del dimmer para garantizar que no exista contacto eléctrico hacia el servo ni el ESP32. Aísla todas las conexiones de CA con cinta aislante o termofit.
- **Compatibilidad de la carga en el Dimmer:** Utiliza exclusivamente un **foco incandescente o halógeno** (carga resistiva). Los focos LED estándar no dimmeables parpadean erráticamente, no regulan temperatura de forma lineal y pueden dañar el TRIAC del dimmer.
- **Calibración de topes mecánicos (`90°` a `180°`):** Antes de fijar el acople mecánico al potenciómetro del dimmer, verifica que el rango angular enviado por el servomotor no sobrepase los topes físicos internos del potenciómetro; de lo contrario, el servo quedará bloqueado (*stall current*), sobrecalentándose y dañando sus engranes.
- **Alimentación externa para el servomotor:** Alimenta el servomotor mediante una **fuente externa de 5 V CC** compartiendo tierra (`GND`) con el ESP32. Alimentarlo desde el pin de 3.3 V o exigirle corriente desde el regulador interno de la placa provocará caídas de voltaje y reinicios del microcontrolador.
- **Precaución con el pin de arranque (`GPIO 12`):** La fila 2 del teclado matricial (`4, 5, 6, B`) está conectada al `GPIO 12`, el cual es un *strapping pin* del ESP32 que selecciona el voltaje de la memoria flash al encender. **Evita mantener presionada cualquier tecla de esa fila mientras enciendes o reinicias el ESP32**, ya que podría impedir el arranque del programa.
- **Resistencia *pull-up* y polaridad del DS18B20:** Coloca obligatoriamente la resistencia de **$4.7\,\text{k}\Omega$** entre `DATA` (`GPIO 4`) y `3.3V` para evitar lecturas erróneas (`-127 °C`), y verifica la orientación de sus pines (`GND`, `DATA`, `VDD`) antes de energizar el circuito.
