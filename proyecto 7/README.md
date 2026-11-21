# 🎭 Práctica 7: Máscara Animatrónica Activada por Movimiento (Arduino)

Sistema animatrónico interactivo controlado por microcontrolador (**Arduino Uno / Nano**) que permanece en reposo hasta detectar presencia humana mediante un sensor infrarrojo pasivo **PIR (HC-SR501)**. Al activarse, desencadena una secuencia sincronizada de terror durante 5 segundos: reproduce efectos de sonido a través de un módulo **DFPlayer Mini**, genera parpadeos aleatorios mediante modulación PWM en **4 LEDs** ubicados en los ojos y articula la mandíbula de forma irregular con un **servomotor SG90**.

---

## 📚 Librerías Necesarias

- **`Servo.h`**: Librería nativa incluida en Arduino IDE para controlar servomotores mediante señales PWM.
- **`SoftwareSerial.h`**: Librería nativa incluida en Arduino IDE que habilita un puerto serie adicional por software en pines digitales para comunicarse con el reproductor MP3.
- **`DFRobotDFPlayerMini.h`**: Librería externa necesaria para enviar comandos al módulo `MP3-TF-16P` / DFPlayer Mini. Se instala desde el *Gestor de Librerías* de Arduino IDE buscando **`DFRobotDFPlayerMini`** (autor: DFRobot).

---

## 🧰 Componentes, Sensores y Actuadores Utilizados

- **Microcontrolador:** Placa Arduino compatible (Uno / Nano / Pro Mini, lógica de 5 V).
- **Sensor de presencia:** 1x Sensor infrarrojo pasivo de movimiento **PIR HC-SR501**.
- **Sistema de audio:**
  - 1x Módulo reproductor MP3 **DFPlayer Mini (`MP3-TF-16P`)**.
  - 1x Tarjeta **microSD** (máx. 32 GB, formateada en **FAT32**) con el archivo **`0001.mp3`** en la raíz.
  - 1x Altavoz / bocina pequeña ($3\,\text{W}$, $4\,\Omega$ u $8\,\Omega$).
  - 1x Resistencia de $1\,\text{k}\Omega$ (para filtrar ruido en la línea `TX` $\rightarrow$ `RX`).
- **Actuador mecánico:** 1x Micro-servomotor **SG90** (mecanismo de apertura de mandíbula).
- **Actuadores luminosos:** 4x Diodos LED (2 por cada ojo) + 4x Resistencias limitadoras de $220\,\Omega$ a $330\,\Omega$.
- **Alimentación:** Fuente externa regulada de **5 V CC** (mínimo $1.5\,\text{A}$ – $2\,\text{A}$).

---

## 🔌 Tabla de Conexiones (Pinout)

| Componente | Pin Arduino | Modo / Tipo | Conexión física y observaciones |
| :--- | :--- | :--- | :--- |
| **Servomotor SG90 (`Señal`)** | `Pin 9` | `PWM (~)` | Cable naranja al `Pin 9`; `VCC` (rojo) a `5V` externo y `GND` (marrón) a tierra común. |
| **Sensor PIR (`OUT`)** | `Pin 7` | `INPUT` | Pin central de señal del HC-SR501 al `Pin 7`; `VCC` a `5V` y `GND` a tierra. |
| **LEDs de los ojos (x4)** | `Pines 3, 5, 6, 11` | `OUTPUT` (`PWM ~`) | Cada pin $\rightarrow$ Resistencia $220\,\Omega$ $\rightarrow$ Ánodo (`+`) del LED; cátodos (`-`) a `GND`. |
| **DFPlayer Mini (`TX`)** | `Pin 2` | `SoftwareSerial RX` | Conectar directo al pin `TX` (pin 3) del módulo DFPlayer Mini. |
| **DFPlayer Mini (`RX`)** | `Pin 4` | `SoftwareSerial TX` | `Pin 4` $\rightarrow$ Resistencia **$1\,\text{k}\Omega$** $\rightarrow$ Pin `RX` (pin 2) del DFPlayer Mini. |
| **Semilla aleatoria** | `Pin A0` | `INPUT` (Flotante) | Dejar sin conectar; captura ruido eléctrico ambiental para `randomSeed()`. |

---

## ⚙️ Funcionamiento del Sistema

