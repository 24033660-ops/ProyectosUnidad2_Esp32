# ⌨️ Práctica 6: Control Térmico ON/OFF con Histéresis y Teclado Matricial (ESP32)

Sistema de control de temperatura en lazo cerrado con histéresis desarrollado con el microcontrolador **ESP32**. Emplea un sensor digital **DS18B20** para medir la temperatura en tiempo real, un **teclado matricial 4x4** junto con un **pulsador selector** para ingresar numéricamente los umbrales mínimo y máximo, una pantalla **LCD I2C 16x2** para visualizar la temperatura y la captura de datos, y un **módulo de 2 relés** para alternar entre una etapa de calentamiento (foco) y una de enfriamiento (celda Peltier).

---

## 📚 Librerías Necesarias

- **`Arduino.h` y `Wire.h`**: Librerías nativas incluidas en el paquete base del ESP32 para funciones generales y comunicación I2C.
- **`OneWire.h`**: Librería externa para gestionar el protocolo de comunicación de un solo hilo (*1-Wire*) del sensor DS18B20 (autor: Paul Stoffregen).
- **`DallasTemperature.h`**: Librería externa que gestiona la lectura en grados Celsius y la resolución del sensor DS18B20 (autor: Miles Burton).
- **`LiquidCrystal_I2C.h`**: Librería externa para controlar pantallas LCD alfanuméricas mediante el adaptador I2C `PCF8574` (autor: Frank de Brabander).
- **`Keypad.h`**: Librería externa para el escaneo matricial de filas y columnas del teclado 4x4 (autores: Mark Stanley, Alexander Brevig).

---

## 🧰 Componentes, Sensores y Actuadores Utilizados

- **Microcontrolador:** ESP32 (DevKit V1 de 30 pines).
- **Sensor de temperatura:** 1x Sensor digital **DS18B20** (encapsulado TO-92 o sonda impermeable) + 1x Resistencia *pull-up* de $4.7\,\text{k}\Omega$.
- **Interfaz de entrada:**
  - 1x Teclado matricial de membrana **4x4** (16 teclas).
  - 1x Pulsador (*push button* normalmente abierto - NA) para selección de parámetro.
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
| **DS18B20 (`DATA`)** | `GPIO 4` | `1-Wire` | Pin central (`DATA`) al `GPIO 4` con resistencia de **$4.7\,\text{k}\Omega$** hacia `3.3V`. |
| **Botón Selector (`Min/Max`)** | `GPIO 25` | `INPUT_PULLUP` | Conmuta el parámetro activo (`*`); terminal opuesto directo a `GND`. |
| **Teclado 4x4 (`F1, F2, F3, F4`)** | `GPIO 19, 18, 5, 27` | Barrido Matricial | Pines 1 al 4 del conector del teclado (filas). |
| **Teclado 4x4 (`C1, C2, C3, C4`)** | `GPIO 32, 14, 23, 26` | Barrido Matricial | Pines 5 al 8 del conector del teclado (columnas). |
| **LCD I2C (`SDA`)** | `GPIO 21` | `I2C SDA` | Línea de datos I2C del módulo LCD (`0x27`). |
| **LCD I2C (`SCL`)** | `GPIO 22` | `I2C SCL` | Línea de reloj I2C del módulo LCD (`0x27`). |
| **Relé 1 (Foco)** | `GPIO 2` | `OUTPUT` | Señal `IN1` del módulo de relés (*Active LOW*: `LOW` = encendido). |
| **Relé 2 (Peltier)** | `GPIO 15` | `OUTPUT` | Señal `IN2` del módulo de relés (*Active LOW*: `LOW` = encendido). |

---

## ⚙️ Funcionamiento del Sistema

1. **Adquisición digital no bloqueante (`DS18B20`):** Configurado a **9 bits** de resolución ($0.5\,^\circ\text{C}$ de paso) y conversión asíncrona (`setWaitForConversion(false)`), actualiza la temperatura cada `1000 ms` sin bloquear la lectura del teclado matricial.
2. **Configuración numérica mediante Teclado 4x4 y Selector:**
   - **Selección de umbral (`GPIO 25`):** Con antirrebote de `180 ms`, alterna el cursor `*` entre `Min` (`30 °C` por defecto) y `Max` (`60 °C` por defecto).
   - **Ingreso de dígitos (`0`–`9`):** Al presionar teclas numéricas, la fila inferior del LCD cambia temporalmente a `Ingresando: ...` acumulando hasta 3 dígitos.
   - **Confirmación (`#`) y borrado (`*`):** La tecla `#` valida y guarda el número ingresado (verificando que `tempMin < tempMax` para conservar la histéresis), mientras que `*` limpia el *buffer* de entrada.
3. **Lógica de control por histéresis:**
   - **Estado `CALENTANDO` (`[CAL]`):** Mantiene activado el **Foco** (`GPIO 2` en `LOW`) y apagada la **Peltier** hasta que $T_{\text{actual}} \ge T_{\text{max}}$.
   - **Estado `ENFRIANDO` (`[ENF]`):** Al superar el límite superior, apaga el foco y enciende la **Celda Peltier** (`GPIO 15` en `LOW`) hasta que la temperatura descienda a $T_{\text{actual}} \le T_{\text{min}}$, reiniciando el ciclo.

---

## 🛡️ Cuidados de Sensores, Actuadores y Precauciones Eléctricas

- **Seguridad con Corriente Alterna (110 V / 220 V CA):** El circuito del foco trabaja con tensión de red potencialmente mortal. Aísla todas las uniones de CA con cinta aislante o termofit, mantén el cableado de potencia físicamente separado de la protoboard y **nunca toques la cara inferior del módulo de relés** mientras esté energizado.
- **Disipación térmica obligatoria en la Celda Peltier:** Nunca energices la celda Peltier sin un **disipador de aluminio con ventilador y pasta térmica** acoplados en su cara caliente, ya que se sobrecalienta y daña permanentemente en segundos. Aliméntala siempre desde una **fuente externa de 12 V CC**, nunca desde el ESP32.
- **Resistencia *pull-up* y polaridad del DS18B20:** Coloca obligatoriamente la resistencia de **$4.7\,\text{k}\Omega$** entre `DATA` (`GPIO 4`) y `3.3V` para evitar lecturas erróneas (`-127 °C`). Revisa la distribución de pines (`GND`, `DATA`, `VDD`) antes de encender: invertir la polaridad destruye el sensor instantáneamente.
- **Cuidado mecánico del teclado matricial:** Evita doblar en ángulo recto o jalar la cinta flexible (*ribbon cable*) del teclado de membrana 4x4, ya que las pistas conductoras internas de tinta de plata se fracturan con facilidad; fíjalo sobre una superficie plana y evita presionar teclas durante el encendido del ESP32 (especialmente la fila 3 conectada al *strapping pin* `GPIO 5`).
- **Precaución con el pin de programación (`GPIO 2`):** Si al subir código desde la computadora aparece el error *"Failed to connect to ESP32"*, desconecta temporalmente el cable del `GPIO 2` durante la carga, ya que algunos módulos de relés *Active LOW* mantienen ese pin en nivel alto e impiden entrar en modo *flash*.