1. **Calibración inicial del sensor PIR (~30 segundos):** Al encender el circuito, el programa ejecuta un ciclo de espera de **30 segundos** mientras parpadea tenuemente los 4 LEDs (`brillo = 30`). Este tiempo es indispensable para que el sensor HC-SR501 estabilice su lectura térmica infrarroja del entorno y evite falsas activaciones al arrancar.
2. **Estado de espera (*Standby*):** Concluida la calibración, los ojos pasan al brillo de reposo (`BRILLO_REPOSO = 0`), la mandíbula se posiciona cerrada (`90°`) y el microcontrolador sondea el sensor PIR cada `50 ms`.
3. **Secuencia animatrónica (`susto()`):** Al detectar movimiento (`PIN_PIR == HIGH`), inicia la reproducción de `0001.mp3` con volumen **`27`** (escala de `0` a `30`) y ejecuta una animación aleatoria durante **`5000 ms`** (`DURACION_SUSTO`):
   - **Iluminación errática:** Varía el ciclo de trabajo PWM de cada LED de forma independiente entre intensidad media (`80`) y máxima (`255`) para simular destellos perturbadores.
   - **Articulación irregular de mandíbula:** Alterna posiciones aleatorias alrededor de `BOCA_ABIERTA` ($50^\circ\text{ a }64^\circ$) y `BOCA_CERRADA` ($81^\circ\text{ a }90^\circ$) con intervalos variables ($80\,\text{ms}$ a $220\,\text{ms}$) para imitar el movimiento de una risa o grito.
4. **Tiempo de enfriamiento (*Cooldown*):** Al cumplirse los 5 segundos, detiene el audio, cierra la boca (`90°`), apaga los ojos y espera **`3000 ms`** (`ENFRIAMIENTO`) antes de permitir una nueva activación.

---

## 🛡️ Cuidados de Sensores, Actuadores y Precauciones Eléctricas

- **Formato y orden de archivos en la microSD:** El módulo DFPlayer Mini exige que la memoria microSD esté formateada en **FAT32** (capacidad máxima de 32 GB). Guarda el archivo de sonido directamente en la carpeta raíz con el nombre **`0001.mp3`**; evita copiar y borrar múltiples archivos sin formatear, ya que `reproductor.play(1)` reproduce el primer archivo indexado en la tabla de asignación de la tarjeta.
- **Resistencia de $1\,\text{k}\Omega$ en la comunicación del DFPlayer:** Aunque el DFPlayer Mini se alimenta a `5 V`, su lógica serial interna trabaja a `3.3 V`. Intercalar una resistencia de **$1\,\text{k}\Omega$** entre el `Pin 4` (`TX` del Arduino) y el pin `RX` del DFPlayer protege el chip de audio y elimina el zumbido de fondo en la bocina durante la reproducción.
- **Fuente de alimentación con suficiente amperaje:** El pico de corriente al reproducir audio a volumen alto (`27/30`) simultáneamente con el movimiento del servomotor SG90 puede superar los $800\,\text{mA}$ – $1\,\text{A}$. Alimenta el sistema con una **fuente externa de 5 V CC (2 A)** en lugar del puerto USB de la computadora para evitar reinicios inesperados o cortes de sonido.
- **Ajuste físico del sensor PIR HC-SR501:** La placa del sensor posee dos potenciómetros naranjas de ajuste: **Sensibilidad (`Sx`)** (alcance de $3\,\text{m}$ a $7\,\text{m}$) y **Tiempo de retención (`Tx`)**. Gira el potenciómetro de tiempo (`Tx`) completamente al **mínimo** (sentido antihorario) para que la duración del evento sea controlada exclusivamente por las variables `DURACION_SUSTO` y `ENFRIAMIENTO` del código.
- **Calibración mecánica de la mandíbula:** Antes de fijar definitivamente el brazo del servomotor SG90 a la estructura de la máscara, verifica que el recorrido entre `50°` y `90°` no choque con topes físicos. Si la mandíbula encuentra resistencia mecánica, el servo entrará en corriente de rotor bloqueado (*stall*), sobrecalentándose y dañando sus engranes internos.
- **Protección individual de los 4 LEDs:** Conecta cada LED de los ojos con su propia resistencia limitadora de **$220\,\Omega$ a $330\,\Omega$** en serie; nunca conectes los 4 LEDs en paralelo usando una sola resistencia ni directamente a los pines PWM (`3, 5, 6, 11`).
